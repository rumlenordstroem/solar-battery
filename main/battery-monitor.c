#include <stdint.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_bit_defs.h"
#include "hal/uart_types.h"

#include "victron_mppt.h"

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

static uint32_t get_time_sec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

static const uint8_t char_data[] =
{
    0x04, 0x0e, 0x0e, 0x0e, 0x1f, 0x00, 0x04, 0x00,
    0x1f, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x1f, 0x00
};

static const uint8_t battery_chars[] =
{
    0x1F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, // Left bracket empty
    0x1F, 0x10, 0x17, 0x17, 0x17, 0x17, 0x10, 0x1F, // Left bracket full
    0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, // Middle bracket empty
    0x1F, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x1F, // Middle bracket full
    0x1F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1F, // Right bracket empty
    0x1F, 0x01, 0x1d, 0x1d, 0x1d, 0x1d, 0x01, 0x1F, // Right bracket full
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
    victron_mppt_data_t victron_mppt_data;
    victron_mppt_parse_text(VICTRON_MPPT_DATA_EXAMPLE, &victron_mppt_data);
    display_victron_mppt_data(&lcd, &victron_mppt_data);
}

const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;

void setup_uart(void)
{
    const uart_port_t uart_num = UART_NUM_0;
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
    
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &victron_mppt_uart_config));

    // Read UART data
    uint8_t data[128];
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    length = uart_read_bytes(uart_num, data, length, 100);
}

void app_main(void)
{
  // printf("Battery voltage: %ld\n", victron_mppt_data.battery_voltage);
  // setup_hd44780();
  // display_victron_mppt_data(&lcd, &victron_mppt_data);
  xTaskCreate(setup_hd44780, "setup_hd44780", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}
