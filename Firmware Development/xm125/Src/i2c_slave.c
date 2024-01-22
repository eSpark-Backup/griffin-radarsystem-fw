/*
 * i2c_slave.c
 *
 *  Created on: Jun 9, 2023
 *      Author: controllerstech
 */


#include "main.h"
#include "i2c_slave.h"

extern I2C_HandleTypeDef hi2c2;
extern volatile float speed;
extern volatile int32_t max_magnitude_index;
extern volatile uint8_t numDistances;
extern volatile float distances[10];

typedef struct
{
	float speed;
	int32_t maxMagnitudeIndex;
	float distanceArray[10];
	float *ptrToArray;
	uint8_t numDistances;

}TransmitData;

#define RxSIZE  6
uint8_t RxData[RxSIZE];

int count = 0;

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	TransmitData transmitData;
	transmitData.speed = speed;
	transmitData.maxMagnitudeIndex = max_magnitude_index;
	transmitData.numDistances = 5;
    printf("logssssssssssssssssssssssssssssssssssssssssssssss\n");
    float array[5] = {2,1,3,5,2};
	for(uint8_t i=0; i<transmitData.numDistances ;i++)
	{
	   transmitData.distanceArray[i] = array[i];
//	   printf("number distnaces %d distance Value %f\n", numDistances ,  distances[i]);
	}
	if (TransferDirection == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
		// receive using sequential function.
		// The I2C_FIRST_AND_LAST_FRAME implies that the slave will send a NACK after receiving "entered" num of bytes
		HAL_I2C_Slave_Sequential_Receive_IT(hi2c, RxData, RxSIZE, I2C_FIRST_AND_LAST_FRAME);
//		for (int i = 0; i<6 ; i++)
//		{
//			printf("RxData: %d\n" , RxData[i]);
//		}

	}
	else  // if the master requests the data from the slave
	{
//		for (uint8_t i = 0; i < numDistances ; i++)
//		{
//			printf("number distnaces %d distance Value %f\n", numDistances ,  distances[i]);
//
//		}
		HAL_I2C_Slave_Seq_Transmit_IT(hi2c, (uint8_t*)&transmitData , sizeof(transmitData) , I2C_FIRST_FRAME);

	}
}

//void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
//{
////	txcount++;
//	uint8_t txData = 6;
//	HAL_I2C_Slave_Seq_Transmit_IT(hi2c, txData, 1, I2C_NEXT_FRAME);
//}

//void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
//{
//	count++;
//}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	HAL_I2C_EnableListen_IT(hi2c);
}
