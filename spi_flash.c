//=============================================================================
//                       ##### SPI_FLASH #####
//=============================================================================
#define __IN_SPI_FLASH_C

/* Includes ------------------------------------------------------------------*/
#include <spi_flash.h>

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SPI_FLASH_SET_CS							(GPIOB->BSRRL |= GPIO_BSRR_BS_12)
#define SPI_FLASH_RESET_CS							(GPIOB->BSRRH |= GPIO_BSRR_BS_12)


//	DEVICE COMMANDS FOR SST25PF040C - instrukcje dla pamieci flash
#define	SPI_FLASH_CMD_READ_CONT						0x03 						// READ CONTINOUSLY - ci¹g³y odczyt pamiêci
#define SPI_FLASH_CMD_SECTOR_ERASE 					0x20 						// 1 SECTOR = 4kB - komenda czyszczenia sektora
#define SPI_FLASH_CMD_BLOCK_ERASE					0xD8						// 1 BLOCK = 64kB - komenda czyszczenia bloku
#define SPI_FLASH_CMD_CHIP_ERASE					0xC7						// FULL CHIP ERASE - wyczyszczenie calej pamieci
#define SPI_FLASH_CMD_WRITE_EN						0x06						// WRITE ENABLE - zezwolenie na zapis do pamieci
#define	SPI_FLASH_CMD_PAGE_PROGRAM					0x02						// 1 PAGE = 256B - programowanie stronami
#define SPI_FLASH_CMD_READ_SSR						0x05						// READ SOFTWARE STATUS REGISTER - odczyt status register
#define SPI_FLASH_CMD_WRITE_STATUS_REG					0x01						// WRITE/READ SOFTWARE STATUS REGISTER - zpais do status register
#define SPI_FLASH_CMD_READ_ID						0xAB						// READ DEVICE ID - odczyt id pamieci
#define SPI_FLASH_CMD_JEDEC_ID						0x9F						// READ JEDEC ID - odczyt jedec id
#define SPI_FLASH_CMD_DEEP_POWER_DM					0xB9						// DEEP POWER DOWN MODE - stan glebokiego uspienia
#define SPI_DUMMY_DATA							0xFF 						// DUMMY DATA FOR SYNCHRONIC TRANSMISSION

/* Private const -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototypes ----------------------------------------------*/
static unsigned int WaitForBusyFlag(void);									// czekaj az pamiec nie bedzie wykonywac operacji
static void SpiFlashSendAddress(unsigned int address_24bit);							// wyslij 24 bitowy adress
static unsigned char prvSpiFlashGetStatusReg(void);								// zwraca stan rejestru Status Register
static unsigned int	prvIsWriteEnable(void);									// sprawdz, czy jest mozliwy zapis do pamieci
static void prvSpiFlashWriteEnable(void);									// zezwolenie na zapis


/* Public functions ----------------------------------------------------------*/
void SpiFlashInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    Spi2PinsSet();

	/* PB12 - CS */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB, GPIO_Pin_12);

    /* Podpiecie zegara do GPIOD */
    RCC_AHBPeriphClockCmd(RCC_AHBENR_GPIODEN, ENABLE);
    /* PD8 - WP */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_SetBits(GPIOD, GPIO_Pin_8);

    /* HADWARE CONFIGURATION OF PORT AND INIT DEVICE*/
    Spi2InterfaceSet();
    SPI_Cmd(SPI2, ENABLE);
}

unsigned char SpiFlashGetID(void)
{
	unsigned char retval;
	SPI_FLASH_SET_CS;
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_READ_ID);
	SpiSendByte(SPI2, SPI_DUMMY_DATA);
	SpiSendByte(SPI2, SPI_DUMMY_DATA);
	SpiSendByte(SPI2, SPI_DUMMY_DATA);

	retval = SpiReadByte(SPI2);
	SPI_FLASH_SET_CS;

	return retval;
}

