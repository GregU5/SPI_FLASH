//=============================================================================
//                       ##### SPI #####
//=============================================================================
// spi.c
#define __IN_SPI_C

/* Includes ------------------------------------------------------------------*/
#include "spi.h"

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private const -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototypes ----------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

// ustawienie pinów procesora
// pin PB12 = SPI_CS (programowy)
// pin PB13 = SPI2_SCK
// pin PB14 = SPI2_MISO
// pin PB15 = SPI2_MOSI
void
Spi2PinsSet(void)
{
	// POD£¥CZENIE ZEGARA DO GPIOB
	RCC_AHBPeriphClockCmd(RCC_AHBENR_GPIOBEN, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	// PB13 = SPI2_SCK
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);

	// PB14 = SPI2_MISO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);

	// PB15 = SPI2_MOSI
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);
}

//ustawienie interfejsu SPI
void
Spi2InterfaceSet(void)
{
	// POD£¥CZENIE ZEGARA DO SPI2
	RCC_APB1PeriphClockCmd(RCC_APB1ENR_SPI2EN, ENABLE);
	SPI_I2S_DeInit(SPI2);

	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);
    SPI_Cmd(SPI2, ENABLE);
}

// Wyslij bajt
unsigned char SpiSendByte(SPI_TypeDef *spi, unsigned char byte)
{
	unsigned char read_dummy_byte = 0xFF;

	SPI_I2S_SendData(spi, byte);
	while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET)
	{
		//timeout
	}

	while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET)
	{
		//timeout
	}
	read_dummy_byte = (unsigned char) SPI_I2S_ReceiveData(spi);

	return read_dummy_byte;
}

// Odczytaj bajt
unsigned char SpiReadByte(SPI_TypeDef *spi)
{
	unsigned char retval = 0;
	//czekaj az zostanie wyzerowa flaga zajetosci bufora, 0 - pe³ny bufor, 1 - pusty bufor
	while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_TXE) == RESET)
	{
		//timeout
	}
	// wyslij bajt
	SPI_I2S_SendData(spi, 0xFF);
	//czekaj na odbiór danych, jesli 0 - bufor pusty, jesli 1 to bufor zapisany i gotowy do odczytu
    while(SPI_I2S_GetFlagStatus(spi, SPI_I2S_FLAG_RXNE) == RESET)
    {
    	//timeout
    }
	//odczyt danych z rejestru
	retval = (unsigned char) SPI_I2S_ReceiveData(spi);

	return retval;
}

/* Private functions ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
//
//                PIERWSZA GRUPA FUNKCJI PRYWATNYCH
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//                DRUGA GRUPA FUNKCJI PRYWATNYCH
//
//-----------------------------------------------------------------------------

#undef __IN_SPI_C
