#include "stm32f10x.h"
#include "OLED_Font.h"

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

/*引脚初始化*/
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C\u5f00\u59cb
  * @param  \u65e0
  * @retval \u65e0
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C\u505c\u6b62
  * @param  \u65e0
  * @retval \u65e0
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C\u53d1\u9001\u4e00\u4e2a\u5b57\u8282
  * @param  Byte \u8981\u53d1\u9001\u7684\u4e00\u4e2a\u5b57\u8282
  * @retval \u65e0
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);	//\u989d\u5916\u7684\u4e00\u4e2a\u65f6\u949f\uff0c\u4e0d\u5904\u7406\u5e94\u7b54\u4fe1\u53f7
	OLED_W_SCL(0);
}

/**
  * @brief  OLED\u5199\u547d\u4ee4
  * @param  Command \u8981\u5199\u5165\u7684\u547d\u4ee4
  * @retval \u65e0
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//\u4ece\u673a\u5730\u5740
	OLED_I2C_SendByte(0x00);		//\u5199\u547d\u4ee4
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED\u5199\u6570\u636e
  * @param  Data \u8981\u5199\u5165\u7684\u6570\u636e
  * @retval \u65e0
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//\u4ece\u673a\u5730\u5740
	OLED_I2C_SendByte(0x40);		//\u5199\u6570\u636e
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED\u8bbe\u7f6e\u5149\u6807\u4f4d\u7f6e
  * @param  Y \u4ee5\u5de6\u4e0a\u89d2\u4e3a\u539f\u70b9\uff0c\u5411\u4e0b\u65b9\u5411\u7684\u5750\u6807\uff0c\u8303\u56f4\uff1a0~7
  * @param  X \u4ee5\u5de6\u4e0a\u89d2\u4e3a\u539f\u70b9\uff0c\u5411\u53f3\u65b9\u5411\u7684\u5750\u6807\uff0c\u8303\u56f4\uff1a0~127
  * @retval \u65e0
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//\u8bbe\u7f6eY\u4f4d\u7f6e
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//\u8bbe\u7f6eX\u4f4d\u7f6e\u9ad84\u4f4d
	OLED_WriteCommand(0x00 | (X & 0x0F));			//\u8bbe\u7f6eX\u4f4d\u7f6e\u4f4e4\u4f4d
}

/**
  * @brief  OLED\u6e05\u5c4f
  * @param  \u65e0
  * @retval \u65e0
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED\u663e\u793a\u4e00\u4e2a\u5b57\u7b26
  * @param  Line \u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  Char \u8981\u663e\u793a\u7684\u4e00\u4e2a\u5b57\u7b26\uff0c\u8303\u56f4\uff1aASCII\u53ef\u89c1\u5b57\u7b26
  * @retval \u65e0
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//\u8bbe\u7f6e\u5149\u6807\u4f4d\u7f6e\u5728\u4e0a\u534a\u90e8\u5206
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//\u663e\u793a\u4e0a\u534a\u90e8\u5206\u5185\u5bb9
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//\u8bbe\u7f6e\u5149\u6807\u4f4d\u7f6e\u5728\u4e0b\u534a\u90e8\u5206
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//\u663e\u793a\u4e0b\u534a\u90e8\u5206\u5185\u5bb9
	}
}

/**
  * @brief  OLED\u663e\u793a\u5b57\u7b26\u4e32
  * @param  Line \u8d77\u59cb\u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u8d77\u59cb\u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  String \u8981\u663e\u793a\u7684\u5b57\u7b26\u4e32\uff0c\u8303\u56f4\uff1aASCII\u53ef\u89c1\u5b57\u7b26
  * @retval \u65e0
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED\u6b21\u65b9\u51fd\u6570
  * @retval \u8fd4\u56de\u503c\u7b49\u4e8eX\u7684Y\u6b21\u65b9
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED\u663e\u793a\u6570\u5b57\uff08\u5341\u8fdb\u5236\uff0c\u6b63\u6570\uff09
  * @param  Line \u8d77\u59cb\u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u8d77\u59cb\u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  Number \u8981\u663e\u793a\u7684\u6570\u5b57\uff0c\u8303\u56f4\uff1a0~4294967295
  * @param  Length \u8981\u663e\u793a\u6570\u5b57\u7684\u957f\u5ea6\uff0c\u8303\u56f4\uff1a1~10
  * @retval \u65e0
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED\u663e\u793a\u6570\u5b57\uff08\u5341\u8fdb\u5236\uff0c\u5e26\u7b26\u53f7\u6570\uff09
  * @param  Line \u8d77\u59cb\u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u8d77\u59cb\u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  Number \u8981\u663e\u793a\u7684\u6570\u5b57\uff0c\u8303\u56f4\uff1a-2147483648~2147483647
  * @param  Length \u8981\u663e\u793a\u6570\u5b57\u7684\u957f\u5ea6\uff0c\u8303\u56f4\uff1a1~10
  * @retval \u65e0
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED\u663e\u793a\u6570\u5b57\uff08\u5341\u516d\u8fdb\u5236\uff0c\u6b63\u6570\uff09
  * @param  Line \u8d77\u59cb\u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u8d77\u59cb\u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  Number \u8981\u663e\u793a\u7684\u6570\u5b57\uff0c\u8303\u56f4\uff1a0~0xFFFFFFFF
  * @param  Length \u8981\u663e\u793a\u6570\u5b57\u7684\u957f\u5ea6\uff0c\u8303\u56f4\uff1a1~8
  * @retval \u65e0
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED\u663e\u793a\u6570\u5b57\uff08\u4e8c\u8fdb\u5236\uff0c\u6b63\u6570\uff09
  * @param  Line \u8d77\u59cb\u884c\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~4
  * @param  Column \u8d77\u59cb\u5217\u4f4d\u7f6e\uff0c\u8303\u56f4\uff1a1~16
  * @param  Number \u8981\u663e\u793a\u7684\u6570\u5b57\uff0c\u8303\u56f4\uff1a0~1111 1111 1111 1111
  * @param  Length \u8981\u663e\u793a\u6570\u5b57\u7684\u957f\u5ea6\uff0c\u8303\u56f4\uff1a1~16
  * @retval \u65e0
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED\u663e\u793a\u56fe\u7247 (128x64 monochrome bitmap, 1024 bytes)
  * @param  Bitmap \u6307\u54111024\u5b57\u8282\u4f4d\u56fe\u6570\u636e\u7684\u6307\u9488\uff0c\u6bcf\u5b57\u82828\u4e2a\u50cf\u7d20\uff0cMSB\u5728\u524d
  * @retval \u65e0
  */