void SpiFlashReadJedecID(JEDEC *xJedec)
{
	SPI_FLASH_SET_CS;
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_JEDEC_ID);

	xJedec->DevID = SpiReadByte(SPI2);
	xJedec->MemType = SpiReadByte(SPI2);
	xJedec->MemCap = SpiReadByte(SPI2);
	xJedec->ResCode = SpiReadByte(SPI2);

	SPI_FLASH_SET_CS;
}

unsigned char SpiFlashReadByte(unsigned int address)
{
	unsigned char byte = 0;
	if (address >= SPI_FLASH_SIZE)
	{
		/* podany adres jest poza przestrzenia adresowa */
		return 0xFF;
	}

	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_READ_CONT);					// komenda odczytu ciag³ego
	SpiFlashSendAddress(address);								// 24bit adrress

	byte = SpiReadByte(SPI2);

	SPI_FLASH_SET_CS;

	return byte;
}

unsigned int SpiFlashIsWrited(unsigned int address)
{
	unsigned char data_read = 0;
	data_read = SpiFlashReadByte(address);

	if(data_read != 0xFF)
	{
		return 1; 												// flash zapisany
	}

	return 0;													// flash niezapisany
}


int SpiFlashReadData(struct SpiFlash *xFlash, unsigned char *dst)
{
	unsigned int i = 0;
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_READ_CONT);					// komenda odczytu ciag³ego
	SpiFlashSendAddress(xFlash->StartAddr);						// 24bit adrress
	xFlash->ActualAddr = xFlash->StartAddr;

	for (; i < xFlash->NumberOfBytes; i++)
	{
		*(dst + i) = SpiReadByte(SPI2);
		xFlash->ActualAddr++;
	}
	SPI_FLASH_SET_CS;
	WaitForBusyFlag();

	return 1;
}

int SpiFlashPageProg(unsigned int *address, unsigned int *num_of_bytes, unsigned char *src)
{
	unsigned int i = 0;

	if (*num_of_bytes > SPI_FLASH_PAGE_SIZE)
	{
		//blad, nie mozna zapisac wiekszej ilosci bajtow niz jej rozmiar
		return -1;
	}

	prvSpiFlashWriteEnable();
	//czy jest mozliwy zapis do pamieci
	if (prvIsWriteEnable() == 0)
	{
		return -2;
	}
	SPI_FLASH_RESET_CS;

	SpiSendByte(SPI2, SPI_FLASH_CMD_PAGE_PROGRAM);	//rozkaz page program
	SpiFlashSendAddress(*address); 				//przeslij 24 bitowy adres
	for (; i < *num_of_bytes; i++)
	{
		SpiSendByte(SPI2, *(src + i));
		(*address)++;								//inkrementacja adresu
	}

	while((SPI2->SR & SPI_SR_BSY) == SPI_SR_BSY)
	{
		//todo: timeout
	}

	SPI_FLASH_SET_CS;
	WaitForBusyFlag();

	return 1;
}

int SpiFlashWriteByte(unsigned int address, unsigned char byte)
{
	unsigned int number_of_bytes = 1;

/*	if (SpiFlashIsDataIn(address) == 1)
	{
		return -1;
	}*/

	if (address >= SPI_FLASH_SIZE)
	{
		/* podany adres jest poza przestrzenia adresowa */
		return -2;
	}

	SpiFlashPageProg(&address, &number_of_bytes, &byte);

	return 1;
}

