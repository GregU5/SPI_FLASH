//=============================================================================
//                       ##### SPI_FLASH #####
//=============================================================================
#define __IN_SPI_FLASH_C

/* Includes ------------------------------------------------------------------*/
#include "spi_flash.h"

#include <inttypes.h>

/* Global variables ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SPI_FLASH_SET_CS                       (GPIOB->BSRRL |= GPIO_BSRR_BS_12)
#define SPI_FLASH_RESET_CS                     (GPIOB->BSRRH |= GPIO_BSRR_BS_12)

/* DEVICE COMMANDS FOR SST25PF040C - instrukcje dla pamieci flash */
#define	SPI_FLASH_CMD_READ_CONT             0x03        /* READ CONTINOUSLY - ciagly odczyt pamiêci */
#define SPI_FLASH_CMD_SECTOR_ERASE          0x20        /* 1 SECTOR = 4kB - komenda czyszczenia sektora */
#define SPI_FLASH_CMD_BLOCK_ERASE           0xD8        /* 1 BLOCK = 64kB - komenda czyszczenia bloku */
#define SPI_FLASH_CMD_CHIP_ERASE            0xC7        /* FULL CHIP ERASE - wyczyszczenie calej pamieci */
#define SPI_FLASH_CMD_WRITE_EN              0x06        /* WRITE ENABLE - zezwolenie na zapis do pamieci */
#define	SPI_FLASH_CMD_PAGE_PROGRAM          0x02        /* 1 PAGE = 256B - programowanie stronami */
#define SPI_FLASH_CMD_READ_SSR              0x05        /* READ SOFTWARE STATUS REGISTER - odczyt status register */
#define SPI_FLASH_CMD_WRITE_STATUS_REG      0x01        /* WRITE/READ SOFTWARE STATUS REGISTER - zpais do status register */
#define SPI_FLASH_CMD_READ_ID               0xAB        /* READ DEVICE ID - odczyt id pamieci */
#define SPI_FLASH_CMD_JEDEC_ID              0x9F        /* READ JEDEC ID - odczyt jedec id */
#define SPI_FLASH_CMD_DEEP_POWER_DM         0xB9        /* DEEP POWER DOWN MODE - stan glebokiego uspienia */
#define SPI_DUMMY_DATA                      0xFF        /* DUMMY DATA FOR SYNCHRONIC TRANSMISSION */
/* Private const -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototypes ----------------------------------------------*/

/** czekaj az pamiec nie bedzie wykonywac operacji */
static uint32_t WaitForBusyFlag(void);
/** wyslij 24 bitowy adress */
static void prvSpiFlashSendAddress(uint32_t address_24bit);
/** sprawdz, czy jest mozliwy zapis do pamieci */
static uint8_t prvSpiFlashGetStatusReg(void);
static uint32_t	prvIsWriteEnable(void);
/** zezwolenie na zapis */
static void prvSpiFlashWriteEnable(void);


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

uint8_t SpiFlashGetID(void)
{
    uint8_t retval = 0;

    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_READ_ID);
    SpiSendByte(SPI2, SPI_DUMMY_DATA);
    SpiSendByte(SPI2, SPI_DUMMY_DATA);
    SpiSendByte(SPI2, SPI_DUMMY_DATA);

    retval = SpiReadByte(SPI2);
    SPI_FLASH_SET_CS;

    return retval;
}

void SpiFlashReadJedecID(jedec_t *xJedec)
{
    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_JEDEC_ID);

    xJedec->DevID = SpiReadByte(SPI2);
    xJedec->MemType = SpiReadByte(SPI2);
    xJedec->MemCap = SpiReadByte(SPI2);
    xJedec->ResCode = SpiReadByte(SPI2);

    SPI_FLASH_SET_CS;
}

uint8_t SpiFlashReadByte(uint32_t address)
{
    uint8_t byte = 0;
    if (address >= SPI_FLASH_SIZE)
    {
        /* podany adres jest poza przestrzenia adresowa */
        return 0xFF;
    }

    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_READ_CONT);   /* komenda odczytu ciagłego */
    prvSpiFlashSendAddress(address);              /* 24bit address */

    byte = SpiReadByte(SPI2);

    SPI_FLASH_SET_CS;

    return byte;
}

uint32_t SpiFlashIsWrited(uint32_t address)
{
    uint8_t data_read = 0;
    data_read = SpiFlashReadByte(address);

    if (data_read != 0xFF)
    {
        return 1;   /* flash zapisany */
    }

    return 0;       /* flash niezapisany */
}


