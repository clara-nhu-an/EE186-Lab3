/* USER CODE BEGIN Header */
/* EE186 Lab 3 - Part 5b: single-transaction XYZ read */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* USER CODE BEGIN PD */
#define ACCEL_ADDRESS  0x19u
#define MAG_ADDRESS    0x1Eu

#define WHO_AM_I_A     0x0Fu
#define WHO_AM_I_M     0x4Fu

#define CTRL_REG1_A    0x20u
#define CTRL_REG2_A    0x21u
#define CTRL_REG4_A    0x23u

#define OUT_X_L_A      0x28u
#define AUTO_INCREMENT 0x80u
/* USER CODE END PD */

/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Peripheral handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;

/* USER CODE BEGIN PV */
volatile uint8_t accel_id = 0xFFu;
volatile uint8_t mag_id = 0xFFu;

volatile HAL_StatusTypeDef accel_status = HAL_ERROR;
volatile HAL_StatusTypeDef mag_status = HAL_ERROR;

volatile int16_t raw_x = 0;
volatile int16_t raw_y = 0;
volatile int16_t raw_z = 0;

volatile float x_g = 0;
volatile float y_g = 0;
volatile float z_g = 0;
/* USER CODE END PV */

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_LPUART1_UART_Init(void);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* Original UART printing function retained. */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(
        &hlpuart1,
        (uint8_t *)&ch,
        1,
        HAL_MAX_DELAY
    );

    return ch;
}

/* Combine low/high bytes into a signed 16-bit value. */
static int16_t DecodeAxis(uint8_t low, uint8_t high)
{
    int32_t value = ((uint16_t)high << 8) | low;

    if (value & 0x8000)
    {
        value -= 65536;
    }

    return (int16_t)value;
}

/*
 * PART 5b:
 * Read X, Y and Z in ONE I2C read transaction.
 *
 * Starting sub-address: 0x28 | 0x80 = 0xA8.
 * Bit 7 enables automatic register-address increment.
 *
 * Received byte order:
 * data[0] = X low  (0x28)
 * data[1] = X high (0x29)
 * data[2] = Y low  (0x2A)
 * data[3] = Y high (0x2B)
 * data[4] = Z low  (0x2C)
 * data[5] = Z high (0x2D)
 */
static HAL_StatusTypeDef ReadAllAxes(
    int16_t *x,
    int16_t *y,
    int16_t *z
)
{
    uint8_t data[6];

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        &hi2c1,
        ACCEL_ADDRESS << 1,
        OUT_X_L_A | AUTO_INCREMENT,
        I2C_MEMADD_SIZE_8BIT,
        data,
        sizeof(data),
        100
    );

    if (status != HAL_OK)
    {
        return status;
    }

    *x = DecodeAxis(data[0], data[1]);
    *y = DecodeAxis(data[2], data[3]);
    *z = DecodeAxis(data[4], data[5]);

    return HAL_OK;
}

static HAL_StatusTypeDef ConfigureAccelerometer(void)
{
    /*
     * Stop measurements while configuring.
     * Disable high-pass filtering to retain gravity.
     * Enable BDU, high resolution and +/-2 g range.
     * Enable X/Y/Z at 100 Hz.
     */
    const uint8_t registers[] = {
        CTRL_REG1_A,
        CTRL_REG2_A,
        CTRL_REG4_A,
        CTRL_REG1_A
    };

    const uint8_t values[] = {
        0x00,
        0x00,
        0x88,
        0x57
    };

    for (unsigned int i = 0; i < 4; ++i)
    {
        uint8_t value = values[i];

        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
            &hi2c1,
            ACCEL_ADDRESS << 1,
            registers[i],
            I2C_MEMADD_SIZE_8BIT,
            &value,
            1,
            100
        );

        if (status != HAL_OK)
        {
            return status;
        }
    }

    HAL_Delay(100);

    return HAL_OK;
}

