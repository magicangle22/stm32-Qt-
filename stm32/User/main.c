#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "Key.h"

uint8_t KeyNum;

int main(void)
{
	OLED_Init();
	Key_Init();
	Serial_Init();
	
	OLED_ShowString(2,1,"Key1:SendImg2");
	
	Serial_TxPacket[0] = 0x01;
	Serial_TxPacket[1] = 0x02;
	Serial_TxPacket[2] = 0x03;
	Serial_TxPacket[3] = 0x04;
	
	Serial_SendPacket();
	
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == 1)
		{
			Serial_TxPacket[0] ++;
			Serial_TxPacket[1] ++;
			Serial_TxPacket[2] ++;
			Serial_TxPacket[3] ++;
			
			OLED_ShowHexNum(2,1,Serial_TxPacket[0],2);
			OLED_ShowHexNum(2,4,Serial_TxPacket[1],2);
			OLED_ShowHexNum(2,7,Serial_TxPacket[2],2);
			OLED_ShowHexNum(2,10,Serial_TxPacket[3],2);
			
			Serial_SendPacket();
		}
		if(Serial_GetRxFlag() == 1)
		{
			OLED_ShowHexNum(4,1,Serial_RxPacket[0],2);
			OLED_ShowHexNum(4,4,Serial_RxPacket[1],2);
			OLED_ShowHexNum(4,7,Serial_RxPacket[2],2);
			OLED_ShowHexNum(4,10,Serial_RxPacket[3],2);
		}
		if(Serial_GetImageRxFlag() == 1)
		{
			OLED_ShowImage(Serial_ImageRxBuffer);
		}
	}
}