int32_t SpiFlashReadData(struct SpiFlash *xFlash, uint8_t* dst)
{
    uint32_t i = 0; /* C89 style */
    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_READ_CONT); /* komenda odczytu ciagłego */
    prvSpiFlashSendAddress(xFlash->StartAddr);  /* 24bit adrress */
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

int32_t SpiFlashPageProg(uint32_t* address, uint32_t* num_of_bytes, uint8_t* src)
{
    uint32_t i = 0;

    if (*num_of_bytes > SPI_FLASH_PAGE_SIZE)
    {
        /* blad, nie mozna zapisac wiekszej ilosci bajtow niz jej rozmiar */
        return -1;
    }

    prvSpiFlashWriteEnable();
    /* czy jest mozliwy zapis do pamieci */
    if (prvIsWriteEnable() == 0)
    {
        return -2;
    }
    SPI_FLASH_RESET_CS;

    SpiSendByte(SPI2, SPI_FLASH_CMD_PAGE_PROGRAM);  /* rozkaz page program */
    prvSpiFlashSendAddress(*address);               /* przeslij 24 bitowy adres */
    for (; i < *num_of_bytes; i++)
    {
        SpiSendByte(SPI2, *(src + i));
        (*address)++;                               /* inkrementacja adresu */
    }

    while ((SPI2->SR & SPI_SR_BSY) == SPI_SR_BSY)
    {
        /* TODO: timeout */
    }

    SPI_FLASH_SET_CS;
    WaitForBusyFlag();

    return 1;
}

int32_t SpiFlashWriteByte(uint32_t address, uint8_t byte)
{
    uint32_t number_of_bytes = 1;

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

int32_t SpiFlashWriteData(struct SpiFlash *xFlash, uint8_t* src)
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

    uint32_t NumberOfBytesWrote = 0;
    uint32_t PagesToWrite = (xFlash->NumberOfBytes / SPI_FLASH_PAGE_SIZE);
    uint32_t LeftByteToWrite = (xFlash->NumberOfBytes % SPI_FLASH_PAGE_SIZE);
/**
 * Pamiec programuje sie stronami. Rozmiar strony to 256 bajtow, aby zaprogramowac wiecej niz 1 strone na raz, trzeba wywolac funkcje z kolejnym poczatkowym adresem strony.
 * Dlatego sprawdzamy, czy podany adres jest poczatkiem strony, jesli jest to adres jest wyrownany, jesli nie to trzeba sprawdzic, ile mozemy zapisac do konca strony.
 */
    uint32_t AddressIsNotAligned = xFlash->ActualAddr % SPI_FLASH_PAGE_SIZE; 	/* sprawdz, czy adres jest wyrownany */
    uint32_t page = 0;															/* aktualna liczba zapisanych stron */

    /* jesli adres jest wyrownany */
    if (AddressIsNotAligned == 0)
    {
        /* Rob dopoki liczba aktualnych zapisanych stron nie pokryje sie z iloscia do zapisania */
        while (page != PagesToWrite)
        {
            unsigned int bytes = SPI_FLASH_PAGE_SIZE;
            SpiFlashPageProg(&xFlash->ActualAddr, &bytes, src);     /* zapisz strone (256bajtow) */
            NumberOfBytesWrote += SPI_FLASH_PAGE_SIZE;              /* liczba zapisanych bajtów */
            page++;                                                 /* po kazdym zapisie inkrementuj liczbe zapisanych stron */
        }
        SpiFlashPageProg(&xFlash->ActualAddr, &LeftByteToWrite, src);   /* zapisz pozostale bajty */
        NumberOfBytesWrote += LeftByteToWrite;
    }
    /* jesli adres nie jest wyrownany, obliczamy ilosc bajtow do konca strony */
    else
    {
        uint32_t BytesToPageAddress = SPI_FLASH_PAGE_SIZE - AddressIsNotAligned;
        /* Sprawdz, czy liczba bajtow do zapisu jest wieksza od liczby wolnych bajtow do konca strony */
        /* jesli tak to zapisz wolne bajty do konca strony */
        if (xFlash->NumberOfBytes > BytesToPageAddress)
        {
            SpiFlashPageProg(&xFlash->ActualAddr, &BytesToPageAddress, src);
            /* Oblicz ile zostalo bajtow po wyrownaniu strony, bajty te zapiszemy na koncu, po zapisie stron */
            LeftByteToWrite = (xFlash->NumberOfBytes - BytesToPageAddress) % SPI_FLASH_PAGE_SIZE;
            NumberOfBytesWrote += BytesToPageAddress;

            /* po zapisaniu wolnych bajtow strony, odejmij jedna strone, gdyz ilosc stron do zapisnaia zmniejszy sie o 1 */
            if (PagesToWrite != 0)
            {
                PagesToWrite--;
            }
        }
        /* Zapisz pozostale strony */
        while (page != PagesToWrite)
        {
            uint32_t bytes = SPI_FLASH_PAGE_SIZE;
            SpiFlashPageProg(&xFlash->ActualAddr, &bytes, src);
            NumberOfBytesWrote += SPI_FLASH_PAGE_SIZE;
            page++;
        }

        /* zapisanie pozostalych bajtow */
        SpiFlashPageProg(&xFlash->ActualAddr, &LeftByteToWrite, src);
        NumberOfBytesWrote += LeftByteToWrite;
    }

    return 1;
}

int32_t SpiFlashSectorErase(uint32_t address)
{
    prvSpiFlashWriteEnable();

    /* czy jest mozliwy zapis do pamieci */
    if (prvIsWriteEnable() == 0)
    {
        /* CANT SET WRITE ENABLE FLAG */
        return -1;
    }

    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_SECTOR_ERASE);
    prvSpiFlashSendAddress(address);
    SPI_FLASH_SET_CS;

    WaitForBusyFlag();      /* czekaj az flash zakonczy operacje */

    return 1;
}

int32_t SpiFlashNumSectorErase(uint8_t number_of_sector)
{
    if (number_of_sector > 127)
    {
        /* NUMBER OF SECTORS IS OUT OF RANGE */
        return 0;
    }

    uint32_t address = 0;
    /* wyliczenie poczatku adresu dla danego sektora */
    address = (uint32_t) number_of_sector * SPI_FLASH_SECTOR_SIZE;
    SpiFlashSectorErase(address);

    return 1;
}

int32_t SpiFlashBlockErase(uint32_t address)
{
    prvSpiFlashWriteEnable();

    /* sprawdz czy jest mozliwy zapis do pamieci */
    if (prvIsWriteEnable() == 0)
    {
        /* CANT SET WRITE ENABLE FLAG */
        return -1;
    }

    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_BLOCK_ERASE);
    prvSpiFlashSendAddress(address);
    SPI_FLASH_SET_CS;

    WaitForBusyFlag();

    return 1;
}

int32_t SpiFlashBlockNumErase(uint8_t number_of_block)
{
    uint32_t address = 0;
    if (number_of_block > 7)
    {
        return 0;
    }
    address = number_of_block * SPI_FLASH_BLOCK_SIZE;
    SpiFlashBlockErase(address);

    return 1;
}


int32_t SpiFlashFullErase(void)
{
    prvSpiFlashWriteEnable();
    if (prvIsWriteEnable() == 0)
    {
        /* CANT SET WRITE ENABLE FLAG */
        return -1;
    }

    SPI_FLASH_RESET_CS;
    SpiSendByte(SPI2, SPI_FLASH_CMD_CHIP_ERASE);
    SPI_FLASH_SET_CS;

    WaitForBusyFlag();

    return 1;
}

/* jeszcze nie wiem, czy beda prywatne - wyjdzie w trakcie pisania bootloadera */
int32_t SpiFlashWriteSR(uint8_t set_STATUS_REG)
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
static uint32_t	WaitForBusyFlag(void)
{
    /* czekaj az pamiec flash nie bedzie wykonywac operacji */
    while ( (prvSpiFlashGetStatusReg() & SPI_FLASH_SSR_BUSY_BIT) == SPI_FLASH_SSR_BUSY_BIT )
    {
        /* odczytaj status register pamieci flash - rejestr jest 8 bitowy */
        prvSpiFlashGetStatusReg();
        /*TODO: TIMEOUT */
    }
    return 1;
}

static void prvSpiFlashSendAddress(uint32_t address_24bit)
{
    uint8_t temp_address = 0;

    temp_address = (uint8_t) ((address_24bit & 0xFF0000) >> 16);
    SpiSendByte(SPI2, temp_address); //1
    temp_address = (uint8_t) ((address_24bit & 0xFF00) >> 8);
    SpiSendByte(SPI2, temp_address); //2
    temp_address = (uint8_t) (address_24bit & 0xFF);
    SpiSendByte(SPI2, temp_address); //3
}

static uint32_t prvIsWriteEnable(void)
{
    uint8_t sr = 0;
    sr = prvSpiFlashGetStatusReg();
    if ((sr & SPI_FLASH_SSR_WEL_BIT) == SPI_FLASH_SSR_WEL_BIT)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint8_t prvSpiFlashGetStatusReg(void)
{
    uint8_t data = 0;
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
