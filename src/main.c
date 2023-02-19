#include <stdio.h>
#include "py32f0xx_hal.h"
#include "debounce.h"

static debounce_pin_t user_button_pin_debounce = {
    .bank = GPIOA,
    .pin = GPIO_PIN_11,
};

static TIM_HandleTypeDef tim16_hndl = {
    .Instance = TIM16,
    .Init.Prescaler = 1000 - 1,
    .Init.CounterMode = TIM_COUNTERMODE_UP,
    .Init.Period = 3200 - 1,
    .Init.ClockDivision = TIM_CLOCKDIVISION_DIV1,
    .Init.RepetitionCounter = 1 - 1,
    .Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
};

static volatile int button_pressed = 0;

static void APP_SystemClockConfig(void);
static void APP_ErrorHandler(void);

// Interrupt Vector
void SysTick_Handler()
{
    HAL_IncTick();
}

// Interrupt Vector
void TIM16_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&tim16_hndl);
}

// timer callback
void timer16_period_elapsed(TIM_HandleTypeDef *handle)
{
    debounce(&user_button_pin_debounce);
    if (debounce_is_low(&user_button_pin_debounce)) {
        button_pressed = 1;
    } else {
        button_pressed = 0;
    }
}

int main (void)
{
    HAL_Init();

    // output clock on MCO pin (PA8)
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);

    APP_SystemClockConfig();
    SystemCoreClockUpdate();

    // setup pins
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // PA12 is LED
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){.Mode = GPIO_MODE_OUTPUT_OD, .Pin = GPIO_PIN_12});
    // PA11 is user pushbutton
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){.Mode = GPIO_MODE_INPUT, .Pin = GPIO_PIN_11});

    debounce_init(&user_button_pin_debounce, 1);

    // setup timer16, clock source is the PLL
    __HAL_RCC_TIM16_CLK_ENABLE();
    // initialize
    if (HAL_TIM_Base_Init(&tim16_hndl) != HAL_OK)
    {
        APP_ErrorHandler();
    }
    // start it
    if (HAL_TIM_Base_Start_IT(&tim16_hndl) != HAL_OK)
    {
        APP_ErrorHandler();
    }
    HAL_TIM_RegisterCallback(&tim16_hndl, HAL_TIM_PERIOD_ELAPSED_CB_ID, timer16_period_elapsed);

    int led_flash_rate = 100;

    while(1) {
        if (button_pressed == 0) {
            while (button_pressed == 0)
                HAL_Delay(1);
            led_flash_rate = led_flash_rate == 100 ? 200 : 100;
        }
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
        HAL_Delay(led_flash_rate);
    }
}

static void APP_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI \
        | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;  /* 配置HSE、HSI、LSI、LSE */
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                                             /* 开启HSI */
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                                             /* HSI不分频 */
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_16MHz;                    /* HSI校准频率16MHz */
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;                                            /* 关闭HSE */
    /* RCC_OscInitStruct.HSEFreq = RCC_HSE_16_32MHz; */                                  /* HSE频率范围16~32MHz */
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;                                            /* 关闭LSI */
    RCC_OscInitStruct.LSEState = RCC_LSE_OFF;                                            /* 关闭LSE */
    /* RCC_OscInitStruct.LSEDriver = RCC_LSEDRIVE_MEDIUM; */                             /* 默认LSE驱动能力 */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                                         /* 开启PLL */
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;                                 /* PLL时钟源HSI */
    /* 配置振荡器 */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;/* 配置SYSCLK、HCLK、PCLK */
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;                                     /* 配置系统时钟为PLL */
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;                                            /* AHB时钟不分频 */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;                                             /* APB时钟不分频 */
    /* 配置时钟源 */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

static void APP_ErrorHandler(void)
{
    while (1) {
    }
}