static void PrintAxis(const char *axis, int16_t raw)
{
    /*
     * High-resolution output is 12-bit, left-justified.
     * raw / 16 gives the signed measurement in counts.
     * At +/-2 g: 1 count = 1 mg.
     *
     * Therefore:
     * acceleration in g = raw / 16000.
     *
     * Integer formatting avoids float printf requirements.
     */
    int mg = raw / 16;
    int magnitude = mg < 0 ? -mg : mg;

    printf(
        "%s: raw=%6d  %c%d.%03d g\r\n",
        axis,
        (int)raw,
        mg < 0 ? '-' : '+',
        magnitude / 1000,
        magnitude % 1000
    );
}

/* USER CODE END 0 */

int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_LPUART1_UART_Init();

    /* USER CODE BEGIN 2 */
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\r\nHello from Nucleo-L4R5ZI-P!\r\n");
    printf("Part 5b: XYZ in one six-byte I2C read\r\n");

    while (ConfigureAccelerometer() != HAL_OK)
    {
        printf(
            "Accelerometer setup failed: error=0x%08lX\r\n",
            (unsigned long)HAL_I2C_GetError(&hi2c1)
        );

        HAL_Delay(1000);
    }
    /* USER CODE END 2 */

    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        int16_t x = 0;
        int16_t y = 0;
        int16_t z = 0;

        /* One transaction obtains all three axes. */
        accel_status = ReadAllAxes(&x, &y, &z);

        if (accel_status == HAL_OK)
        {
            raw_x = x;
            raw_y = y;
            raw_z = z;

            x_g = (float)x / 16000.0f;
            y_g = (float)y / 16000.0f;
            z_g = (float)z / 16000.0f;

            printf("------------------------------\r\n");
            PrintAxis("X", x);
            PrintAxis("Y", y);
            PrintAxis("Z", z);
        }
        else
        {
            printf(
                "I2C read failed: status=%d error=0x%08lX\r\n",
                (int)accel_status,
                (unsigned long)HAL_I2C_GetError(&hi2c1)
            );
        }

        HAL_Delay(200);
    }
    /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(
            PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_I2C1_Init(void)
{
    /* USER CODE BEGIN I2C1_Init 0 */
    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */
    /* USER CODE END I2C1_Init 1 */

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00100D14;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(
            &hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN I2C1_Init 2 */
    /* USER CODE END I2C1_Init 2 */
}

static void MX_LPUART1_UART_Init(void)
{
    /* USER CODE BEGIN LPUART1_Init 0 */
    /* USER CODE END LPUART1_Init 0 */

    /* USER CODE BEGIN LPUART1_Init 1 */
    /* USER CODE END LPUART1_Init 1 */

    hlpuart1.Instance = LPUART1;
    hlpuart1.Init.BaudRate = 115200;
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    hlpuart1.FifoMode = UART_FIFOMODE_DISABLE;

    if (HAL_UART_Init(&hlpuart1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(
            &hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(
            &hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN LPUART1_Init 2 */
    /* USER CODE END LPUART1_Init 2 */
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    HAL_PWREx_EnableVddIO2();

    /* Configure GPIO pins: PE2 PE3 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* Configure GPIO pins: PF0 PF1 PF2 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* Configure GPIO pin: PF7 */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* Configure GPIO pins: PC0 PC1 PC2 PC3 PC4 PC5 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
        GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure GPIO pin: PA0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pins: PA1 PA3 */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pins: PA4 PA5 PA6 PA7 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pin: PB0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pin: PB1 */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pins: PB2 PB6 */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pins: PE7 PE8 PE9 PE10 PE11 PE12 PE13 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
        GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* Configure GPIO pins: PE14 PE15 */
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM1_COMP1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* Configure GPIO pin: PB10 */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pins: PB12 PB13 PB15 */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pin: PB14 */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF14_TIM15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pins: PD8 PD9 */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure GPIO pins: PD14 PD15 */
    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure GPIO pin: PC6 */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_SAI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure GPIO pin: PC7 */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure GPIO pins: PC8 PC9 PC10 PC11 PC12 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
        GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure GPIO pins: PA8 PA10 */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pin: PA9 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure GPIO pin: PD0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure GPIO pin: PD2 */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure GPIO pins: PD3 PD4 PD5 PD6 */
    GPIO_InitStruct.Pin =
        GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Configure GPIO pins: PB3 PB4 PB5 */
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure GPIO pin: PE0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif
