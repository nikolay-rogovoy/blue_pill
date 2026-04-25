#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

static void MX_GPIO_Init(void);

void vLEDTask(void *pvParameters)
{
    (void) pvParameters;  // Подавляем warning о неиспользуемом параметре
    
    /* Бесконечный цикл задачи */
    for(;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        
        /* Задержка в миллисекундах (FreeRTOS) */
        vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_INTERVAL_MS));
    }
}

int main(void)
{
    HAL_Init();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    SystemClock_Config();
    MX_GPIO_Init();

    xTaskCreate(
        vLEDTask,            /* Функция задачи */
        "LED",               /* Имя задачи (для отладки) */
        128, /* Размер стека в словах (не байтах!) */
        NULL,                /* Параметр задачи */
        1,   /* Приоритет (1 = низкий) */
        NULL                 /* Хэндл задачи (не нужен) */
    );

    /* 5. Запуск планировщика FreeRTOS */
    /* (Эта функция никогда не возвращает управление) */
    vTaskStartScheduler();

    // while (1)
    // {
    //     HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
    //     HAL_Delay(LED_TOGGLE_INTERVAL_MS);
    // }
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void Error_Handler(void)
{
    while (1)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

void _init(void)
{
    /* Пустая функция для C++ инициализации */
}
