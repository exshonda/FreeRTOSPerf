/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "FreeRTOS.h"
#include "task.h"

// Which core to run on if configNUMBER_OF_CORES==1
#ifndef RUN_FREE_RTOS_ON_CORE
#define RUN_FREE_RTOS_ON_CORE 0
#endif /* RUN_FREE_RTOS_ON_CORE */

/* Priorities of our threads - higher numbers are higher priority */
#define LOW_PRIORITY      ( tskIDLE_PRIORITY + 2UL )
#define MID_PRIORITY      ( tskIDLE_PRIORITY + 3UL )
#define HIGH_PRIORITY     ( tskIDLE_PRIORITY + 4UL )

/* Stack sizes of our threads in words (4 bytes) */
#define TASK_STACK_SIZE configMINIMAL_STACK_SIZE

/* Task Handle */
TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle;

/* Task Prototype */
void vTask1( void * pvParameters );
void vTask2( void * pvParameters );

/* Number of Measurement */
#define LOOP_CNT 20000

/*Othere Variable*/
int curloop_cnt = 0;

/* Measurement GPIO Pin */
#define M_GPIO_NO  2

static inline void
begin_measure(void)
{
    gpio_put(M_GPIO_NO, 1);
}

static inline void
end_measure(void)
{
    gpio_put(M_GPIO_NO, 0);
}

void
vTask1(void *pvParameters)
{
    while(true) {
        end_measure();
        if(++curloop_cnt >= LOOP_CNT) {
            printf("curloop_cnt:%d\n", curloop_cnt);
            while(true);
        }        
        xTaskNotifyGive(xTask2Handle);
    }
}

void
vTask2(void *pvParameters)
{
    curloop_cnt = 0;
    
    while(true) {
        begin_measure();
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
}


void
vLaunch(void) {
    /*GPIO Init*/
    gpio_init(M_GPIO_NO);
    gpio_set_dir(M_GPIO_NO, GPIO_OUT);
    
    /*Create Task*/
    if(xTaskCreate(vTask1, "Task_1", TASK_STACK_SIZE, NULL, LOW_PRIORITY, &xTask1Handle) != pdPASS) {
        printf("Task1 creation failed!\n");
        while(true);
    }
    if(xTaskCreate(vTask2, "Task_2", TASK_STACK_SIZE, NULL, MID_PRIORITY, &xTask2Handle) != pdPASS) {
        printf("Task2 creation failed!\n");
        while(true);
    }
    
#if configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    vTaskCoreAffinitySet(xTask1Handle, 1 << RUN_FREE_RTOS_ON_CORE);
    vTaskCoreAffinitySet(xTask2Handle, 1 << RUN_FREE_RTOS_ON_CORE);
#endif /* configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1 */

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}


int
main(void)
{
    const char *rtos_name;
    
    stdio_init_all();
   
#if (configNUMBER_OF_CORES > 1)
    rtos_name = "FreeRTOS SMP";
#else /* !(configNUMBER_OF_CORES > 1) */
    rtos_name = "FreeRTOS";
#endif /* (configNUMBER_OF_CORES > 1) */

#if (configNUMBER_OF_CORES > 1)
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif (RUN_FREE_RTOS_ON_CORE == 1 && configNUMBER_OF_CORES==1)
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else /* (RUN_FREE_RTOS_ON_CORE == 1 && configNUMBER_OF_CORES==1) */
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif /* (configNUMBER_OF_CORES > 1) */
    return 0;
}
