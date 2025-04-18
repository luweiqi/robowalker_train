/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "1_Middleware/1_Driver/BSP/drv_djiboarda.h"

#include "2_Device/Motor/Motor_DJI/dvc_motor_dji.h"
#include "2_Device/Serialplot/dvc_serialplot.h"
#include "2_Device/Powermeter/dvc_powermeter.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

bool init_finished = 0;

Class_Serialplot serialplot;
Class_Powermeter powermeter;
Class_Motor_DJI_C620 motor;

int32_t Counter = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief CAN报文回调函数
 *
 * @param Rx_Buffer CAN接收的信息结构体
 */
void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer)
{
    switch (Rx_Buffer->Header.StdId)
    {
    case (0x201):
    {
        motor.CAN_RxCpltCallback(Rx_Buffer->Data);
    }
    }
}

/**
 * @brief HAL库UART接收DMA空闲中断
 *
 * @param huart UART编号
 * @param Size 长度
 */
void UART_Serialplot_Call_Back(uint8_t *Buffer, uint16_t Length)
{
}

/**
 * @brief HAL库UART接收DMA空闲中断
 *
 * @param huart UART编号
 * @param Size 长度
 */
void UART_Powermeter_Call_Back(uint8_t *Buffer, uint16_t Length)
{
    powermeter.UART_RxCpltCallback(Buffer, Length);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CAN2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_TIM5_Init();
    MX_TIM12_Init();
    MX_CAN1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_USART6_UART_Init();
    MX_UART7_Init();
    MX_UART8_Init();
    MX_ADC1_Init();
    MX_SPI5_Init();
    /* USER CODE BEGIN 2 */

    BSP_Init(BSP_DC24_LU_ON | BSP_DC24_LD_ON | BSP_DC24_RU_ON | BSP_DC24_RD_ON);
    CAN_Init(&hcan1, CAN_Motor_Call_Back);
    UART_Init(&huart2, UART_Serialplot_Call_Back, SERIALPLOT_RX_VARIABLE_ASSIGNMENT_MAX_LENGTH);
    UART_Init(&huart6, UART_Powermeter_Call_Back, 20);

    serialplot.Init(&huart2, Serialplot_Checksum_8_ENABLE, NULL);
    powermeter.Init(&huart6);

    motor.PID_Omega.Init(0.15f, 0.0f, 0.0f, 0.0f, 20.0f, 20.0f);
    motor.Init(&hcan1, Motor_DJI_ID_0x201, Motor_DJI_Control_Method_CURRENT, 1.0f);

    init_finished = 1;

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        Counter++;
        motor.Set_Control_Method(Motor_DJI_Control_Method_CURRENT);
        motor.Set_Target_Current(5.0f * sinf(10.0f * PI * (Counter - 22000) / 1000.0f));

        // 保持存活
        static uint32_t Counter_KeepAlive = 0;
        if (Counter_KeepAlive++ > 100)
        {
            Counter_KeepAlive = 0;

            motor.TIM_100ms_Alive_PeriodElapsedCallback();
        }

        // 串口绘图显示内容

        float now_power = powermeter.Get_Power();
        // k0: 0.6641993412640775
        // k1: 0.006444284468539646
        // k2: 0.0001423857226262331
        // k3: 0.017644430204543864
        // k4: 0.1650143850678086
        // k5: 3.096721772539512e-05
        // k0 + k1 * I + k2 * ω + k3 * I * ω + k4 * I^2 + k5 * ω^2
        float estimate_power = 0.6641993412640775f + 0.006444284468539646f * motor.Get_Now_Current() + 0.0001423857226262331f * motor.Get_Now_Omega() + 0.017644430204543864f * motor.Get_Now_Current() * motor.Get_Now_Omega() + 0.1650143850678086f * motor.Get_Now_Current() * motor.Get_Now_Current() + 3.096721772539512e-05f * motor.Get_Now_Omega() * motor.Get_Now_Omega();
        // k0: 0.5991965101356735
        // k1: 0.008890313160294648
        // k2: 0.0027051431483868043
        // k3: 0.017649028943483212
        // k4: 0.16397227788754365
        // k5: 1.7927069597532903e-05
        float estimate_power_abs_k2 = 0.5991965101356735f + 0.008890313160294648f * motor.Get_Now_Current() + 0.0027051431483868043f * Math_Abs(motor.Get_Now_Omega()) + 0.017649028943483212f * motor.Get_Now_Current() * motor.Get_Now_Omega() + 0.16397227788754365f * motor.Get_Now_Current() * motor.Get_Now_Current() + 1.7927069597532903e-05f * motor.Get_Now_Omega() * motor.Get_Now_Omega();
        // k0: 0.577098513642893
        // k1: 0.04305930072969628
        // k2: 0.0027755999369711956
        // k3: 0.01763683292538948
        // k4: 0.15492097167594218
        // k5: 1.7697891966311674e-05
        float estimate_power_abs_k1_k2 = 0.5991965101356735f + 0.008890313160294648f * Math_Abs(motor.Get_Now_Current()) + 0.0027051431483868043f * Math_Abs(motor.Get_Now_Omega()) + 0.017649028943483212f * motor.Get_Now_Current() * motor.Get_Now_Omega() + 0.16397227788754365f * motor.Get_Now_Current() * motor.Get_Now_Current() + 1.7927069597532903e-05f * motor.Get_Now_Omega() * motor.Get_Now_Omega();
        serialplot.Set_Data(4, &now_power, &estimate_power, &estimate_power_abs_k2, &estimate_power_abs_k1_k2);
        serialplot.TIM_1ms_Write_PeriodElapsedCallback();

        // 电机控制
        motor.TIM_Calculate_PeriodElapsedCallback();

        // 通信设备回调数据
        TIM_1ms_CAN_PeriodElapsedCallback();

        // 延时1ms
        HAL_Delay(0);
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 6;
    RCC_OscInitStruct.PLL.PLLN = 168;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
