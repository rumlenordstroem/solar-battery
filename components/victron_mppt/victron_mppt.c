#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "freertos/idf_additions.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include "esp_log.h"

#include "victron_mppt.h"

static const char *TAG = "victron_mppt";

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t victron_mppt_init(victron_mppt_handle_t victron_mppt, uart_port_t uart_port, gpio_num_t rx_gpio_num, gpio_num_t tx_gpio_num)
{
    CHECK_ARG(victron_mppt);

    const uart_config_t uart_config = {
        .baud_rate = VICTRON_MPPT_BAUD_RATE,
        .data_bits = VICTRON_MPPT_DATA_BITS,
        .parity = VICTRON_MPPT_PARITY,
        .stop_bits = VICTRON_MPPT_STOP_BITS,
        .flow_ctrl = VICTRON_MPPT_FLOWCTRL,
    };

    victron_mppt->uart_port = uart_port;
    victron_mppt->uart_rx.buffer = (uint8_t *) malloc(VICTRON_MPPT_RX_BUFFER_SIZE);
    if (!victron_mppt->uart_rx.buffer)
        return ESP_ERR_NO_MEM;

    CHECK(uart_driver_install(victron_mppt->uart_port, VICTRON_MPPT_RX_BUFFER_SIZE, 0, VICTRON_MPPT_UART_QUEUE_SIZE, &victron_mppt->uart_event_queue, 0));
    CHECK(uart_param_config(victron_mppt->uart_port, &uart_config));
    CHECK(uart_set_pin(victron_mppt->uart_port, tx_gpio_num, rx_gpio_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    return ESP_OK;
}

esp_err_t victron_mppt_free(victron_mppt_handle_t victron_mppt)
{
    CHECK_ARG(victron_mppt && victron_mppt->uart_rx.buffer);

    free(victron_mppt->uart_rx.buffer);
    victron_mppt->uart_rx.buffer = NULL;

    return ESP_OK;
}

 esp_err_t victron_mppt_check_text_checksum(victron_mppt_uart_packet_handle_t victron_mppt)
{
    CHECK_ARG(victron_mppt && victron_mppt->buffer);

    int checksum = 0;
    for (int i = 0; i < victron_mppt->length; i++) {
        checksum = (checksum + victron_mppt->buffer[i]) & 255; /* Take modulo 256 in account */
    }

    return checksum == 0 ? ESP_OK : ESP_ERR_INVALID_CRC;
}

esp_err_t victron_mppt_uart_read_data(victron_mppt_handle_t victron_mppt)
{
    CHECK_ARG(victron_mppt && victron_mppt->uart_rx.buffer);

    size_t length = 0;
    CHECK(uart_get_buffered_data_len(victron_mppt->uart_port, &length));

    if (length > 0) {
        victron_mppt->uart_rx.length = uart_read_bytes(victron_mppt->uart_port, victron_mppt->uart_rx.buffer, VICTRON_MPPT_RX_BUFFER_SIZE, 150 / portTICK_PERIOD_MS);

        ESP_LOGI(TAG, "UART RX buffer has %d bytes.", victron_mppt->uart_rx.length);
        ESP_LOG_BUFFER_HEXDUMP(TAG, victron_mppt->uart_rx.buffer, victron_mppt->uart_rx.length, ESP_LOG_INFO);

        // Clear buffer
        CHECK(uart_flush(victron_mppt->uart_port));
    } else {
        ESP_LOGI(TAG, "UART RX buffer is empty. Nothing was read.");
    }

    return ESP_OK;
}

esp_err_t victron_mppt_get_field_from_str(const char *field_str, victron_mppt_field_t *field)
{
    CHECK_ARG(field_str && field);

    if (strcmp("PID", field_str) == 0) *field = PRODUCT_ID;
    else if (strcmp("FW", field_str) == 0) *field = FIRMWARE_VERSION;
    else if (strcmp("SER#", field_str) == 0) *field = SERIAL_NUMBER;
    else if (strcmp("V", field_str) == 0) *field = BATTERY_VOLTAGE;
    else if (strcmp("I", field_str) == 0) *field = BATTERY_CURRENT;
    else if (strcmp("VPV", field_str) == 0) *field = SOLAR_VOLTAGE;
    else if (strcmp("PPV", field_str) == 0) *field = SOLAR_POWER;
    else if (strcmp("CS", field_str) == 0) *field = STATE_OF_OPERATION;
    else if (strcmp("MPPT", field_str) == 0) *field = TRACKER_OPERATION_MODE;
    else if (strcmp("OR", field_str) == 0) *field = OFF_REASON;
    else if (strcmp("ERR", field_str) == 0) *field = ERROR_CODE;
    else if (strcmp("LOAD", field_str) == 0) *field = LOAD_OUTPUT_STATE;
    else if (strcmp("IL", field_str) == 0) *field = LOAD_CURRENT;
    else if (strcmp("H19", field_str) == 0) *field = YIELD_TOTAL;
    else if (strcmp("H20", field_str) == 0) *field = YIELD_TODAY;
    else if (strcmp("H21", field_str) == 0) *field = MAXIMUM_POWER_TODAY;
    else if (strcmp("H22", field_str) == 0) *field = YIELD_YESTERDAY;
    else if (strcmp("H23", field_str) == 0) *field = MAXIMUM_POWER_YESTERDAY;
    else if (strcmp("HSDS", field_str) == 0) *field = DAY_SEQUENCE_NUMBER;
    else if (strcmp("Checksum", field_str) == 0) *field = CHECKSUM;
    else *field = UNKNOWN;

    return ESP_OK;
}

esp_err_t victron_mppt_set_value_from_str(const char *field_str, const char *value_str, victron_mppt_data_handle_t data)
{
    CHECK_ARG(field_str && value_str && data);

    victron_mppt_field_t field;

    CHECK(victron_mppt_get_field_from_str(field_str, &field));

    switch (field) {
    case PRODUCT_ID:
        data->product_id = (uint16_t) strtoul(value_str, NULL, 0);
        break;
    case FIRMWARE_VERSION:
        data->firmware_version = (uint16_t) strtoul(value_str, NULL, 10);
        break;
    case SERIAL_NUMBER:
        strncpy(data->serial_number, value_str, sizeof(data->serial_number) - 1);
        data->serial_number[sizeof(data->serial_number) - 1] = '\0';
        break;
    case BATTERY_VOLTAGE:
        data->battery_voltage = (int32_t) strtol(value_str, NULL, 10);
        break;
    case BATTERY_CURRENT:
        data->battery_current = (int32_t) strtol(value_str, NULL, 10);
        break;
    case SOLAR_VOLTAGE:
        data->solar_voltage = (int32_t) strtol(value_str, NULL, 10);
        break;
    case SOLAR_POWER:
        data->solar_power = (int32_t) strtol(value_str, NULL, 10);
        break;
    case STATE_OF_OPERATION:
        data->state_of_operation = (uint8_t) strtoul(value_str, NULL, 10);
        break;
    case TRACKER_OPERATION_MODE:
        data->tracker_operation_mode = (uint8_t) strtoul(value_str, NULL, 10);
        break;
    case OFF_REASON:
        data->off_reason = (uint32_t) strtoul(value_str, NULL, 0);
        break;
    case ERROR_CODE:
        data->error_code = (uint8_t) strtoul(value_str, NULL, 10);
        break;
    case LOAD_OUTPUT_STATE:
        strncpy(data->load_output_state, value_str, sizeof(data->load_output_state) - 1);
        data->load_output_state[sizeof(data->load_output_state) - 1] = '\0';
        break;
    case LOAD_CURRENT:
        data->load_current = (int32_t) strtol(value_str, NULL, 10);
        break;
    case YIELD_TOTAL:
        data->yield_total = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case YIELD_TODAY:
        data->yield_today = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case MAXIMUM_POWER_TODAY:
        data->maximum_power_today = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case YIELD_YESTERDAY:
        data->yield_yesterday = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case MAXIMUM_POWER_YESTERDAY:
        data->maximum_power_yesterday = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case DAY_SEQUENCE_NUMBER:
        data->day_sequence_number = (uint32_t) strtoul(value_str, NULL, 10);
        break;
    case CHECKSUM:
        data->checksum = (uint8_t) value_str[0];
        break;
    case UNKNOWN:
        ESP_LOGI(TAG, "Unknown field: %s", field_str);
        break;
    }

    return ESP_OK;
}

esp_err_t victron_mppt_parse_text(victron_mppt_uart_packet_handle_t uart_packet, victron_mppt_data_handle_t data)
{
    CHECK_ARG(uart_packet && uart_packet->buffer && uart_packet->length > 0 && data);

    // Verify data integrity
    CHECK(victron_mppt_check_text_checksum(uart_packet));

    char field[9];
    char value[33];

    char *buffer, *token, *subtoken;

    buffer = (char *) uart_packet->buffer;

    // NULL terminate since we use string functions
    buffer[uart_packet->length] = '\0';

    while((token = strsep(&buffer, "\r")) != NULL) {
        if (*token == '\0') continue;
        while((subtoken = strsep(&token, "\t")) != NULL) {
            if (*subtoken == '\n') {
                subtoken++;
                strncpy(field, subtoken, sizeof(field) - 1);
                field[sizeof(field) - 1] = '\0';
            } else {
                strncpy(value, subtoken, sizeof(value) - 1);
                value[sizeof(value) - 1] = '\0';
            }
        }
        victron_mppt_set_value_from_str(field, value, data);
    }

    return ESP_OK;
}

esp_err_t victron_mppt_print_data(victron_mppt_data_handle_t data)
{
    printf("PID 0x%hx\nFirmware version %hu\nSerial number %s\nBattery voltage %ldmV\nBattery current %ldmA\nSolar voltage %ldmV\nSolar power %ldW\nState of operation %hhu\nTracker operation mode %hhu\nOff reason 0x%lx\nError code %hhu\nLoad output state %s\nLoad current %ldmA\nYield total %lukWh(0.01)\nYield today %lukWh(0.01)\nMaximum power today %luW\nYield yesterday %lukWh(0.01)\nMaximum power yesterday %luW\nDay sequence number %lu\n",
        data->product_id,
        data->firmware_version,
        data->serial_number,
        data->battery_voltage,
        data->battery_current,
        data->solar_voltage,
        data->solar_power,
        data->state_of_operation,
        data->tracker_operation_mode,
        data->off_reason,
        data->error_code,
        data->load_output_state,
        data->load_current,
        data->yield_total,
        data->yield_today,
        data->maximum_power_today,
        data->yield_yesterday,
        data->maximum_power_yesterday,
        data->day_sequence_number
    );

    return ESP_OK;
}

esp_err_t victron_mppt_listen_uart(victron_mppt_handle_t victron_mppt)
{
    CHECK_ARG(victron_mppt && victron_mppt->uart_rx.buffer);

    uart_event_t uart_event;

    if (xQueueReceive(victron_mppt->uart_event_queue, (void *)&uart_event, (TickType_t)portMAX_DELAY)) {
        switch (uart_event.type) {
        case UART_DATA:
            CHECK(uart_get_buffered_data_len(victron_mppt->uart_port, &victron_mppt->uart_rx.length));
            if (victron_mppt->uart_rx.length > 0) {
                victron_mppt->uart_rx.length = uart_read_bytes(victron_mppt->uart_port, victron_mppt->uart_rx.buffer, VICTRON_MPPT_RX_BUFFER_SIZE, 150 / portTICK_PERIOD_MS);
                uart_flush(victron_mppt->uart_port);
                ESP_LOGI(TAG, "Received data:");
                ESP_LOG_BUFFER_HEXDUMP(TAG, victron_mppt->uart_rx.buffer, victron_mppt->uart_rx.length, ESP_LOG_INFO);
            }
            break;
        case UART_FIFO_OVF:
        ESP_LOGE(TAG, "FIFO overflow");
            uart_flush_input(victron_mppt->uart_port);
            xQueueReset(victron_mppt->uart_event_queue);
            break;
        case UART_BUFFER_FULL:
            ESP_LOGI(TAG, "Ring buffer full");
            uart_flush_input(victron_mppt->uart_port);
            xQueueReset(victron_mppt->uart_event_queue);
            break;
        case UART_BREAK:
            ESP_LOGI(TAG, "UART RX break");
            break;
        case UART_PARITY_ERR:
            ESP_LOGI(TAG, "UART parity error");
            break;
        case UART_FRAME_ERR:
            ESP_LOGI(TAG, "UART frame error");
            break;
        default:
            ESP_LOGI(TAG, "UART event type: %d", uart_event.type);
            break;
        }
    }

    return ESP_OK;
}
