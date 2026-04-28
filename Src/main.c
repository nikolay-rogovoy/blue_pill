#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

static void MX_GPIO_Init(void);
USBD_HandleTypeDef USBD_Device;
TIM_HandleTypeDef htim4;

void MX_TIM4_Init(void)
{
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 7200 - 1; /* 72 MHz / 7200 = 10 kHz */
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 10 - 1; /* 10 kHz / 10 = 1 kHz */
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim4);
    HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base)
{
    if (htim_base->Instance == TIM4)
    {
        __HAL_RCC_TIM4_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM4_IRQn, 15, 0); /* Низший приоритет */
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
}

void vLEDTask(void *pvParameters)
{
    (void)pvParameters; // Подавляем warning о неиспользуемом параметре

    /* Бесконечный цикл задачи */
    for (;;)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);

        /* Задержка в миллисекундах (FreeRTOS) */
        vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_INTERVAL_MS));
    }
}

void MX_USB_DEVICE_Init()
{
    /* Инициализация USB */
    if (USBD_Init(&USBD_Device, &VCP_Desc, 0) != USBD_OK)
    {
        Error_Handler();
    }
    if (USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS) != USBD_OK)
    {
        Error_Handler();
    }
    if (USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops) != USBD_OK)
    {
        Error_Handler();
    }
    if (USBD_Start(&USBD_Device) != USBD_OK)
    {
        Error_Handler();
    }
}

void vUSBTask(void *pvParameters)
{
    /* Ждём, пока система стабилизируется */
    vTaskDelay(pdMS_TO_TICKS(500));

    vTaskSuspendAll();

    MX_USB_DEVICE_Init();

    xTaskResumeAll();

    vTaskDelay(pdMS_TO_TICKS(500));

    /* USB инициализирован, задача больше не нужна */
    vTaskDelete(NULL);
}

int main(void)
{
    HAL_Init();
    // HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    SystemClock_Config();
    MX_GPIO_Init();

    MX_USB_DEVICE_Init();

    // HAL_Delay(100);

    MX_TIM4_Init();
    HAL_TIM_Base_Start_IT(&htim4);

    // /* Init Device Library */
    // USBD_Init(&USBD_Device, &VCP_Desc, 0);

    // // /* Add Supported Class */
    // USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);

    // // /* Add CDC Interface Class */
    // USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);

    // // /* Start Device Process */
    // USBD_Start(&USBD_Device);

    /* Проверка: мигаем LED, чтобы убедиться, что HAL работает */
    for (int i = 0; i < 3; i++)
    {
        HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
        HAL_Delay(100);
    }

    // xTaskCreate(vUSBTask, "USB", 512, NULL, 1, NULL); /* Низкий приоритет */

    xTaskCreate(
        vLEDTask, /* Функция задачи */
        "LED",    /* Имя задачи (для отладки) */
        128,      /* Размер стека в словах (не байтах!) */
        NULL,     /* Параметр задачи */
        1,        /* Приоритет (1 = низкий) */
        NULL      /* Хэндл задачи (не нужен) */
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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

void _init(void)
{
    /* Пустая функция для C++ инициализации */
}
