#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include "driver/gpio.h"
#include "esp_bit_defs.h"
#include "hal/uart_types.h"

#include "victron_mppt.h"

static const char *TAG = "solar_battery";

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define DISPLAY_REGISTER_SELECT_PIN 16
#define DISPLAY_READ_WRITE_SELECT_PIN 17
#define DISPLAY_ENABLE_PIN 18
#define DISPLAY_DATA_BIT_0_PIN 19
#define DISPLAY_DATA_BIT_1_PIN 21
#define DISPLAY_DATA_BIT_2_PIN 22
#define DISPLAY_DATA_BIT_3_PIN 23
#define DISPLAY_DATA_BIT_4_PIN 25
#define DISPLAY_DATA_BIT_5_PIN 26
#define DISPLAY_DATA_BIT_6_PIN 27
#define DISPLAY_DATA_BIT_7_PIN 32
#define DISPLAY_CLOCK_FREQUENCY 270000
#define DISPLAY_CMD_BITS 8
#define DISPLAY_PARAM_BITS 8

#define VICTRON_MPPT_UART_RX_PIN 16
#define VICTRON_MPPT_UART_TX_PIN 17

const uint8_t char_data[] =
{
    0x04, 0x0e, 0x0e, 0x0e, 0x1f, 0x00, 0x04, 0x00,
    0x1f, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x1f, 0x00
};

const uint8_t battery_chars[] =
{
    0x1F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, // Left bracket empty
    0x1F, 0x10, 0x17, 0x17, 0x17, 0x17, 0x10, 0x1F, // Left bracket full
    0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, // Middle bracket empty
    0x1F, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F, // Middle bracket full
    0x1F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1F, // Right bracket empty
    0x1F, 0x01, 0x1d, 0x1d, 0x1d, 0x1d, 0x01, 0x1F, // Right bracket full
};

const uint8_t full_uart_message[] =
{
    0x0d, 0x0a, 0x50, 0x49, 0x44, 0x09, 0x30, 0x78, 0x41, 0x30, 0x37, 0x35, 0x0d, 0x0a, 0x46, 0x57,  // |..PID.0xA075..FW|
    0x09, 0x31, 0x37, 0x34, 0x0d, 0x0a, 0x53, 0x45, 0x52, 0x23, 0x09, 0x48, 0x51, 0x32, 0x35, 0x34,  // |.174..SER#.HQ254|
    0x30, 0x34, 0x4a, 0x44, 0x55, 0x51, 0x0d, 0x0a, 0x56, 0x09, 0x31, 0x32, 0x38, 0x38, 0x30, 0x0d,  // |04JDUQ..V.12880.|
    0x0a, 0x49, 0x09, 0x30, 0x0d, 0x0a, 0x56, 0x50, 0x56, 0x09, 0x31, 0x30, 0x0d, 0x0a, 0x50, 0x50,  // |.I.0..VPV.10..PP|
    0x56, 0x09, 0x30, 0x0d, 0x0a, 0x43, 0x53, 0x09, 0x30, 0x0d, 0x0a, 0x4d, 0x50, 0x50, 0x54, 0x09,  // |V.0..CS.0..MPPT.|
    0x30, 0x0d, 0x0a, 0x4f, 0x52, 0x09, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31,  // |0..OR.0x00000001|
    0x0d, 0x0a, 0x45, 0x52, 0x52, 0x09, 0x30, 0x0d, 0x0a, 0x4c, 0x4f, 0x41, 0x44, 0x09, 0x4f, 0x4e,  // |..ERR.0..LOAD.ON|
    0x0d, 0x0a, 0x49, 0x4c, 0x09, 0x30, 0x0d, 0x0a, 0x48, 0x31, 0x39, 0x09, 0x31, 0x31, 0x0d, 0x0a,  // |..IL.0..H19.11..|
    0x48, 0x32, 0x30, 0x09, 0x30, 0x0d, 0x0a, 0x48, 0x32, 0x31, 0x09, 0x30, 0x0d, 0x0a, 0x48, 0x32,  // |H20.0..H21.0..H2|
    0x32, 0x09, 0x30, 0x0d, 0x0a, 0x48, 0x32, 0x33, 0x09, 0x30, 0x0d, 0x0a, 0x48, 0x53, 0x44, 0x53,  // |2.0..H23.0..HSDS|
    0x09, 0x31, 0x35, 0x0d, 0x0a, 0x43, 0x68, 0x65, 0x63, 0x6b, 0x73, 0x75, 0x6d, 0x09, 0xce,     // |.15..Checksum..|
};

