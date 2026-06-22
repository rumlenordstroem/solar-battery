#include "state_of_charge.h"

#include <stdbool.h>
#include <stdint.h>
#include <sys/_timeval.h>
#include <sys/time.h>

#include "esp_err.h"

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

esp_err_t state_of_charge_update_percentage(state_of_charge_handle_t state_of_charge)
{
    CHECK_ARG(state_of_charge);

    state_of_charge->percentage = (int32_t)(100L * (state_of_charge->charge_mc / 1000L)) / (state_of_charge->capacity_mc / 1000L);

    return ESP_OK;
}

esp_err_t state_of_charge_print(state_of_charge_handle_t state_of_charge)
{
    CHECK_ARG(state_of_charge);

    printf("Charge %lld mC\nCapacity %lld\nPercentage %ld %%\n", state_of_charge->charge_mc, state_of_charge->capacity_mc, state_of_charge->percentage);

    return ESP_OK;
}

esp_err_t state_of_charge_init(state_of_charge_handle_t state_of_charge, uint64_t charge_mc, uint64_t capacity_mc)
{
    CHECK_ARG(state_of_charge);

    state_of_charge->charge_mc = charge_mc;
    CHECK(gettimeofday(&state_of_charge->timestamp, NULL));
    state_of_charge->capacity_mc = capacity_mc;
    CHECK(state_of_charge_update_percentage(state_of_charge));

    return ESP_OK;
}

esp_err_t state_of_charge_coulomb_count_update(state_of_charge_handle_t state_of_charge, int64_t current_ma)
{
    CHECK_ARG(state_of_charge);

    struct timeval timestamp, delta_time;

    // Temporarily store current time in delta time
    CHECK(gettimeofday(&timestamp, NULL));

    // Calculate delta time
    timersub(&timestamp, &state_of_charge->timestamp, &delta_time);
    int64_t delta_time_ms = ((int64_t)delta_time.tv_sec * 1000L + ((int64_t)delta_time.tv_usec / 1000L));
    printf("delta time %lld\n", delta_time_ms);
    state_of_charge_print(state_of_charge);

    // Update state of charge (TODO check underflow)
    state_of_charge->charge_mc = state_of_charge->charge_mc + ((current_ma * delta_time_ms) / 1000L);
    CHECK(state_of_charge_update_percentage(state_of_charge));

    // Set new timestamp
    state_of_charge->timestamp.tv_sec = timestamp.tv_sec;
    state_of_charge->timestamp.tv_usec = timestamp.tv_usec;

    return ESP_OK;
}

esp_err_t state_of_charge_calibrate_capacity_at_full(state_of_charge_handle_t state_of_charge)
{
    CHECK_ARG(state_of_charge);

    // Set the capacity to the current charge
    state_of_charge->capacity_mc = state_of_charge->charge_mc;

    CHECK(state_of_charge_update_percentage(state_of_charge));

    return ESP_OK;
}

esp_err_t state_of_charge_calibrate_charge_at_empty(state_of_charge_handle_t state_of_charge)
{
    CHECK_ARG(state_of_charge);

    state_of_charge->charge_mc = 0L;

    CHECK(state_of_charge_update_percentage(state_of_charge));

    return ESP_OK;
}
