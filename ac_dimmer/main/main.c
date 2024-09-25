#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_timer.h"

#define INPUT_PIN GPIO_NUM_22
#define OUTPUT_PIN GPIO_NUM_18
#define EX_UART_NUM UART_NUM_0
#define BUF_SIZE (1024)

volatile bool zero_crossing_flag = false;
volatile int cnt = 0;
uint32_t dim = 0;

static void IRAM_ATTR zeroCrossing(void *arg)
{
    zero_crossing_flag = true;
    cnt = 0;
    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_PIN, 0);
}

void dimmer(void *param)
{
    if (zero_crossing_flag == true)
    {
        if (cnt > dim)
        {
            gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
            gpio_set_level(OUTPUT_PIN, 1);
            zero_crossing_flag = false;
            cnt = 0;
        }
        cnt++;
    }
}

static void uart_event_task(void *parameters)
{
    uint8_t data[BUF_SIZE];
    while(1)
    {
        int len = uart_read_bytes(EX_UART_NUM, data, BUF_SIZE, 1000/portTICK_PERIOD_MS);
        uart_write_bytes(EX_UART_NUM, (const char *) data, len);
        if (len > 0)
        {
            data[len] = '\0';
            dim = atoi((const char*) data);
            if (dim < 0)
            {
                dim = 0;
            }
            else if (dim > 100)
            {
                dim = 100;
            }
            dim = 100 - dim;
            printf("\n");
        }
    }
}

void app_main(void)
{
    esp_rom_gpio_pad_select_gpio(OUTPUT_PIN);
    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);

    // gpio interrupt
    esp_rom_gpio_pad_select_gpio(INPUT_PIN);
    gpio_set_direction(INPUT_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(INPUT_PIN);
    gpio_pullup_dis(INPUT_PIN);
    gpio_set_intr_type(INPUT_PIN, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INPUT_PIN, zeroCrossing, NULL);

    // timer interrupt
    const esp_timer_create_args_t my_timer_arg = {
        .callback = &dimmer,
        .name = "timer interrupt"
    };
    esp_timer_handle_t timer_handler;
    esp_timer_create(&my_timer_arg, &timer_handler);
    esp_timer_start_periodic(timer_handler, 100);
    // uart
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    xTaskCreate(uart_event_task, "uart_event_task", 2024, NULL, 10, NULL);

}