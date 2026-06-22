#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/idf_additions.h"
#include <hd44780.h>
#include <esp_idf_lib_helpers.h>
#include "driver/gpio.h"
#include "esp_bit_defs.h"
#include "esp_err.h"
#include "hal/uart_types.h"
#include "esp_log.h"
#include "nvs.h"
#include "portmacro.h"
#include "nvs_flash.h"

#include "victron_mppt.h"
#include "state_of_charge.h"

static const char *TAG = "solar_battery";

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define DISPLAY_REGISTER_SELECT_PIN 4
#define DISPLAY_READ_WRITE_SELECT_PIN 5
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

#define INIT_STATE_OF_CHARGE_FROM_NVS true

const uint8_t char_data[] =
{
    // Empty battery
    0b00100,
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,

    // 20% battery
    0b00100,
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b11111,

    // 40% battery
    0b00100,
    0b11111,
    0b10001,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11111,

    // 60% battery
    0b00100,
    0b11111,
    0b10001,
    0b10001,
    0b11111,
    0b11111,
    0b11111,
    0b11111,

    // 80% battery
    0b00100,
    0b11111,
    0b10001,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,

    // 100% battery
    0b00100,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
};

typedef enum battery_charge_level_t {
    CHARGE_LEVEL_0_PERCENT = 0,
    CHARGE_LEVEL_20_PERCENT = 1,
    CHARGE_LEVEL_40_PERCENT = 2,
    CHARGE_LEVEL_60_PERCENT = 3,
    CHARGE_LEVEL_80_PERCENT = 4,
    CHARGE_LEVEL_100_PERCENT = 5,
} battery_charge_level_t;

static QueueHandle_t process_data_queue;
static QueueHandle_t update_display_queue;

static void victron_mppt_uart_event_task(void *pvParameters)
{
    static victron_mppt_t victron_mppt;
    static victron_mppt_handle_t handle = &victron_mppt;

    ESP_ERROR_CHECK(victron_mppt_init(handle, UART_NUM_2, VICTRON_MPPT_UART_RX_PIN, VICTRON_MPPT_UART_TX_PIN));

    while (1) {
        ESP_ERROR_CHECK(victron_mppt_listen_uart(handle));
        if (handle->uart_rx.length > 0) {
            victron_mppt_uart_packet_t uart_packet;
            uart_packet.buffer = malloc(handle->uart_rx.length + 1);
            uart_packet.length = handle->uart_rx.length;
            if (!uart_packet.buffer) {
                ESP_LOGE(TAG, "No memory");
            } else {
                memcpy(uart_packet.buffer, handle->uart_rx.buffer, handle->uart_rx.length);
                xQueueSend(process_data_queue, &uart_packet, portMAX_DELAY);
            }
        }
    }
    ESP_ERROR_CHECK(victron_mppt_free(handle));
    vTaskDelete(NULL);
}

static void victron_mppt_process_data_task(void *pvParameters)
{
    victron_mppt_uart_packet_t uart_packet = {0};
    victron_mppt_uart_packet_handle_t uart_packet_handle = &uart_packet;

    while (1) {
        if (xQueueReceive(process_data_queue, uart_packet_handle, portMAX_DELAY)) {
            victron_mppt_data_t victron_mppt_data = {0};
            victron_mppt_data_handle_t victron_mppt_data_handle = &victron_mppt_data;
            ESP_LOGI(TAG, "Processing data");
            ESP_ERROR_CHECK(victron_mppt_parse_text(uart_packet_handle, victron_mppt_data_handle));
            victron_mppt_print_data(victron_mppt_data_handle);
            xQueueSend(update_display_queue, victron_mppt_data_handle, portMAX_DELAY);
            free(uart_packet_handle->buffer);
        }
    }
    vTaskDelete(NULL);
}

