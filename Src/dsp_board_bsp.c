
#include <stdio.h>
#include "main.h"
#include "dsp_board_bsp.h"
#include "stm32f4xx_hal_i2c.h"
#include <math.h>

EncoderValues_t henc1;
EncoderValues_t henc2;

extern ADC_HandleTypeDef hadc1;

float BSP_ReadBatteryVoltage(uint16_t* values, uint8_t size)
{
	uint32_t sum = 0;
	for(uint8_t i=0; i<size; i++){
		HAL_ADC_Start(&hadc1);
	}
	for(uint8_t i=0; i<size; i++){
		sum += (uint32_t)values[i];
	}
	sum /= (uint32_t)size;
	// 4096 = 3.3V
	// 1 = 3.3/4096;
	return 0.733333f * (float)(sum) * (3.3f/4096.0f);  // 4.5V correction factor = 0.7333
}

void BSP_SetBatteryCurrent(ChargeCurrent_t current)
{
	if(current == CHARGE_CURRENT_500MA)
		HAL_GPIO_WritePin(SET_I_LIM_GPIO_Port, SET_I_LIM_Pin, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(SET_I_LIM_GPIO_Port, SET_I_LIM_Pin, GPIO_PIN_RESET);
}

int32_t BSP_ReadEncoder_Difference(EncoderPosition_t encoder)
{
	BSP_ReadEncoder(encoder); // update both encoders

	// @TODO
	return 0;
}

uint32_t BSP_ReadEncoder(EncoderPosition_t encoder)
{
	
	henc1.value = (0xffff - TIM3->CNT)/2 +1;  // right encoder
	henc2.value = (         TIM4->CNT)/2;     // left encoder
	
	if(encoder == ENCODER_LEFT)
		return henc1.value;
	else
		return henc2.value;
}

void BSP_SelectAudioIn(uint8_t mode)
{
	if(mode == AUDIO_IN_EXT){
		HAL_GPIO_WritePin(SET_LIN_GPIO_Port, SET_LIN_Pin, GPIO_PIN_RESET);
	} else {
		// AUDIO_IN_LINE
		// default
		HAL_GPIO_WritePin(SET_LIN_GPIO_Port, SET_LIN_Pin, GPIO_PIN_SET);
	}
}

uint16_t BSP_SineWave(float fs, float fout, uint16_t A, uint16_t* pData, uint16_t len)
{
	float Ts   = 1.0f/fs;   // Sampling Period
	float Tsig = 1.0f/fout; // Signal Period
	float omega_sig = 2*BSP_PI*fout; // Signal Frequency in rad/s
	
	uint16_t n_samples = (uint16_t)( Tsig/Ts );  // number of Data Points
	
	// check if number of samples exceeds output buffer length
	if(len < (2*n_samples))
		n_samples = len/2;
	
	for(uint16_t i = 0; i<n_samples; i++){
		float dt = ((float) i ) * Ts;
		float sinVal = sinf( omega_sig * dt );
		pData[2*i]     = (uint16_t)(  (1 + sinVal) * ((float)A)  );  // Right Channel
		pData[2*i + 1] = (uint16_t)(  (1 + sinVal) * ((float)A)  );  // Left  Channel
	}
	return n_samples;
}


void BSP_Print_to_Matlab(uint16_t* vals, uint16_t len)
{
	printf("x = [");
	for(uint16_t i = 0; i < len; i++){
		if(i) printf(", ");  // print colon prior to value for i>0
		printf("%d", vals[i]);  // @TODO: is this correct dereference ?
	}
	printf(" ];\n");
	printf("plot(x); grid on;\n\n");
}

/** scan I2C ------------------------------------------------------------------*/
void BSP_I2C_ScanAddresses(I2C_HandleTypeDef *hi2c)
{
	uint8_t error, address;
	uint16_t nDevices;
	nDevices = 0;
	printf("Scanning for available I2C devices...\n");
	for(address = 1; address < 127; address++ )
	{
		HAL_Delay(10);
		error = HAL_I2C_Master_Transmit(hi2c, address, 0x00, 1, 1);
		//error = HAL_I2C_Master_Receive(hi2c, address, 0x00, 1, 1);
		if (error == HAL_OK)
		{
			printf("I2C device found at address 0x");
			if (address<16)
				printf("0");
			printf("%X", address);
			printf("  !\n");
			nDevices++;
		}
	}
	if (nDevices == 0)
		printf("No I2C devices found\n");
	else
		printf("done\n");
}
