#include <stdint.h>
#include <sys/time.h>

#include "esp_err.h"

#define AMPERE_HOURS_TO_MILLI_COULOMB(amp_hours) (amp_hours * 3600000)

typedef struct state_of_charge_t {
    int32_t charge_mc;
    int32_t capacity_mc;
    struct timeval timestamp;
    int32_t percentage;
} state_of_charge_t;

typedef state_of_charge_t *state_of_charge_handle_t;

esp_err_t state_of_charge_init(state_of_charge_handle_t state_of_charge, uint32_t charge_mc, uint32_t capacity_mc);
esp_err_t state_of_charge_coulomb_count_update(state_of_charge_handle_t state_of_charge, int32_t current_ma);
esp_err_t state_of_charge_calibrate_capacity_at_full(state_of_charge_handle_t state_of_charge);
esp_err_t state_of_charge_calibrate_charge_at_empty(state_of_charge_handle_t state_of_charge);
