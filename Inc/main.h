#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "usbd_core.h"
#include "stm32f1xx_hal_pcd.h"
#include "usbd_desc.h"
#include "usbd_cdc.h" 
#include "usbd_cdc_interface.h"
/* Определения пинов */
#define LED_PIN                     GPIO_PIN_13
#define LED_PORT                    GPIOC

/* Другие определения */
#define LED_TOGGLE_INTERVAL_MS      100

/* Прототипы функций */
void Error_Handler(void);
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */