/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 * This notice applies to any and all portions of this file
 * that are not between comment pairs USER CODE BEGIN and
 * USER CODE END. Other portions of this file, whether
 * inserted by the user or by software development tools
 * are owned by their respective copyright owners.
 *
 * Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_hal.h"
#include "fatfs.h"
#include "audio_hal.h"
#include "retarget.h"
#include "logger.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
DFSDM_Filter_HandleTypeDef hdfsdm1_filter0;
DFSDM_Channel_HandleTypeDef hdfsdm1_channel0;
DMA_HandleTypeDef hdma_dfsdm1_flt0;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd1;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

#define N_MS N_MS_PER_INTERRUPT

#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLING_FREQUENCY 32000

#if (AUDIO_SAMPLING_FREQUENCY == 8000)
#define MAX_DECIMATION_FACTOR 160
#else
#define MAX_DECIMATION_FACTOR 128
#endif

#define PCM_BUFFER_LENGTH ((AUDIO_SAMPLING_FREQUENCY/1000)*AUDIO_CHANNELS * N_MS)
//#define PCM_BUFFER_LENGTH 32
#define RECORD_TIME 20000
#define SD_BUFFER_TIME 250
#define PCM_SD_BUFFER_LENGTH 8192 //((AUDIO_SAMPLING_FREQUENCY/1000)*AUDIO_CHANNELS * SD_BUFFER_TIME)

wave_sample_t PCM_Buffer[PCM_BUFFER_LENGTH];
wave_sample_t PCM_SD_buffers[2][PCM_SD_BUFFER_LENGTH];
bool audio_ready = false;
uint32_t SD_buffer_pos[2] = {0, 0};
uint32_t SD_buffer_num = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_DFSDM1_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_UART4_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI1_Init(void);
static void MX_USB_OTG_FS_USB_Init(void);
static void MX_I2C2_Init(void);
static void MX_I2C1_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

void record_audio(FIL* file)
{
	// Preallocate 10 seconds of audio
	FRESULT res = f_lseek(file, (AUDIO_SAMPLING_FREQUENCY/1000)*RECORD_TIME*3);
	if(res != FR_OK)
	{
		Error_Handler();
	}

	res = f_lseek(file, 0);
	if(res != FR_OK)
	{
		Error_Handler();
	}

	//Init_Acquisition_Peripherals(AUDIO_SAMPLING_FREQUENCY, AUDIO_CHANNELS, 0);
	if(BSP_AUDIO_IN_Init(AUDIO_SAMPLING_FREQUENCY, 16, 1) != AUDIO_OK)
		Error_Handler();

	//Start_Acquisition();
	if(BSP_AUDIO_IN_Record((uint16_t *)PCM_Buffer, 0) != AUDIO_OK)
		Error_Handler();

	uint32_t start = HAL_GetTick();
	uint32_t tot_samples = 0;

	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, 1);
	while(HAL_GetTick() < start + RECORD_TIME)
	{
		if(audio_ready)
		{
			wave_sample_t* buffer_arr[] = {PCM_SD_buffers[SD_buffer_num]};
			uint32_t num_samples = SD_buffer_pos[SD_buffer_num];
			SD_buffer_pos[0] = 0;
			SD_buffer_pos[1] = 0;
			tot_samples += num_samples;
			SD_buffer_num = (SD_buffer_num + 1)%2;
			audio_ready = false;

			if(logger_wav_append_nchannels(file, 1, num_samples, buffer_arr) != LOGGER_OK)
			{
				Error_Handler();
			}
		}
	}
	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, 0);
	BSP_AUDIO_IN_Stop();

	//	wave_sample_t* PCM_Buffer_arr[] = {PCM_Buffer_2};
	//	if(logger_wav_append_nchannels(file, 1, tot_samples, PCM_Buffer_arr) != LOGGER_OK)
	//	{
	//		Error_Handler();
	//	}

	if(logger_wav_write_header(file, AUDIO_SAMPLING_FREQUENCY, 1, tot_samples) != LOGGER_OK)
	{
		Error_Handler();
	}

	//while(1);
}