void display_victron_mppt_data(hd44780_t *lcd, victron_mppt_data_t *data)
{
    char first_line[20];
    snprintf(first_line, sizeof(first_line), "PV: %.01fV, BV: %.01fV", (float) data->solar_voltage / 1000, (float) data->battery_voltage / 1000);
    hd44780_gotoxy(lcd, 0, 0x14);
    hd44780_puts(lcd, first_line);
}

void setup_hd44780(void *pvParameters)
{
    gpio_set_level(DISPLAY_READ_WRITE_SELECT_PIN, 0);
    gpio_config_t display_read_write_select_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_READ_WRITE_SELECT_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_read_write_select_cfg);

    gpio_set_level(DISPLAY_READ_WRITE_SELECT_PIN, 0);

    hd44780_t lcd =
    {
        .write_cb = NULL,
        .font = HD44780_FONT_5X8,
        .lines = 2,
        .pins = {
            .rs = DISPLAY_REGISTER_SELECT_PIN,
            .e  = DISPLAY_ENABLE_PIN,
            .d4 = DISPLAY_DATA_BIT_4_PIN,
            .d5 = DISPLAY_DATA_BIT_5_PIN,
            .d6 = DISPLAY_DATA_BIT_6_PIN,
            .d7 = DISPLAY_DATA_BIT_7_PIN,
            .bl = HD44780_NOT_USED
        }
    };

    ESP_ERROR_CHECK(hd44780_init(&lcd));

    hd44780_upload_character(&lcd, 0, char_data);
    hd44780_upload_character(&lcd, 1, char_data + 8);
    hd44780_upload_character(&lcd, 2, battery_chars);
    hd44780_upload_character(&lcd, 3, battery_chars + 8);
    hd44780_upload_character(&lcd, 4, battery_chars + 16);
    hd44780_upload_character(&lcd, 5, battery_chars + 24);
    hd44780_upload_character(&lcd, 6, battery_chars + 32);
    hd44780_upload_character(&lcd, 7, battery_chars + 40);

    // hd44780_gotoxy(&lcd, 0, 0);
    // hd44780_puts(&lcd, "\x08 Hej Tobias og Karl Eivind!");
    // hd44780_gotoxy(&lcd, 0, 1);
    // hd44780_puts(&lcd, "\x09");
    // hd44780_gotoxy(&lcd, 20, 1);
    // hd44780_puts(&lcd, "\x0a\x0b\x0c\x0d\x0e\x0f");
    // hd44780_gotoxy(&lcd, 20, 1);
    // hd44780_puts(&lcd, "Batteri \x0b\x0d\x0d\x0c\x0e");

    // char time[16];

    // while (1)
    // {
    //     hd44780_gotoxy(&lcd, 2, 1);

    //     snprintf(time, 7, "%" PRIu32 "  ", get_time_sec());
    //     time[sizeof(time) - 1] = 0;

    //     hd44780_puts(&lcd, time);

    //     vTaskDelay(pdMS_TO_TICKS(500));
    // }
    // victron_mppt_data_t victron_mppt_data;
    // victron_mppt_parse_text(victrin, &victron_mppt_data);
    // display_victron_mppt_data(&lcd, &victron_mppt_data);
}

victron_mppt_t victron_mppt;
victron_mppt_handle_t victron_mppt_handle = &victron_mppt;
victron_mppt_data_t victron_mppt_data;

static void victron_mppt_task(void *pvParameters)
{
    victron_mppt_init(victron_mppt_handle, UART_NUM_2, VICTRON_MPPT_UART_RX_PIN, VICTRON_MPPT_UART_TX_PIN);
    memcpy(victron_mppt_handle->uart_rx_buffer, full_uart_message, sizeof(full_uart_message));
    victron_mppt_handle->uart_rx_data_length = sizeof(full_uart_message);
    victron_mppt_print_data(&victron_mppt_data);
    victron_mppt_parse_text(victron_mppt_handle, &victron_mppt_data);
    victron_mppt_print_data(&victron_mppt_data);

    while (1){
        // victron_mppt_read_data(victron_mppt_handle);
        // victron_mppt_print_data(&victron_mppt_data);
        // 500 / portTICK_PERIOD_MS
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    victron_mppt_free(victron_mppt_handle);
}

void app_main(void)
{
  xTaskCreate(victron_mppt_task, "victron_mppt_task", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
  // xTaskCreate(setup_hd44780, "setup_hd44780", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}
