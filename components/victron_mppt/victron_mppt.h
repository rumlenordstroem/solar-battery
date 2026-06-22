#pragma once

#include <stdint.h>
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include "freertos/idf_additions.h"
#include "esp_err.h"

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

typedef victron_mppt_data_t *victron_mppt_data_handle_t;

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

typedef enum victron_mppt_off_reason_t {
    NO_INPUT_POWER = 0x0000001,
    SWITCHED_OFF_POWER_SWITCH = 0x0000002,
    SWITCHED_OFF_DEVICE_MODE_REGISTER = 0x0000004,
    REMOTE_INPUT = 0x00000008,
    PROTECTION_ACTIVE = 0x00000010,
    PAYGO = 0x00000020,
    BMS = 0x00000040,
    ENGINE_SHUTDOWN_DETECTION = 0x00000080,
    ANALYSING_INPUT_VOLTAGE = 0x00000100,
} victron_mppt_off_reason_t;

typedef enum victron_mppt_state_of_operation_t {
    OFF = 0,
    FAULT = 2,
    BULK = 3,
    ABSORPTION = 4,
    FLOAT = 5,
    EQUALIZE = 7,
    STARTING_UP = 245,
    AUTO_EQUALIZE = 247,
    EXTERNAL_CONTROL = 252,
} victron_mppt_state_of_operation_t;

typedef enum victron_mppt_error_code_t {
    NO_ERROR = 0,
    BATTERY_VOLTAGE_TOO_HIGH = 2,
    CHARGER_TEMPERATURE_TOO_HIGH = 17,
    CHARGER_OVER_CURRENT = 18,
    CHARGER_CURRENT_REVERSED = 19,
    BULK_TIME_LIMIT_EXCEEDED = 20,
    CURRENT_SENSOR_ISSUE = 21,
    TERMINALS_OVERHEATED = 26,
    INPUT_VOLTAGE_TOO_HIGH = 33,
    INPUT_CURRENT_TOO_HIGH = 34,
    INPUT_SHUTDOWN_EXCESSIVE_BATTERY_VOLTAGE = 38,
    INPUT_SHUTDOWN_CURRENT_FLOW_IN_OFF_MODE = 39,
    LOST_COMMUNICATION = 65,
    SYNCHRONISED_CHARGING_DEVICE_CONFIGURATION_ISSUE = 66,
    BMS_LOST_CONNECTION = 67,
    NETWORK_MISCONFIGURED = 68,
    FACTORY_CALIBRATION_DATA_LOST = 116,
    INVALID_FIRMWARE = 117,
    USER_INVALID_SETTINGS = 119,
} victron_mppt_error_code_t;

typedef struct {
    uint8_t *buffer;
    size_t length;
} victron_mppt_uart_packet_t;

typedef victron_mppt_uart_packet_t *victron_mppt_uart_packet_handle_t;

typedef struct {
    uart_port_t uart_port;
    victron_mppt_uart_packet_t uart_rx;
    QueueHandle_t uart_event_queue;
} victron_mppt_t;

typedef victron_mppt_t *victron_mppt_handle_t;

esp_err_t victron_mppt_init(victron_mppt_handle_t victron_mppt, uart_port_t uart_port, gpio_num_t rx_gpio_num, gpio_num_t tx_gpio_num);
esp_err_t victron_mppt_free(victron_mppt_handle_t victron_mppt);
esp_err_t victron_mppt_uart_read_data(victron_mppt_handle_t victron_mppt);
esp_err_t victron_mppt_parse_text(victron_mppt_uart_packet_handle_t uart_packet, victron_mppt_data_handle_t data);
esp_err_t victron_mppt_print_data(victron_mppt_data_handle_t data);
esp_err_t victron_mppt_get_field_from_str(const char *field_str, victron_mppt_field_t *field);
esp_err_t victron_mppt_set_value_from_str(const char *field, const char *value, victron_mppt_data_handle_t data);
esp_err_t victron_mppt_listen_uart(victron_mppt_handle_t victron_mppt);