int SpiFlashWriteData(struct SpiFlash *xFlash, unsigned char *src)
{
/*	if (SpiFlashIsDataIn(xFlash->StartAddr) == 1)
	{
		return -1;
	}*/

	if (xFlash->StartAddr >= SPI_FLASH_SIZE)
	{
		/* podany adres jest poza przestrzenia adresowa */
		return -2;
	}

	xFlash->ActualAddr = xFlash->StartAddr;

	unsigned int NumberOfBytesWrote = 0;
	unsigned int PagesToWrite = (xFlash->NumberOfBytes / SPI_FLASH_PAGE_SIZE);
	unsigned int LeftByteToWrite = (xFlash->NumberOfBytes % SPI_FLASH_PAGE_SIZE);
/**
 * Pamiec programuje sie stronami. Rozmiar strony to 256 bajtow, aby zaprogramowac wiecej niz 1 strone na raz, trzeba wywolac funkcje z kolejnym poczatkowym adresem strony.
 * Dlatego sprawdzamy, czy podany adres jest poczatkiem strony, jesli jest to adres jest wyrownany, jesli nie to trzeba sprawdzic, ile mozemy zapisac do konca strony.
 */
	unsigned int AddressIsNotAligned = xFlash->ActualAddr % SPI_FLASH_PAGE_SIZE; 	//sprawdz, czy adres jest wyrownany
	unsigned int page = 0;															//aktualna liczba zapisanych stron

	// jesli adres jest wyrownany
	if (AddressIsNotAligned == 0)
	{
		//Rob dopoki liczba aktualnych zapisanych stron nie pokryje sie z iloscia do zapisania
		while (page != PagesToWrite)
		{
			unsigned int bytes = SPI_FLASH_PAGE_SIZE;
			SpiFlashPageProg(&xFlash->ActualAddr, &bytes, src);						//zapisz strone (256bajtow)
			NumberOfBytesWrote += SPI_FLASH_PAGE_SIZE;								//liczba zapisanych bajtów
			page++;																	//po kazdym zapisie inkrementuj liczbe zapisanych stron
		}
		SpiFlashPageProg(&xFlash->ActualAddr, &LeftByteToWrite, src);				//zapisz pozostale bajty
		NumberOfBytesWrote += LeftByteToWrite;
	}
	//jesli adres nie jest wyrownany, obliczamy ilosc bajtow do konca strony
	else
	{
		unsigned int BytesToPageAddress = SPI_FLASH_PAGE_SIZE - AddressIsNotAligned;
		//Sprawdz, czy liczba bajtow do zapisu jest wieksza od liczby wolnych bajtow do konca strony,
		//jesli tak to zapisz wolne bajty do konca strony
		if (xFlash->NumberOfBytes > BytesToPageAddress)
		{
			SpiFlashPageProg(&xFlash->ActualAddr, &BytesToPageAddress, src);
			//Oblicz ile zostalo bajtow po wyrownaniu strony, bajty te zapiszemy na koncu, po zapisie stron
			LeftByteToWrite = (xFlash->NumberOfBytes - BytesToPageAddress) % SPI_FLASH_PAGE_SIZE;
			NumberOfBytesWrote += BytesToPageAddress;

			//po zapisaniu wolnych bajtow strony, odejmij jedna strone, gdyz ilosc stron do zapisnaia zmniejszy sie o 1
			if (PagesToWrite != 0)
			{
				PagesToWrite--;
			}
		}
		//Zapisz pozostale strony
		while (page != PagesToWrite)
		{
			unsigned int bytes = SPI_FLASH_PAGE_SIZE;
			SpiFlashPageProg(&xFlash->ActualAddr, &bytes, src);
			NumberOfBytesWrote += SPI_FLASH_PAGE_SIZE;
			page++;
		}

		//zapisanie pozostalych bajtow
		SpiFlashPageProg(&xFlash->ActualAddr, &LeftByteToWrite, src);
		NumberOfBytesWrote += LeftByteToWrite;
	}

	return 1;
}

int SpiFlashSectorErase(unsigned int address)
{
	prvSpiFlashWriteEnable();

	//czy jest mozliwy zapis do pamieci
	if (prvIsWriteEnable() == 0)
	{
		/*CANT SET WRITE ENABLE FLAG */
		return -1;
	}

	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_SECTOR_ERASE);
	SpiFlashSendAddress(address);
	SPI_FLASH_SET_CS;

	WaitForBusyFlag();						//czekaj az flash zakonczy operacje

	return 1;
}

int SpiFlashNumSectorErase(unsigned char number_of_sector)
{
	if (number_of_sector > 127)
	{
		/* NUMBER OF SECTORS IS OUT OF RANGE */
		return 0;
	}

	unsigned int address = 0;
	//wyliczenie poczatku adresu dla danego sektora
	address = (unsigned int) number_of_sector * SPI_FLASH_SECTOR_SIZE;
	SpiFlashSectorErase(address);

	return 1;
}