void OLED_ShowImage(const uint8_t *Bitmap)
{
    uint8_t page, col;
    for (page = 0; page < 8; page++)
    {
        OLED_SetCursor(page, 0);
        for (col = 0; col < 128; col++)
        {
            OLED_WriteData(Bitmap[page * 128 + col]);
        }
    }
}
void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//\u4e0a\u7535\u5ef6\u65f6
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_I2C_Init();			//\u7aef\u53e3\u521d\u59cb\u5316
	
	OLED_WriteCommand(0xAE);	//\u5173\u95ed\u663e\u793a
	
	OLED_WriteCommand(0xD5);	//\u8bbe\u7f6e\u663e\u793a\u65f6\u949f\u5206\u9891\u6bd4/\u632f\u8361\u5668\u9891\u7387
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//\u8bbe\u7f6e\u591a\u8def\u590d\u7528\u7387
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//\u8bbe\u7f6e\u663e\u793a\u504f\u79fb
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//\u8bbe\u7f6e\u663e\u793a\u5f00\u59cb\u884c
	
	OLED_WriteCommand(0xA1);	//\u8bbe\u7f6e\u5de6\u53f3\u65b9\u5411\uff0c0xA1\u6b63\u5e38 0xA0\u5de6\u53f3\u53cd\u7f6e
	
	OLED_WriteCommand(0xC8);	//\u8bbe\u7f6e\u4e0a\u4e0b\u65b9\u5411\uff0c0xC8\u6b63\u5e38 0xC0\u4e0a\u4e0b\u53cd\u7f6e

	OLED_WriteCommand(0xDA);	//\u8bbe\u7f6eCOM\u5f15\u811a\u786c\u4ef6\u914d\u7f6e
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//\u8bbe\u7f6e\u5bf9\u6bd4\u5ea6\u63a7\u5236
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//\u8bbe\u7f6e\u9884\u5145\u7535\u5468\u671f
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//\u8bbe\u7f6eVCOMH\u53d6\u6d88\u9009\u62e9\u7ea7\u522b
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//\u8bbe\u7f6e\u6574\u4e2a\u663e\u793a\u6253\u5f00/\u5173\u95ed

	OLED_WriteCommand(0xA6);	//\u8bbe\u7f6e\u6b63\u5e38/\u5012\u8f6c\u663e\u793a

	OLED_WriteCommand(0x8D);	//\u8bbe\u7f6e\u5145\u7535\u6cf5
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//\u5f00\u542f\u663e\u793a
		
	OLED_Clear();				//OLED\u6e05\u5c4f
}
