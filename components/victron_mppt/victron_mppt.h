#pragma once

#include <stdint.h>
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include "freertos/idf_additions.h"
#include "esp_err.h"

#define VICTRON_MPPT_DATA_FORMAT "\r\nPID\t0x%hx\r\nFW\t%hu\r\nSER#\t%s\r\nV\t%ld\r\nI\t%ld\r\nVPV\t%ld\r\nPPV\t%ld\r\nCS\t%hhu\r\nMPPT\t%hhu\r\nOR\t0x%lx\r\nERR\t%hhu\r\nLOAD\t%s\r\nIL\t%ld\r\nH19\t%ld\r\nH20\t%ld\r\nH21\t%ld\r\nH22\t%ld\r\nH23\t%ld\r\nHSDS\t%ld\r\nChecksum\t"
#define VICTRON_MPPT_DATA_EXAMPLE "\r\nPID\t0xA075\r\nFW\t174\r\nSER#\tHQ25404JDUQ\r\nV\t12920\r\nI\t-10\r\nVPV\t30\r\nPPV\t0\r\nCS\t0\r\nMPPT\t0\r\nOR\t0x00000001\r\nERR\t0\r\nLOAD\tON\r\nIL\t0\r\nH19\t3\r\nH20\t0\r\nH21\t0\r\nH22\t0\r\nH23\t2\r\nHSDS\t6\r\nChecksum\t"

#define VICTRON_MPPT_BAUD_RATE 19200
#define VICTRON_MPPT_DATA_BITS UART_DATA_8_BITS
#define VICTRON_MPPT_PARITY UART_PARITY_DISABLE
#define VICTRON_MPPT_STOP_BITS UART_STOP_BITS_1
#define VICTRON_MPPT_FLOWCTRL UART_HW_FLOWCTRL_DISABLE
#define VICTRON_MPPT_RX_BUFFER_SIZE 512
#define VICTRON_MPPT_UART_QUEUE_SIZE 5

typedef struct {
    uint16_t product_id;
    uint16_t firmware_version;
    char serial_number[12];
    int32_t battery_voltage;
    int32_t battery_current;
    int32_t solar_voltage;
    int32_t solar_power;
    uint8_t state_of_operation;
    uint8_t tracker_operation_mode;
    uint32_t off_reason;
    uint8_t error_code;
    char load_output_state[4];
    int32_t load_current;
    uint32_t yield_total;
    uint32_t yield_today;
    uint32_t maximum_power_today;
    uint32_t yield_yesterday;
    uint32_t maximum_power_yesterday;
    uint32_t day_sequence_number;
    uint32_t checksum;
} victron_mppt_data_t;

typedef enum victron_mppt_field_t {
    PRODUCT_ID,
    FIRMWARE_VERSION,
    SERIAL_NUMBER,
    BATTERY_VOLTAGE,
    BATTERY_CURRENT,
    SOLAR_VOLTAGE,
    SOLAR_POWER,
    STATE_OF_OPERATION,
    TRACKER_OPERATION_MODE,
    OFF_REASON,
    ERROR_CODE,
    LOAD_OUTPUT_STATE,
    LOAD_CURRENT,
    YIELD_TOTAL,
    YIELD_TODAY,
    MAXIMUM_POWER_TODAY,
    YIELD_YESTERDAY,
    MAXIMUM_POWER_YESTERDAY,
    DAY_SEQUENCE_NUMBER,
    CHECKSUM,
    UNKNOWN
} victron_mppt_field_t;

typedef struct {
    uart_port_t uart_port;
    size_t uart_rx_data_length;
    uint8_t *uart_rx_buffer;
    QueueHandle_t uart_event_queue;
} victron_mppt_t;

typedef victron_mppt_t * victron_mppt_handle_t;

esp_err_t victron_mppt_init(victron_mppt_handle_t handle, uart_port_t uart_port, gpio_num_t rx_gpio_num, gpio_num_t tx_gpio_num);
esp_err_t victron_mppt_free(victron_mppt_handle_t handle);
esp_err_t victron_mppt_read_data(victron_mppt_handle_t handle);
int victron_mppt_parse_text(victron_mppt_handle_t handle, victron_mppt_data_t *data);
void victron_mppt_print_data(victron_mppt_data_t *data);
victron_mppt_field_t victron_mppt_get_field_from_str(const char *field);
esp_err_t victron_mppt_set_value_from_str(const char *field, const char *value, victron_mppt_data_t *data);
esp_err_t victron_mppt_listen_uart(victron_mppt_handle_t handle);
