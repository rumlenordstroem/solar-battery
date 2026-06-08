#include <inttypes.h>
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
#include "esp_lcd_io_i80.h"
#include "freertos/FreeRTOS.h"
#include "hal/uart_types.h"

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

const char *serial_out_example = "PID     0xA075\nFW      174\nSER#    HQ25404JDUQ\nV       12920\nI       -10\nVPV     30\nPPV     0\nCS      0\nMPPT    0\nOR      0x00000001\nERR     0\nLOAD    ON\nIL      0\nH19     3\nH20     0\nH21     0\nH22     0\nH23     2\nHSDS    6\nChecksum        5";
const char *victron_mppt_data_format = "PID     0x%x\nFW      %d\nSER#    %s\nV       %d\nI       %d\nVPV     %d\nPPV     %d\nCS      %d\nMPPT    %d\nOR      0x%x\nERR     %d\nLOAD    %d\nIL      %d\nH19     %d\nH20     %d\nH21     %d\nH22     %d\nH23     %d\nHSDS    %d\nChecksum        %d";

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

void setup_hd44780(void)
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
        .lines = 4,
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

    hd44780_gotoxy(&lcd, 0, 0);
    hd44780_puts(&lcd, "\x08 Hello world!");
    hd44780_gotoxy(&lcd, 0, 1);
    hd44780_puts(&lcd, "\x09 ");

    char time[16];

    while (1)
    {
        hd44780_gotoxy(&lcd, 2, 1);

        snprintf(time, 7, "%" PRIu32 "  ", get_time_sec());
        time[sizeof(time) - 1] = 0;

        hd44780_puts(&lcd, time);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

const int uart_buffer_size = (1024 * 2);
QueueHandle_t uart_queue;

void setup_uart(void)
{
    const uart_port_t uart_num = UART_NUM_0;
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
    
    uart_config_t uart_config = {
        .baud_rate = 19200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t*)&length));
    length = uart_read_bytes(uart_num, data, length, 100);
}

typedef struct {
    uint16_t pid;
    uint16_t firmware_version;
    char serial_num[11];
    int32_t battery_voltage;
    int32_t battery_current;
    int32_t solar_voltage;
    int32_t solar_current;
    uint8_t state_of_operation;
    uint8_t tracker_operation_mode;
    uint32_t off_reason;
    uint8_t error_code;
    char *load_output_state[2];
    int32_t load_current;
    uint32_t yield_total;
    uint32_t yield_today;
    uint32_t maximum_power_today;
    uint32_t yield_yesterday;
    uint32_t maximum_power_yesterday;
    uint32_t day_sequence_number;
    uint32_t checksum;
} victron_mppt_data_t;

void app_main(void)
{
  victron_mppt_data_t victron_mppt_data;
  sscanf(
     serial_out_example,
     victron_mppt_data_format,
     &victron_mppt_data.pid,
     &victron_mppt_data.firmware_version,
     &victron_mppt_data.serial_num,
     &victron_mppt_data.battery_voltage,
     &victron_mppt_data.battery_current,
     &victron_mppt_data.solar_voltage,
     &victron_mppt_data.solar_current,
     &victron_mppt_data.state_of_operation,
     &victron_mppt_data.tracker_operation_mode,
     &victron_mppt_data.off_reason,
     &victron_mppt_data.error_code,
     &victron_mppt_data.load_output_state,
     &victron_mppt_data.load_current,
     &victron_mppt_data.yield_total,
     &victron_mppt_data.yield_today,
     &victron_mppt_data.maximum_power_today,
     &victron_mppt_data.yield_yesterday,
     &victron_mppt_data.maximum_power_yesterday,
     &victron_mppt_data.day_sequence_number,
     &victron_mppt_data.checksum
 );
  printf("Battery voltage: %ld\n", victron_mppt_data.battery_voltage);
  setup_hd44780();
}