static void hd44780_update_display_task(void *pvParameters)
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

    // Upload charge level characters to LCD RAM
    hd44780_upload_character(&lcd, CHARGE_LEVEL_0_PERCENT, char_data + (CHARGE_LEVEL_0_PERCENT * 8));
    hd44780_upload_character(&lcd, CHARGE_LEVEL_20_PERCENT, char_data + (CHARGE_LEVEL_20_PERCENT * 8));
    hd44780_upload_character(&lcd, CHARGE_LEVEL_40_PERCENT, char_data + (CHARGE_LEVEL_40_PERCENT * 8));
    hd44780_upload_character(&lcd, CHARGE_LEVEL_60_PERCENT, char_data + (CHARGE_LEVEL_60_PERCENT * 8));
    hd44780_upload_character(&lcd, CHARGE_LEVEL_80_PERCENT, char_data + (CHARGE_LEVEL_80_PERCENT * 8));
    hd44780_upload_character(&lcd, CHARGE_LEVEL_100_PERCENT, char_data + (CHARGE_LEVEL_100_PERCENT * 8));

    victron_mppt_data_t victron_mppt_data = {0};
    victron_mppt_data_handle_t victron_mppt_data_handle = &victron_mppt_data;
    state_of_charge_t state_of_charge = {0};
    state_of_charge_handle_t state_of_charge_handle = &state_of_charge;

    ESP_ERROR_CHECK(state_of_charge_init(state_of_charge_handle, AMPERE_HOURS_TO_MILLI_COULOMB(8), AMPERE_HOURS_TO_MILLI_COULOMB(9)));

    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("nvs_storage", NVS_READWRITE, &nvs_handle));

    // Read charge and capacity
    if (INIT_STATE_OF_CHARGE_FROM_NVS) {
        ESP_LOGI(TAG, "Reading state of charge from NVS");
        nvs_err = nvs_get_i64(nvs_handle, "charge_mc", &state_of_charge_handle->charge_mc);
        switch (nvs_err) {
            case ESP_OK:
                ESP_LOGI(TAG, "Found charge %" PRIi64 " mC", state_of_charge_handle->charge_mc);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(TAG, "Charge is not initialized in NVS yet");
                break;
            default:
                ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(nvs_err));
        }
        nvs_err = nvs_get_i64(nvs_handle, "capacity_mc", &state_of_charge_handle->capacity_mc);
        switch (nvs_err) {
            case ESP_OK:
                ESP_LOGI(TAG, "Found capacity %" PRIi64 " mC", state_of_charge_handle->capacity_mc);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                ESP_LOGW(TAG, "Capacity is not initialized in NVS yet");
                break;
            default:
                ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(nvs_err));
        }
    }

    char display_lines[4][21];
    memset(display_lines, 0, sizeof(display_lines));

    while (1) {
        if (xQueueReceive(update_display_queue, victron_mppt_data_handle, portMAX_DELAY)) {
            if (victron_mppt_data_handle->state_of_operation == FLOAT) {
                ESP_ERROR_CHECK(state_of_charge_calibrate_capacity_at_full(state_of_charge_handle));
                ESP_ERROR_CHECK(nvs_set_i64(nvs_handle, "capacity_mc", state_of_charge_handle->capacity_mc));
            }

            // TODO find a better way to determine empty state
            if (strcmp(victron_mppt_data_handle->load_output_state, "OFF") == 0) {
                ESP_ERROR_CHECK(state_of_charge_calibrate_charge_at_empty(state_of_charge_handle));
            }

            ESP_ERROR_CHECK(nvs_set_i64(nvs_handle, "charge_mc", state_of_charge_handle->charge_mc));

            // Commit to NVS
            ESP_ERROR_CHECK(nvs_commit(nvs_handle));

            // Update state of charge
            state_of_charge_coulomb_count_update(state_of_charge_handle, victron_mppt_data_handle->battery_current);

            // Update display
            ESP_LOGI(TAG, "Updating display");
            snprintf(display_lines[0], sizeof(display_lines[0]), "\x08 %ld mV %ld mA", victron_mppt_data_handle->battery_voltage, victron_mppt_data_handle->battery_current);
            snprintf(display_lines[1], sizeof(display_lines[1]), "Charge: %ld %%", state_of_charge_handle->percentage);
            snprintf(display_lines[2], sizeof(display_lines[2]), "SV: %ld mV", victron_mppt_data_handle->solar_voltage);
            snprintf(display_lines[3], sizeof(display_lines[3]), "SW: %ld W", victron_mppt_data_handle->solar_power);
            hd44780_gotoxy(&lcd, 0, 0);
            hd44780_puts(&lcd, display_lines[0]);
            hd44780_gotoxy(&lcd, 0, 1);
            hd44780_puts(&lcd, display_lines[1]);
            hd44780_gotoxy(&lcd, 20, 0);
            hd44780_puts(&lcd, display_lines[2]);
            hd44780_gotoxy(&lcd, 20, 1);
            hd44780_puts(&lcd, display_lines[3]);
        }
    }
    nvs_close(nvs_handle);
    vTaskDelete(NULL);
}

void app_main(void)
{
    process_data_queue = xQueueCreate(5, sizeof(victron_mppt_uart_packet_t));
    update_display_queue = xQueueCreate(5, sizeof(victron_mppt_data_t));
    xTaskCreate(victron_mppt_uart_event_task, "uart_event", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(victron_mppt_process_data_task, "process_data", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
    xTaskCreate(hd44780_update_display_task, "update_display", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL);
}