int SpiFlashBlockErase(unsigned int address)
{
	prvSpiFlashWriteEnable();

	//sprawdz czy jest mozliwy zapis do pamieci
	if (prvIsWriteEnable() == 0)
	{
		/*CANT SET WRITE ENABLE FLAG */
		return -1;
	}

	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_BLOCK_ERASE);
	SpiFlashSendAddress(address);
	SPI_FLASH_SET_CS;

	WaitForBusyFlag();

	return 1;
}

int SpiFlashBlockNumErase(unsigned char number_of_block)
{
	unsigned int address = 0;
	if (number_of_block > 7)
	{
		return 0;
	}
	address = number_of_block * SPI_FLASH_BLOCK_SIZE;
	SpiFlashBlockErase(address);

	return 1;
}


int SpiFlashFullErase(void)
{
	prvSpiFlashWriteEnable();
	if (prvIsWriteEnable() == 0)
	{
		/*CANT SET WRITE ENABLE FLAG */
		return -1;
	}

	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_CHIP_ERASE);
	SPI_FLASH_SET_CS;

	WaitForBusyFlag();

	return 1;
}

// jeszcze nie wiem, czy beda prywatne - wyjdzie w trakcie pisania bootloadera
int SpiFlashWriteSR(unsigned char set_STATUS_REG)
{
	prvSpiFlashWriteEnable();
	if (prvIsWriteEnable() == 0)
	{
		/*CANT SET WRITE ENABLE FLAG */
		return -1;
	}

	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_WRITE_STATUS_REG);
	SpiSendByte(SPI2, set_STATUS_REG);
	SPI_FLASH_SET_CS;

	WaitForBusyFlag();

	return 1;
}

void SpiFlashWriteDisable(void)
{
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_WRITE_EN);
	SPI_FLASH_SET_CS;
}

/* Private functions ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
//
//                PIERWSZA GRUPA FUNKCJI PRYWATNYCH
//
//-----------------------------------------------------------------------------
static unsigned int	WaitForBusyFlag(void)
{
	/* czekaj az pamiec flash nie bedzie wykonywac operacji */
	while ( (prvSpiFlashGetStatusReg() & SPI_FLASH_SSR_BUSY_BIT) == SPI_FLASH_SSR_BUSY_BIT )
	{
		//odczytaj status register pamieci flash - rejestr jest 8 bitowy
		prvSpiFlashGetStatusReg();
		/*TODO: TIMEOUT */
	}
	return 1;
}

static void SpiFlashSendAddress(unsigned int address_24bit)
{
	unsigned char temp_address = 0;

	temp_address = (unsigned char) ( (address_24bit & 0xFF0000) >> 16);
	SpiSendByte(SPI2, temp_address); //1
	temp_address = (unsigned char) ( (address_24bit & 0xFF00) >> 8);
	SpiSendByte(SPI2, temp_address); //2
	temp_address = (unsigned char) (address_24bit & 0xFF);
	SpiSendByte(SPI2, temp_address); //3
}

static unsigned int prvIsWriteEnable(void)
{
	unsigned char sr = 0;
	sr = prvSpiFlashGetStatusReg();
	if ( ( sr & SPI_FLASH_SSR_WEL_BIT ) == SPI_FLASH_SSR_WEL_BIT )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static unsigned char prvSpiFlashGetStatusReg(void)
{
	char data;
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_READ_SSR);
	data = SpiReadByte(SPI2);
	SPI_FLASH_SET_CS;

	return data;
}
//-----------------------------------------------------------------------------
//
//                DRUGA GRUPA FUNKCJI PRYWATNYCH
//
//-----------------------------------------------------------------------------

void prvSpiFlashWriteEnable(void)
{
	SPI_FLASH_RESET_CS;
	SpiSendByte(SPI2, SPI_FLASH_CMD_WRITE_EN);
	SPI_FLASH_SET_CS;
}


/* --------------------------------------------------------------------------------------------------------------- */

//koniec bariery redefinicji public/extern
#undef __IN_SPI_FLASH_C