/**
 * @brief  Half Transfer user callback, called by BSP functions.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_IN_HalfTransfer_CallBack(void)
{
	BSP_AUDIO_IN_TransferComplete_CallBack();
}

/**
 * @brief  Transfer Complete user callback, called by BSP functions.
 * @param  None
 * @retval None
 */
void BSP_AUDIO_IN_TransferComplete_CallBack(void)
{
	if(audio_ready) return;
	if(SD_buffer_pos[SD_buffer_num] + PCM_BUFFER_LENGTH < PCM_SD_BUFFER_LENGTH)
	{
		memcpy(PCM_SD_buffers[SD_buffer_num] + SD_buffer_pos[SD_buffer_num], PCM_Buffer, PCM_BUFFER_LENGTH*sizeof(wave_sample_t));
		SD_buffer_pos[SD_buffer_num] += PCM_BUFFER_LENGTH;
	}
	if(SD_buffer_pos[SD_buffer_num] + PCM_BUFFER_LENGTH >= PCM_SD_BUFFER_LENGTH) audio_ready = true;
}

void BSP_AUDIO_IN_Error_Callback(void)
{
	Error_Handler();
}



#define N_BLOCKS 2
bool tx_cplt = false;

int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

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
	MX_SDMMC1_SD_Init();
	//MX_DFSDM1_Init();
	//MX_USART3_UART_Init();
	MX_UART4_Init();
	//MX_RTC_Init();
	//MX_SPI1_Init();
	//MX_USB_OTG_FS_USB_Init();
	//MX_I2C2_Init();
	//MX_FATFS_Init();
	//MX_I2C1_Init();

	RetargetInit(&huart4);


//	if(BSP_AUDIO_IN_Init(AUDIO_SAMPLING_FREQUENCY, 16, 1) != AUDIO_OK)
//			Error_Handler();
//	if(BSP_AUDIO_IN_Record((uint16_t *)PCM_Buffer, 0) != AUDIO_OK)
//		Error_Handler();
//
//	if(BSP_SD_Init() != MSD_OK)
//	{
//		printf("Init error\n");
//		Error_Handler();
//	}
//
//	HAL_SD_CardInfoTypeDef info;
//	HAL_SD_CardStateTypedef state;
//	HAL_SD_GetCardInfo(&_HSD, &info);
//	state = HAL_SD_GetCardState(&_HSD);
//
//	printf("Sector size %lu\n", info.LogBlockSize);
//	printf("Num sectors %lu\n", info.LogBlockNbr);
//	printf("State %i\n", state);
//
//	uint8_t pData[512*N_BLOCKS];
//	memset(pData, 0x33, 512*N_BLOCKS);

//	if (HAL_SD_WriteBlocks_DMA(&_HSD, (uint8_t *)pData, 0, N_BLOCKS) != HAL_OK)
//	{
//		printf("Fail starting DMA write\n");
//		Error_Handler();
//	}

//	if (HAL_SD_WriteBlocks(&_HSD, (uint8_t *)pData, 0, N_BLOCKS, 10000) != HAL_OK)
//	{
//		printf("Fail starting write\n");
//		Error_Handler();
//	}
//
//
//	//while(!tx_cplt);
//	//printf("Xfer complete\n");
//
//	while((state = HAL_SD_GetCardState(&_HSD)) != HAL_SD_CARD_TRANSFER);
//
//	printf("State after xfer %i\n", state);


	FATFS SDFatFs;  /* File system object for SD card logical drive */
	FIL MyFile;     /* File object */
	//char SDPath[4]; /* SD card logical drive path */

	FRESULT res;                                          /* FatFs function common result code */
	uint32_t byteswritten, bytesread;                     /* File write/read counts */
//	uint8_t wtext[] = "This is STM32 working with FatFs"; /* File write buffer */
	uint8_t rtext[100];                                   /* File read buffer */
