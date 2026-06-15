#pragma once

#include <stdint.h>
#include <inttypes.h>

#include "driver/uart.h"

#define VICTRON_MPPT_DATA_FORMAT "\n\rPID\t0x%hx\n\rFW\t%hu\n\rSER#\t%s\n\rV\t%ld\n\rI\t%ld\n\rVPV\t%ld\n\rPPV\t%ld\n\rCS\t%hhu\n\rMPPT\t%hhu\n\rOR\t0x%lx\n\rERR\t%hhu\n\rLOAD\t%s\n\rIL\t%ld\n\rH19\t%ld\n\rH20\t%ld\n\rH21\t%ld\n\rH22\t%ld\n\rH23\t%ld\n\rHSDS\t%ld\n\rChecksum\t"
#define VICTRON_MPPT_DATA_EXAMPLE "\n\rPID\t0xA075\n\rFW\t174\n\rSER#\tHQ25404JDUQ\n\rV\t12920\n\rI\t-10\n\rVPV\t30\n\rPPV\t0\n\rCS\t0\n\rMPPT\t0\n\rOR\t0x00000001\n\rERR\t0\n\rLOAD\tON\n\rIL\t0\n\rH19\t3\n\rH20\t0\n\rH21\t0\n\rH22\t0\n\rH23\t2\n\rHSDS\t6\n\rChecksum\t"

#define VICTRON_MPPT_BAUD_RATE 19200
#define VICTRON_MPPT_DATA_BITS UART_DATA_8_BITS
#define VICTRON_MPPT_PARITY UART_PARITY_DISABLE
#define VICTRON_MPPT_STOP_BITS UART_STOP_BITS_1
#define VICTRON_MPPT_FLOWCTRL UART_HW_FLOWCTRL_DISABLE

extern const uart_config_t victron_mppt_uart_config;

typedef struct {
    uint16_t pid;
    uint16_t firmware_version;
    char serial_num[12];
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

int victron_mppt_parse_text(char *text, victron_mppt_data_t *data);
int victron_mppt_check_text_checksum(char *text, size_t length);
void victron_mppt_print_data(victron_mppt_data_t *data);
