#include <stdio.h>
#include "py32f0xx_hal.h"
#include "debounce.h"

// timer16 overflow count for led change
const int TIM16_OVF_CNT_SLOW = 1200;
const int TIM16_OVF_CNT_FAST = 300;

// Timer16 is going to overflow at 50MHz / 2 * 20000 = 0.8ms
// this gives a debounce period of 13ms for 16bits
static TIM_HandleTypeDef tim16_hndl = {
    .Instance = TIM16,
    .Init.Prescaler = 2 - 1,
    .Init.CounterMode = TIM_COUNTERMODE_UP,
    .Init.Period = 20000 - 1,
    .Init.ClockDivision = TIM_CLOCKDIVISION_DIV1,
    .Init.RepetitionCounter = 1 - 1,
    .Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE,
};

// debouncing pin PA11
static debounce_pin_t user_button = {
    .bank = GPIOA,
    .pin = GPIO_PIN_11,
};

// will be 1 if button is pressed, 0 if not
static volatile int user_button_pressed = 0;

// whether to flash the led slowly or not
static uint32_t led_flash_slow = 0;

// declare functions
static void APP_SystemClockConfig(void);
static void APP_ErrorHandler(void);

/**
 * @brief Override HAL_MspInit
 */
void HAL_MspInit(void)
{
    // not really necessary, but examples have it
}

/**
 * @brief Override HAL_TIM_Base_MspInit
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    // need to use TIM16 with interrupt

    /* 使能TIM16时钟 */
    __HAL_RCC_TIM16_CLK_ENABLE();

    /* 设置中断优先级 */
    HAL_NVIC_SetPriority(TIM16_IRQn, 0, 0);
    /* 使能TIM1中断 */
    HAL_NVIC_EnableIRQ(TIM16_IRQn);
}

// Interrupt Vector
void SysTick_Handler()
{
    HAL_IncTick();
}

// Interrupt Vector
void TIM16_IRQHandler()
{
    HAL_TIM_IRQHandler(&tim16_hndl);
}

int main (void)
{
    HAL_Init();

    APP_SystemClockConfig();
    SystemCoreClockUpdate();

    // setup pins
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // PA12 is LED
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){.Mode = GPIO_MODE_OUTPUT_OD, .Pin = GPIO_PIN_12});
    // PA11 is user button
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){.Mode = GPIO_MODE_INPUT, .Pin = GPIO_PIN_11});

    debounce_init(&user_button, 1);

    // setup timer16, clock source is the PLL
    if (HAL_TIM_Base_Init(&tim16_hndl) != HAL_OK)
    {
        APP_ErrorHandler();
    }
    // start it
    if (HAL_TIM_Base_Start_IT(&tim16_hndl) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    while(1) {
        if (user_button_pressed) {
            while (user_button_pressed);
            // change up frequency - no need to protect as 4byte int has atomic reads/writes
            // value is only changed here, read only in interrupt fn
            led_flash_slow = led_flash_slow ? 0 : 1;
        }
    }

}

/**
  * @brief   更新中断回调函数
  * @param   htim：TIM句柄
  * @retval  无
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static int count = 0;
    count++;
    if ((led_flash_slow && count >= TIM16_OVF_CNT_SLOW) ||
        (!led_flash_slow && count >= TIM16_OVF_CNT_FAST)) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_12);
        count = 0;
    }

    debounce(&user_button);
    if (debounce_is_low(&user_button))
        user_button_pressed = 1;
    else if (debounce_is_high(&user_button))
        user_button_pressed = 0;
}

// use High Speed External Clock to drive PLL
static void APP_SystemClockConfig(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // output sysclock on MCO pin (PA8)
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);

    // setup the HSE clock
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEFreq = RCC_HSE_16_32MHz;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    // setup sysclk and the peripheral bus clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

static void APP_ErrorHandler(void)
{
    while (1) {}
}
