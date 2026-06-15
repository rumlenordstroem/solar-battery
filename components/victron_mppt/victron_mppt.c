#include "victron_mppt.h"
#include <stdio.h>

const uart_config_t victron_mppt_uart_config = {
    .baud_rate = VICTRON_MPPT_BAUD_RATE,
    .data_bits = VICTRON_MPPT_DATA_BITS,
    .parity = VICTRON_MPPT_PARITY,
    .stop_bits = VICTRON_MPPT_STOP_BITS,
    .flow_ctrl = VICTRON_MPPT_FLOWCTRL,
};

int victron_mppt_parse_text(char *text, victron_mppt_data_t *data)
{
  return sscanf(text,
         VICTRON_MPPT_DATA_FORMAT,
         &data->pid,
         &data->firmware_version,
         &data->serial_num[0],
         &data->battery_voltage,
         &data->battery_current,
         &data->solar_voltage,
         &data->solar_power,
         &data->state_of_operation,
         &data->tracker_operation_mode,
         &data->off_reason,
         &data->error_code,
         &data->load_output_state[0],
         &data->load_current,
         &data->yield_total,
         &data->yield_today,
         &data->maximum_power_today,
         &data->yield_yesterday,
         &data->maximum_power_yesterday,
         &data->day_sequence_number
      );
}

int victron_mppt_check_text_checksum(char *text, size_t length)
{
  int checksum = 0;
  for (int i = 0; i < length; i++) {
     checksum = (checksum + text[i]) & 255; /* Take modulo 256 in account */
  }
 
  if (checksum == 0) {
    return 0;
  } else {
    return 1;
  }
}

void victron_mppt_print_data(victron_mppt_data_t *data)
{
  printf("PID 0x%hx\nFirmware version %hu\nSerial number %s\nBattery voltage %ldmV\nBattery current %ldmA\nSolar voltage %ldmV\nSolar power %ldW\nState of operation %hhu\nTracker operation mode %hhu\nOff reason 0x%lx\nError code %hhu\nLoad output state %s\nLoad current %ldmA\nYield total %lukWh(0.01)\nYield today %lukWh(0.01)\nMaximum power today %luW\nYield yesterday %lukWh(0.01)\nMaximum power yesterday %lukWh(0.01)\nDay sequence number %lu\n",
       data->pid,
       data->firmware_version,
       data->serial_num,
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
