#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

#define IMAGE_DATA_SIZE  1024

extern uint8_t Serial_TxPacket[4];
extern uint8_t Serial_RxPacket[4];
extern uint8_t Serial_ImageRxBuffer[IMAGE_DATA_SIZE];
extern uint8_t Serial_ImageRxFlag;

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array, uint16_t Length);
void Serial_SendString(char *String);
void Serial_SendNumer(uint32_t Num,uint8_t Length);
void Serial_Printf(char* format,...);
uint8_t Serial_GetRxFlag(void);
uint8_t Serial_GetImageRxFlag(void);
void Serial_SendPacket(void);

#endif
