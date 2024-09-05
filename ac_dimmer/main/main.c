#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#define INPUT_PIN GPIO_NUM_22
#define OUTPUT_PIN GPIO_NUM_2

bool button_state = false;
bool led_state = false;
static void IRAM_ATTR gpio_interrupt_handle(void *arg)
{
    button_state = true;

}

void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(OUTPUT_PIN);
    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);

    esp_rom_gpio_pad_select_gpio(INPUT_PIN);
    gpio_set_direction(INPUT_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(INPUT_PIN);
    gpio_pullup_dis(INPUT_PIN);
    gpio_set_intr_type(INPUT_PIN, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INPUT_PIN, gpio_interrupt_handle, NULL);

    while(1)
    {
        if(button_state == true)
        {
            led_state = !led_state;
            gpio_set_level(OUTPUT_PIN, led_state);
            button_state = false;
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}