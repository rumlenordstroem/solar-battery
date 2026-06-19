#include <stdint.h>
#include <stdio.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "hal/uart_types.h"
#include "soc/gpio_num.h"
#include "esp_log.h"

#include "victron_mppt.h"

static const char *TAG = "victron_mppt";

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t victron_mppt_init(victron_mppt_handle_t handle, uart_port_t uart_port, gpio_num_t rx_gpio_num, gpio_num_t tx_gpio_num)
{
    CHECK_ARG(handle);

    const uart_config_t uart_config = {
        .baud_rate = VICTRON_MPPT_BAUD_RATE,
        .data_bits = VICTRON_MPPT_DATA_BITS,
        .parity = VICTRON_MPPT_PARITY,
        .stop_bits = VICTRON_MPPT_STOP_BITS,
        .flow_ctrl = VICTRON_MPPT_FLOWCTRL,
    };

    handle->uart_port = uart_port;
    handle->uart_rx_buffer = (uint8_t *) malloc(VICTRON_MPPT_RX_BUFFER_SIZE);
    if (!handle->uart_rx_buffer)
        return ESP_ERR_NO_MEM;

    CHECK(uart_driver_install(handle->uart_port, VICTRON_MPPT_RX_BUFFER_SIZE, 0, VICTRON_MPPT_UART_QUEUE_SIZE, &handle->uart_event_queue, 0));
    CHECK(uart_param_config(handle->uart_port, &uart_config));
    CHECK(uart_set_pin(handle->uart_port, tx_gpio_num, rx_gpio_num, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    return ESP_OK;
}

esp_err_t victron_mppt_free(victron_mppt_handle_t handle)
{
    CHECK_ARG(handle && handle->uart_rx_buffer);

    free(handle->uart_rx_buffer);
    handle->uart_rx_buffer = NULL;
    return ESP_OK;
}

esp_err_t victron_mppt_check_text_checksum(victron_mppt_handle_t handle)
{
  CHECK_ARG(handle && handle->uart_rx_buffer);

  int checksum = 0;
  for (int i = 0; i < handle->uart_rx_data_length; i++) {
     checksum = (checksum + handle->uart_rx_buffer[i]) & 255; /* Take modulo 256 in account */
  }
 
  if (checksum == 0) {
    return ESP_OK;
  }

  return ESP_ERR_INVALID_CRC;
}

esp_err_t victron_mppt_read_data(victron_mppt_handle_t handle)
{
    CHECK_ARG(handle && handle->uart_rx_buffer);

    CHECK(uart_get_buffered_data_len(handle->uart_port, &handle->uart_rx_data_length));

    if (handle->uart_rx_data_length > 0) {
      // Clear RX buffer
      memset(handle->uart_rx_buffer, 0, VICTRON_MPPT_RX_BUFFER_SIZE);

      handle->uart_rx_data_length = uart_read_bytes(handle->uart_port, handle->uart_rx_buffer, handle->uart_rx_data_length, 100 / portTICK_PERIOD_MS);

      ESP_LOGI(TAG, "UART RX buffer has %d bytes.", handle->uart_rx_data_length);
      ESP_LOG_BUFFER_HEXDUMP(TAG, handle->uart_rx_buffer, handle->uart_rx_data_length, ESP_LOG_INFO);

      // Clear buffer
      uart_flush(handle->uart_port);
    } else {
      ESP_LOGI(TAG, "UART RX buffer is empty. Nothing was read.");
    }

    return ESP_OK;
}

victron_mppt_field_t victron_mppt_get_field_from_str(const char *field)
{
  CHECK_ARG(field);

  if (strcmp("PID", field) == 0) return PRODUCT_ID;
  if (strcmp("FW", field) == 0) return FIRMWARE_VERSION;
  if (strcmp("SER#", field) == 0) return SERIAL_NUMBER;
  if (strcmp("V", field) == 0) return BATTERY_VOLTAGE;
  if (strcmp("I", field) == 0) return BATTERY_CURRENT;
  if (strcmp("VPV", field) == 0) return SOLAR_VOLTAGE;
  if (strcmp("PPV", field) == 0) return SOLAR_POWER;
  if (strcmp("CS", field) == 0) return STATE_OF_OPERATION;
  if (strcmp("MPPT", field) == 0) return TRACKER_OPERATION_MODE;
  if (strcmp("OR", field) == 0) return OFF_REASON;
  if (strcmp("ERR", field) == 0) return ERROR_CODE;
  if (strcmp("LOAD", field) == 0) return LOAD_OUTPUT_STATE;
  if (strcmp("IL", field) == 0) return LOAD_CURRENT;
  if (strcmp("H19", field) == 0) return YIELD_TOTAL;
  if (strcmp("H20", field) == 0) return YIELD_TODAY;
  if (strcmp("H21", field) == 0) return MAXIMUM_POWER_TODAY;
  if (strcmp("H22", field) == 0) return YIELD_YESTERDAY;
  if (strcmp("H23", field) == 0) return MAXIMUM_POWER_YESTERDAY;
  if (strcmp("HSDS", field) == 0) return DAY_SEQUENCE_NUMBER;
  if (strcmp("Checksum", field) == 0) return CHECKSUM;

  return UNKNOWN;
}

esp_err_t victron_mppt_set_value_from_str(const char *field, const char *value, victron_mppt_data_t *data)
{
  CHECK_ARG(field && value && data);

  switch (victron_mppt_get_field_from_str(field)) {
    case PRODUCT_ID:
      data->product_id = (uint16_t) strtoul(value, NULL, 0);
      break;
    case FIRMWARE_VERSION:
      data->firmware_version = (uint16_t) strtoul(value, NULL, 10);
      break;
    case SERIAL_NUMBER:
      strncpy(data->serial_number, value, sizeof(data->serial_number));
      break;
    case BATTERY_VOLTAGE:
      data->battery_voltage = (int32_t) strtol(value, NULL, 10);
      break;
    case BATTERY_CURRENT:
      data->battery_current = (int32_t) strtol(value, NULL, 10);
      break;
    case SOLAR_VOLTAGE:
      data->solar_voltage = (int32_t) strtol(value, NULL, 10);
      break;
    case SOLAR_POWER:
      data->solar_power = (int32_t) strtol(value, NULL, 10);
      break;
    case STATE_OF_OPERATION:
      data->state_of_operation = (uint8_t) strtoul(value, NULL, 10);
      break;
    case TRACKER_OPERATION_MODE:
      data->tracker_operation_mode = (uint8_t) strtoul(value, NULL, 10);
      break;
    case OFF_REASON:
      data->off_reason = (uint32_t) strtoul(value, NULL, 0);
      break;
    case ERROR_CODE:
      data->error_code = (uint8_t) strtoul(value, NULL, 10);
      break;
    case LOAD_OUTPUT_STATE:
      strncpy(data->load_output_state, value, sizeof(data->load_output_state));
      break;
    case LOAD_CURRENT:
      data->load_current = (int32_t) strtol(value, NULL, 10);
      break;
    case YIELD_TOTAL:
      data->yield_total = (uint32_t) strtoul(value, NULL, 10);
      break;
    case YIELD_TODAY:
      data->yield_today = (uint32_t) strtoul(value, NULL, 10);
      break;
    case MAXIMUM_POWER_TODAY:
      data->maximum_power_today = (uint32_t) strtoul(value, NULL, 10);
      break;
    case YIELD_YESTERDAY:
      data->yield_yesterday = (uint32_t) strtoul(value, NULL, 10);
      break;
    case MAXIMUM_POWER_YESTERDAY:
      data->maximum_power_yesterday = (uint32_t) strtoul(value, NULL, 10);
      break;
    case DAY_SEQUENCE_NUMBER:
      data->day_sequence_number = (uint32_t) strtoul(value, NULL, 10);
      break;
    case CHECKSUM:
      data->checksum = (uint8_t) value[0];
      break;
    case UNKNOWN:
      ESP_LOGI(TAG, "Unknown field: %s", field);
      break;
  }

  return ESP_OK;
}

esp_err_t victron_mppt_parse_text(victron_mppt_handle_t handle, victron_mppt_data_t *data)
{
  CHECK_ARG(handle && handle->uart_rx_buffer && data);

  char field[9];
  char value[33];

  char *token, *subtoken;

  // Nothing to parse
  if (handle->uart_rx_data_length == 0) return ESP_OK;

  CHECK(victron_mppt_check_text_checksum(handle));

  char *buffer = (char *) malloc(handle->uart_rx_data_length + 1);
  if (!buffer)
      return ESP_ERR_NO_MEM;

  // Keep track of the original pointer for freeing later
  char *orig_buffer_ptr = buffer;

  memcpy(buffer, handle->uart_rx_buffer, handle->uart_rx_data_length);

  // NULL terminate since we use string functions
  buffer[handle->uart_rx_data_length] = '\0';

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

  free(orig_buffer_ptr);

  return ESP_OK;
}

void victron_mppt_print_data(victron_mppt_data_t *data)
{
  printf("PID 0x%hx\nFirmware version %hu\nSerial number %s\nBattery voltage %ldmV\nBattery current %ldmA\nSolar voltage %ldmV\nSolar power %ldW\nState of operation %hhu\nTracker operation mode %hhu\nOff reason 0x%lx\nError code %hhu\nLoad output state %s\nLoad current %ldmA\nYield total %lukWh(0.01)\nYield today %lukWh(0.01)\nMaximum power today %luW\nYield yesterday %lukWh(0.01)\nMaximum power yesterday %lukWh(0.01)\nDay sequence number %lu\n",
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
}

esp_err_t victron_mppt_listen_uart(victron_mppt_handle_t handle)
{
    CHECK_ARG(handle && handle->uart_rx_buffer);

    uart_event_t uart_event;

    if (xQueueReceive(handle->uart_event_queue, (void *)&uart_event, (TickType_t)portMAX_DELAY)) {
        switch (uart_event.type) {
            case UART_DATA:
                CHECK(uart_get_buffered_data_len(handle->uart_port, &handle->uart_rx_data_length));
                if (handle->uart_rx_data_length > 0) {
                  handle->uart_rx_data_length = uart_read_bytes(handle->uart_port, handle->uart_rx_buffer, VICTRON_MPPT_RX_BUFFER_SIZE, 50 / portTICK_PERIOD_MS);
                  uart_flush(handle->uart_port);
                  ESP_LOGI(TAG, "Received data:");
                  ESP_LOG_BUFFER_HEXDUMP(TAG, handle->uart_rx_buffer, handle->uart_rx_data_length, ESP_LOG_INFO);
                }
                break;
            case UART_FIFO_OVF:
                ESP_LOGE(TAG, "FIFO overflow");
                uart_flush_input(handle->uart_port);
                xQueueReset(handle->uart_event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "Ring buffer full");
                uart_flush_input(handle->uart_port);
                xQueueReset(handle->uart_event_queue);
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
