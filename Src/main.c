#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "stm32f1xx_hal_tim.h"

#include "FreeRTOS.h"
#include "task.h"

static void MX_GPIO_Init(void);
USBD_HandleTypeDef USBD_Device;
TIM_HandleTypeDef htim4;

// void MX_TIM4_Init(void)
// {
//     htim4.Instance = TIM4;
//     htim4.Init.Prescaler = 7200 - 1; /* 72 MHz / 7200 = 10 kHz */
//     htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
//     htim4.Init.Period = 10 - 1; /* 10 kHz / 10 = 1 kHz */
//     htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
//     HAL_TIM_Base_Init(&htim4);
//     HAL_NVIC_SetPriority(TIM4_IRQn, 0, 0);
// }

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

TaskHandle_t xHandleUSBTask = NULL;
TaskHandle_t xLEDTaskHandle = NULL;

void USBTask(void *pvParameters)
{
    /* Сохраняем хендл текущей задачи в глобальную переменную */
    xHandleUSBTask = xTaskGetCurrentTaskHandle();

    char line_buffer[256] = {0}; // Буфер для накопления строки
    uint16_t pos = 0;            // Текущая позиция в буфере

    memset(line_buffer, 0, sizeof(line_buffer));

    for (;;)
    {
        /* Ожидаем уведомление от USB прерывания */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // taskENTER_CRITICAL();
        /* Обрабатываем полученные данные */
        while (rxHead != rxTail)
        {
            uint8_t c = rxBuffer[rxTail++];
            if (rxTail >= RX_BUFFER_SIZE)
                rxTail = 0;

            if (c == '\r' || c == '\n')
            {
                // Нажат Enter — обрабатываем строку
                if (pos > 0)
                {
                    line_buffer[pos] = '\0'; // Завершаем строку

                    // ==== ЗДЕСЬ ОБРАБАТЫВАЕМ КОМАНДУ ====
                    // Пример: проверяем команду "LED_ON" или "LED_OFF"
                    if (strcmp(line_buffer, "LED_ON") == 0)
                    {
                        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET); // Включаем LED
                        vTaskResume(xLEDTaskHandle);
                        CDC_Transmit_Async((uint8_t *)"\r\nLED ON\r\n", 11);
                    }
                    else if (strcmp(line_buffer, "LED_OFF") == 0)
                    {
                        HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET); // Выключаем LED
                        vTaskSuspend(xLEDTaskHandle);
                        CDC_Transmit_Async((uint8_t *)"\r\nLED OFF\r\n", 12);
                    }
                    else if (strcmp(line_buffer, "STATUS") == 0)
                    {
                        char status[64];
                        sprintf(status, "\r\nUptime: %lu ms\r\n", HAL_GetTick());
                        CDC_Transmit_Async((uint8_t *)status, strlen(status));
                    }
                    else if (strcmp(line_buffer, "HELP") == 0)
                    {
                        CDC_Transmit_Async((uint8_t *)"\r\n", 2);
                        CDC_Transmit_Async((uint8_t *)"Commands: LED_ON, LED_OFF, STATUS, HELP\r\n", 41);
                    }
                    else
                    {
                        CDC_Transmit_Async((uint8_t *)"\r\n", 2);
                        CDC_Transmit_Async((uint8_t *)"Uncnown command\r\n", 20);
                        CDC_Transmit_Async((uint8_t *)line_buffer, pos);
                        CDC_Transmit_Async((uint8_t *)"\r\n", 2);
                    }
                    // =================================

                    // Сбрасываем буфер
                    pos = 0;
                    memset(line_buffer, 0, sizeof(line_buffer));
                }
            }
            else if (c == '\b' || c == 0x7F) // Backspace или Delete
            {
                // Удаляем последний символ
                if (pos > 0)
                {
                    pos--;
                    line_buffer[pos] = '\0';
                    // Отправляем backspace для терминала
                    CDC_Transmit_Async((uint8_t *)"\b \b", 3); // Затираем символ
                }
            }
            else if (c >= 0x20 && c <= 0x7E) // Печатные символы (ASCII)
            {
                // Добавляем символ в буфер, если есть место
                if (pos < sizeof(line_buffer) - 1)
                {
                    line_buffer[pos++] = c;
                    // Отправляем эхо обратно (видим то, что печатаем)
                    CDC_Transmit_Async(&c, 1);
                }
                else
                {
                    // Буфер переполнен
                    CDC_Transmit_Async((uint8_t *)"\r\nBuffer overflow!\r\n", 22);
                    pos = 0;
                    memset(line_buffer, 0, sizeof(line_buffer));
                }
            }
        }
        // taskEXIT_CRITICAL();
    }
}

int main(void)
{
    HAL_Init();
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    SystemClock_Config();
    MX_GPIO_Init();

    MX_USB_DEVICE_Init();

    xTaskCreate(
        USBTask,        // Функция задачи
        "USB Task",     // Имя задачи
        256,            // Размер стека (слов, не байт!)
        NULL,           // Параметр
        2,              // Приоритет (выше LED, но не слишком высокий)
        &xHandleUSBTask // Сохраняем хендл для уведомлений из прерывания
    );

    xTaskCreate(CDC_SendTask, "USBSend", 256, NULL, 2, NULL);

    xTaskCreate(
        vLEDTask,       /* Функция задачи */
        "LED",          /* Имя задачи (для отладки) */
        128,            /* Размер стека в словах (не байтах!) */
        NULL,           /* Параметр задачи */
        1,              /* Приоритет (1 = низкий) */
        &xLEDTaskHandle /* Хэндл задачи*/
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