//
	/*##-1- Link the micro SD disk I/O driver ##################################*/
	if(FATFS_LinkDriver(&SD_Driver, SD_Path) == 0)
	{
		/*##-2- Register the file system object to the FatFs module ##############*/
		if(f_mount(&SDFatFs, (TCHAR const*)SD_Path, 0) != FR_OK)
		{
			/* FatFs Initialization Error */
			Error_Handler();
		}
		else
		{
			/*##-3- Create a FAT file system (format) on the logical drive #########*/
			/* WARNING: Formatting the uSD card will delete all content on the device */
			if(f_mkfs((TCHAR const*)SD_Path, 0, 0) != FR_OK)
			{
				/* FatFs Format Error */
				Error_Handler();
			}
			else
			{
				/*##-4- Create and Open a new text file object with write access #####*/
				if(f_open(&MyFile, "audio.wav", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
				{
					/* 'STM32.TXT' file Open for write Error */
					Error_Handler();
				}
				else
				{
					/*##-5- Write data to the text file ################################*/
//					res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);
//										wave_sample_t samples_0[] = {0x1122, 0x3344};
//										wave_sample_t samples_1[] = {0x5566, 0x7788};
//										wave_sample_t* samples[2];
//										samples[0] = samples_0;
//										samples[1] = samples_1;
//
//										logger_wav_write_header(&MyFile, 22050, 2, 512);
//										res = logger_wav_append_nchannels(&MyFile, 2, 2, samples);
					record_audio(&MyFile);

					byteswritten = f_tell(&MyFile);

					/*##-6- Close the open text file #################################*/
					if (f_close(&MyFile) != FR_OK )
					{
						Error_Handler();
					}

					if((byteswritten == 0) || (res != FR_OK))
					{
						/* 'STM32.TXT' file Write or EOF Error */
						//Error_Handler();
					}
					else
					{
						/*##-7- Open the text file object with read access ###############*/
						if(f_open(&MyFile, "audio.wav", FA_READ) != FR_OK)
						{
							/* 'STM32.TXT' file Open for read Error */
							Error_Handler();
						}
						else
						{
							/*##-8- Read data from the text file ###########################*/
							res = f_read(&MyFile, rtext, sizeof(rtext), (UINT*)&bytesread);

							if((bytesread == 0) || (res != FR_OK))
							{
								/* 'STM32.TXT' file Read or EOF Error */
								//Error_Handler();
							}
							else
							{
								/*##-9- Close the open text file #############################*/
								f_close(&MyFile);

								/*##-10- Compare read data with the expected data ############*/
								if((bytesread != byteswritten))
								{
									/* Read data is different from the expected data */
									//Error_Handler();
								}
								else
								{
									/* Success of the demo: no error occurrence */
									//BSP_LED_On(LED2);
									HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
								}
							}
						}
					}
				}
			}
		}
	}

	/*##-11- Unlink the RAM disk I/O driver ####################################*/
	FATFS_UnLinkDriver(SD_Path);


	/* Infinite loop */
	while (1)
	{
	}
}


/** System Clock Configuration
 */
void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit;

	/**Configure LSE Drive Capability
	 */
	__HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE
			|RCC_OSCILLATORTYPE_MSI;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.MSIState = RCC_MSI_ON;
	RCC_OscInitStruct.MSICalibrationValue = 0;
	RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
	RCC_OscInitStruct.PLL.PLLM = 6;
	RCC_OscInitStruct.PLL.PLLN = 40;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV4;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART3
			|RCC_PERIPHCLK_UART4|RCC_PERIPHCLK_SAI1
			|RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_I2C2
			|RCC_PERIPHCLK_DFSDM1|RCC_PERIPHCLK_USB
			|RCC_PERIPHCLK_SDMMC1;
	PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_HSI;
	PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_HSI;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_HSI;
	PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI1;
	PeriphClkInit.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_PCLK;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_MSI;
	PeriphClkInit.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_MSI;
	PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
	PeriphClkInit.PLLSAI1.PLLSAI1M = 6;
	PeriphClkInit.PLLSAI1.PLLSAI1N = 43;
	PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
	PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV8;
	PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
	PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	HAL_RCCEx_EnableLSCO(RCC_LSCOSOURCE_LSE);

	/**Enables the Clock Security System
	 */
	 HAL_RCCEx_EnableLSECSS();

	/**Configure the main internal regulator output voltage
	 */
	 if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	 {
		 _Error_Handler(__FILE__, __LINE__);
	 }

	 /**Configure the Systick interrupt time
	  */
	 HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	 /**Configure the Systick
	  */
	 HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	 /**Enable MSI Auto calibration
	  */
	 HAL_RCCEx_EnableMSIPLLMode();

	 /* SysTick_IRQn interrupt configuration */
	 HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* DFSDM1 init function */
static void MX_DFSDM1_Init(void)
{

	hdfsdm1_filter0.Instance = DFSDM1_Filter0;
	hdfsdm1_filter0.Init.RegularParam.Trigger = DFSDM_FILTER_SW_TRIGGER;
	hdfsdm1_filter0.Init.RegularParam.FastMode = ENABLE;
	hdfsdm1_filter0.Init.RegularParam.DmaMode = ENABLE;
	hdfsdm1_filter0.Init.InjectedParam.Trigger = DFSDM_FILTER_SW_TRIGGER;
	hdfsdm1_filter0.Init.InjectedParam.ScanMode = DISABLE;
	hdfsdm1_filter0.Init.InjectedParam.DmaMode = DISABLE;
	hdfsdm1_filter0.Init.InjectedParam.ExtTrigger = DFSDM_FILTER_EXT_TRIG_TIM1_TRGO;
	hdfsdm1_filter0.Init.InjectedParam.ExtTriggerEdge = DFSDM_FILTER_EXT_TRIG_RISING_EDGE;
	hdfsdm1_filter0.Init.FilterParam.SincOrder = DFSDM_FILTER_SINC5_ORDER;
	hdfsdm1_filter0.Init.FilterParam.Oversampling = 1;
	hdfsdm1_filter0.Init.FilterParam.IntOversampling = 1;
	if (HAL_DFSDM_FilterInit(&hdfsdm1_filter0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	hdfsdm1_channel0.Instance = DFSDM1_Channel0;
	hdfsdm1_channel0.Init.OutputClock.Activation = ENABLE;
	hdfsdm1_channel0.Init.OutputClock.Selection = DFSDM_CHANNEL_OUTPUT_CLOCK_AUDIO;
	hdfsdm1_channel0.Init.OutputClock.Divider = 24;
	hdfsdm1_channel0.Init.Input.Multiplexer = DFSDM_CHANNEL_EXTERNAL_INPUTS;
	hdfsdm1_channel0.Init.Input.DataPacking = DFSDM_CHANNEL_STANDARD_MODE;
	hdfsdm1_channel0.Init.Input.Pins = DFSDM_CHANNEL_SAME_CHANNEL_PINS;
	hdfsdm1_channel0.Init.SerialInterface.Type = DFSDM_CHANNEL_SPI_RISING;
	hdfsdm1_channel0.Init.SerialInterface.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL;
	hdfsdm1_channel0.Init.Awd.FilterOrder = DFSDM_CHANNEL_SINC1_ORDER;
	hdfsdm1_channel0.Init.Awd.Oversampling = 10;
	hdfsdm1_channel0.Init.Offset = 0;
	hdfsdm1_channel0.Init.RightBitShift = 0x05;
	if (HAL_DFSDM_ChannelInit(&hdfsdm1_channel0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if (HAL_DFSDM_FilterConfigRegChannel(&hdfsdm1_filter0, DFSDM_CHANNEL_0, DFSDM_CONTINUOUS_CONV_ON) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	if (HAL_DFSDM_FilterConfigInjChannel(&hdfsdm1_filter0, DFSDM_CHANNEL_0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* I2C1 init function */
static void MX_I2C1_Init(void)
{

	hi2c1.Instance = I2C1;
	hi2c1.Init.Timing = 0x0010061A;
	hi2c1.Init.OwnAddress1 = 132;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* I2C2 init function */
static void MX_I2C2_Init(void)
{

	hi2c2.Instance = I2C2;
	hi2c2.Init.Timing = 0x0010061A;
	hi2c2.Init.OwnAddress1 = 224;
	hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c2.Init.OwnAddress2 = 0;
	hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c2) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	/**Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* RTC init function */
static void MX_RTC_Init(void)
{

	/**Initialize RTC Only
	 */
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
	if (HAL_RTC_Init(&hrtc) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* SDMMC1 init function */
static void MX_SDMMC1_SD_Init(void)
{

	hsd1.Instance = SDMMC1;
	hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
	hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
	hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
	hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
	hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
	hsd1.Init.ClockDiv = 4;

}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 7;
	hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* UART4 init function */
static void MX_UART4_Init(void)
{

	huart4.Instance = UART4;
	huart4.Init.BaudRate = 115200;
	huart4.Init.WordLength = UART_WORDLENGTH_8B;
	huart4.Init.StopBits = UART_STOPBITS_1;
	huart4.Init.Parity = UART_PARITY_NONE;
	huart4.Init.Mode = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart4) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* USART3 init function */
static void MX_USART3_UART_Init(void)
{

	huart3.Instance = USART3;
	huart3.Init.BaudRate = 9600;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart3) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

}

/* USB_OTG_FS init function */
static void MX_USB_OTG_FS_USB_Init(void)
{

}

/** 
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void) 
{
	/* DMA controller clock enable */
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Channel4_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 2, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	/* DMA2_Channel4_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Channel4_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Channel4_IRQn);
	/* DMA2_Channel5_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA2_Channel5_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA2_Channel5_IRQn);

}

/** Configure pins as 
 * Analog
 * Input
 * Output
 * EVENT_OUT
 * EXTI
     PA2   ------> RCC_LSCO
     PA10   ------> USB_OTG_FS_ID
     PA11   ------> USB_OTG_FS_DM
     PA12   ------> USB_OTG_FS_DP
 */
static void MX_GPIO_Init(void)
{

	GPIO_InitTypeDef GPIO_InitStruct;

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, LED_2_Pin|LED_1_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPS_EXTINT_Pin|GPS_RESET_N_Pin|RADIO_RESET_N_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(PMIC_FAST_GPIO_Port, PMIC_FAST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : SD_CD_Pin */
	GPIO_InitStruct.Pin = SD_CD_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(SD_CD_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LED_2_Pin LED_1_Pin */
	GPIO_InitStruct.Pin = LED_2_Pin|LED_1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : SWITCH_1_Pin */
	GPIO_InitStruct.Pin = SWITCH_1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(SWITCH_1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PA2 */
	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : SWITCH_2_Pin */
	GPIO_InitStruct.Pin = SWITCH_2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(SWITCH_2_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : GPS_EXTINT_Pin GPS_RESET_N_Pin RADIO_RESET_N_Pin */
	GPIO_InitStruct.Pin = GPS_EXTINT_Pin|GPS_RESET_N_Pin|RADIO_RESET_N_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : RADIO_DIO_2_Pin RADIO_DIO_3_Pin RADIO_DIO_0_Pin RADIO_DIO_1_Pin */
	GPIO_InitStruct.Pin = RADIO_DIO_2_Pin|RADIO_DIO_3_Pin|RADIO_DIO_0_Pin|RADIO_DIO_1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : VUSB_DET_Pin GPS_TIMEPULSE_Pin */
	GPIO_InitStruct.Pin = VUSB_DET_Pin|GPS_TIMEPULSE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : RADIO_DIO_4_Pin RADIO_DIO_5_Pin */
	GPIO_InitStruct.Pin = RADIO_DIO_4_Pin|RADIO_DIO_5_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PMIC_FAST_Pin */
	GPIO_InitStruct.Pin = PMIC_FAST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(PMIC_FAST_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : PA10 PA11 PA12 */
	GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void _Error_Handler(char * file, int line)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1)
	{
		HAL_Delay(40);
		HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
