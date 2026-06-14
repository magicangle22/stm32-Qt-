#include "stm32f10x.h"
#include "OLED_Font.h"

#define OLED_ADDR  0x78

static uint8_t OLED_DMABuf[1025];

/* ---- Low-level I2C1 polled helpers ---- */
static void I2C1_Start(void)
{
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
}

static void I2C1_SendAddr(uint8_t addr)
{
    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
}

static void I2C1_SendByte(uint8_t data)
{
    I2C_SendData(I2C1, data);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

static void I2C1_Stop(void)
{
    I2C_GenerateSTOP(I2C1, ENABLE);
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF));
    I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);
}

static void I2C1_BusRecovery(void)
{
    GPIO_InitTypeDef g;
    /* Toggle SCL 10 times as GPIO to release stuck slave */
    GPIO_SetBits(GPIOB, GPIO_Pin_8 | GPIO_Pin_9);
    for (volatile int i = 0; i < 10; i++) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_8);
        for (volatile int j = 0; j < 20; j++);
        GPIO_SetBits(GPIOB, GPIO_Pin_8);
        for (volatile int j = 0; j < 20; j++);
    }
    /* Generate a STOP condition manually */
    GPIO_ResetBits(GPIOB, GPIO_Pin_9);  /* SDA low */
    for (volatile int j = 0; j < 20; j++);
    GPIO_SetBits(GPIOB, GPIO_Pin_8);    /* SCL high */
    for (volatile int j = 0; j < 20; j++);
    GPIO_SetBits(GPIOB, GPIO_Pin_9);    /* SDA high (STOP) */
    for (volatile int j = 0; j < 20; j++);
}

/* ---- DMA full-image send (1025 bytes: 0x40 + 1024 pixel) ---- */
static void OLED_DMA_SendImage(void)
{
    uint16_t len = 1025;

    I2C_DMACmd(I2C1, ENABLE);
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);

    /* Configure DMA before ADDR is cleared -> TXE fires DMA */
    DMA_Cmd(DMA1_Channel6, DISABLE);
    DMA1_Channel6->CMAR = (uint32_t)OLED_DMABuf;
    DMA1_Channel6->CNDTR = len;
    DMA_Cmd(DMA1_Channel6, ENABLE);

    /* Now ADDR is already cleared by I2C1_SendAddr; DMA auto-starts */
    /* Wait DMA complete */
    while (!DMA_GetFlagStatus(DMA1_FLAG_TC6));
    DMA_ClearFlag(DMA1_FLAG_TC6);
    DMA_Cmd(DMA1_Channel6, DISABLE);
    I2C_DMACmd(I2C1, DISABLE);

    while (!I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF));
    I2C1_Stop();
}

/* ---- Init HW I2C1 (PB8=SCL, PB9=SDA, 400kHz) + DMA1_Ch6 ---- */
static void OLED_HW_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* Step 1: GPIO as Out_OD, do bus recovery */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C1_BusRecovery();

    /* Step 2: Remap I2C1 to PB8/PB9, switch to AF_OD */
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Step 3: Enable clocks and reset I2C */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    I2C_DeInit(I2C1);

    /* Step 4: Configure I2C1 Fast Mode 400kHz */
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = 0x00;
    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed          = 400000;
    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);

    /* Step 5: DMA1 Channel6 for I2C1_TX */
    DMA_DeInit(DMA1_Channel6);
    DMA_InitTypeDef DMA_InitStructure;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&I2C1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)OLED_DMABuf;
    DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize         = 0;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &DMA_InitStructure);
}

/* ---- Public API ---- */

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    uint8_t cmds[] = { 0xB0 | Y, 0x10 | ((X & 0xF0) >> 4), 0x00 | (X & 0x0F) };
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    I2C1_SendByte(cmds[0]);
    I2C1_SendByte(cmds[1]);
    I2C1_SendByte(cmds[2]);
    I2C1_Stop();
}

void OLED_WriteCommand(uint8_t Command)
{
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    I2C1_SendByte(Command);
    I2C1_Stop();
}

void OLED_WriteData(uint8_t Data)
{
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x40);
    I2C1_SendByte(Data);
    I2C1_Stop();
}

void OLED_Clear(void)
{
    const uint8_t setup[] = { 0x20, 0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07 };
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    for (int i = 0; i < 8; i++) I2C1_SendByte(setup[i]);
    I2C1_Stop();

    OLED_DMABuf[0] = 0x40;
    for (uint16_t i = 0; i < 1024; i++) OLED_DMABuf[i + 1] = 0x00;
    OLED_DMA_SendImage();
}

void OLED_ShowImage(const uint8_t *Bitmap)
{
    const uint8_t setup[] = { 0x20, 0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07 };
    I2C1_Start();
    I2C1_SendAddr(OLED_ADDR);
    I2C1_SendByte(0x00);
    for (int i = 0; i < 8; i++) I2C1_SendByte(setup[i]);
    I2C1_Stop();

    OLED_DMABuf[0] = 0x40;
    for (uint16_t i = 0; i < 1024; i++) OLED_DMABuf[i + 1] = Bitmap[i];
    OLED_DMA_SendImage();
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (i = 0; i < 8; i++) OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (i = 0; i < 8; i++) OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
        OLED_ShowChar(Line, Column + i, String[i]);
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) Result *= X;
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0) { OLED_ShowChar(Line, Column, '+'); Number1 = Number; }
    else             { OLED_ShowChar(Line, Column, '-'); Number1 = -Number; }
    for (i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++) {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        OLED_ShowChar(Line, Column + i, SingleNumber < 10 ? SingleNumber + '0' : SingleNumber - 10 + 'A');
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
}

void OLED_Init(void)
{
    uint32_t i, j;
    for (i = 0; i < 3000; i++)
        for (j = 0; j < 1000; j++);

    OLED_HW_Init();

    /* Standard SSD1306 init sequence */
    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5); OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8); OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3); OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA); OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81); OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9); OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB); OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D); OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
}
