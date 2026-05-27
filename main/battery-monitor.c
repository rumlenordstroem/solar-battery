#include <stdint.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "esp_bit_defs.h"
#include "freertos/FreeRTOS.h"

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

void setup_gpio(void)
{
    gpio_set_level(DISPLAY_REGISTER_SELECT_PIN, 0);
    gpio_config_t display_register_select = {
        .pin_bit_mask = BIT64(DISPLAY_REGISTER_SELECT_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_register_select);

    gpio_set_level(DISPLAY_READ_WRITE_SELECT_PIN, 0);
    gpio_config_t display_read_write_select_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_READ_WRITE_SELECT_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_read_write_select_cfg);

    gpio_set_level(DISPLAY_ENABLE_PIN, 0);
    gpio_config_t display_enable_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_ENABLE_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_enable_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_0_PIN, 0);
    gpio_config_t display_data_bit_0_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_0_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_0_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_1_PIN, 0);
    gpio_config_t display_data_bit_1_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_1_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_1_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_2_PIN, 0);
    gpio_config_t display_data_bit_2_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_2_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_2_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_3_PIN, 0);
    gpio_config_t display_data_bit_3_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_3_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_3_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_4_PIN, 0);
    gpio_config_t display_data_bit_4_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_4_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_4_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_5_PIN, 0);
    gpio_config_t display_data_bit_5_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_5_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_5_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_6_PIN, 0);
    gpio_config_t display_data_bit_6_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_6_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_6_cfg);

    gpio_set_level(DISPLAY_DATA_BIT_7_PIN, 0);
    gpio_config_t display_data_bit_7_cfg = {
        .pin_bit_mask = BIT64(DISPLAY_DATA_BIT_7_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&display_data_bit_7_cfg);
}

void clear_display(void)
{
  
}

void display_send_command(uint8_t cmd)
{
  printf("Sending command 0x%x\n", cmd);

    gpio_set_level(DISPLAY_REGISTER_SELECT_PIN, 0);
    gpio_set_level(DISPLAY_READ_WRITE_SELECT_PIN, 0);

    gpio_set_level(DISPLAY_DATA_BIT_0_PIN, (cmd >> 0) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_1_PIN, (cmd >> 1) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_2_PIN, (cmd >> 2) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_3_PIN, (cmd >> 3) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_4_PIN, (cmd >> 4) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_5_PIN, (cmd >> 5) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_6_PIN, (cmd >> 6) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_7_PIN, (cmd >> 7) & 0x01);

    gpio_set_level(DISPLAY_ENABLE_PIN, 1);
    vTaskDelay(5 /portTICK_PERIOD_MS);
    gpio_set_level(DISPLAY_ENABLE_PIN, 0);
}

void display_send_data(uint8_t data)
{
  printf("Sending data 0x%x\n", data);
    gpio_set_level(DISPLAY_REGISTER_SELECT_PIN, 1);
    gpio_set_level(DISPLAY_READ_WRITE_SELECT_PIN, 0);

    gpio_set_level(DISPLAY_DATA_BIT_0_PIN, (data >> 0) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_1_PIN, (data >> 1) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_2_PIN, (data >> 2) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_3_PIN, (data >> 3) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_4_PIN, (data >> 4) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_5_PIN, (data >> 5) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_6_PIN, (data >> 6) & 0x01);
    gpio_set_level(DISPLAY_DATA_BIT_7_PIN, (data >> 7) & 0x01);

    gpio_set_level(DISPLAY_ENABLE_PIN, 1);
    vTaskDelay(40 /portTICK_PERIOD_MS);
    gpio_set_level(DISPLAY_ENABLE_PIN, 0);
}

void display_send_string(char *str)
{
  
  size_t idx = 0;
  while(*(str + idx) != '\0') {
    display_send_data(*(str + idx));
    idx++;
  }
}

void app_main(void)
{
  setup_gpio();

  // Wait at least 40ms after powerup
  vTaskDelay(100 /portTICK_PERIOD_MS);

  // Three wake up commands
  display_send_command(0b00110000);
  vTaskDelay(30 /portTICK_PERIOD_MS);
  display_send_command(0b00110000);
  vTaskDelay(10 /portTICK_PERIOD_MS);
  display_send_command(0b00110000);
  vTaskDelay(10 /portTICK_PERIOD_MS);

  display_send_command(0b00001100);
  vTaskDelay(40 /portTICK_PERIOD_MS);
  display_send_command(0b00000001);
  display_send_command(0b00000110);
  vTaskDelay(500 /portTICK_PERIOD_MS);
  display_send_string("Rumle er sej!");
}
