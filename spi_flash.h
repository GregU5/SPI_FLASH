// Biblioteka do obslugi pamieci flash --> SST25PF040C, udostepnia funkcje do sekwencyjnego zapisu/odczytu lub pojedynczych bajtow.
// Do zapisu/odczytu sekwencyjnego uzywa sie struktury, ktora trzeba zainicjalizowac, a nastepnie przekazac przez wskaznik do funkcji.
// Struktura przechowuje informacje o adresie startowym, aktualnym i liczbie bajtow do zapisu.
// Nie ma koniecznosci przejmowac sie sterowaniem linia CS lub czy podczas zapisu nie dochodzimy do konca strony i zapis sie nie zakonczy na aktualnej stronie.
// Wszystko realizuja za nas funkcje biblioteczne. Wystarczy podac adres i liczbe bajtow do zapisu.
// Kasowanie pamieci jest mozliwe poprzez dwa rodzaje funkcji. W jednym przypadku podajemy adres, a drugim numer sektora lub bloku.
#ifndef __SPI_FLASH_H_
#define __SPI_FLASH_H_

#include "spi.h"

/* Exported define -----------------------------------------------------------*/
//	SOFTWARE STATUS REGISTER - READ/WRITE BITS - flagi STATUS REGISTER do okreslenia stanu pamieci
#define SPI_FLASH_SSR_BUSY_BIT						0x01						// READ - flaga zajętosci, czy pamiec jest w trakcie wykonywania jakiejs operacji
#define SPI_FLASH_SSR_WEL_BIT						0x02						// READ - flaga zapisu, ustawiona sygnalizuje mozliwosc zapisu do pamieci
#define SPI_FLASH_SSR_BP0_BIT						0x04						// READ/WRITE - poniższe bity sluzą do ochrony przed zapisem, strona 9 w dokumentacji
#define	SPI_FLASH_SSR_BP1_BIT						0x08						// READ/WRITE
#define	SPI_FLASH_SSR_BP2_BIT						0x10						// READ/WRITE
#define SPI_FLASH_SSR_TB_BIT						0x20  						// READ/WRITE
#define SPI_FLASH_SSR_RES_BIT						0x40						// N/A
#define SPI_FLASH_SSR_BPL_BIT						0x80						// READ

//	FLASH BLOCKS BEGIN ADDRESS - adresy blokow w zewnetrznej pamieci flash
#define SPI_FLASH_BLOCK_0							0x000000U
#define SPI_FLASH_BLOCK_1							0x010000U
#define SPI_FLASH_BLOCK_2							0x020000U
#define SPI_FLASH_BLOCK_3							0x030000U
#define SPI_FLASH_BLOCK_4							0x040000U
#define SPI_FLASH_BLOCK_5							0x050000U
#define SPI_FLASH_BLOCK_6							0x060000U
#define SPI_FLASH_BLOCK_7							0x070000U

// BASE SIZES FOR PAGE, SECTOR AND BLOCK TO ADRRESS COUNT - rozmiary bloku, sektora i strony
#define SPI_FLASH_SIZE								0x080000U				//rozmiar pamieci	- 4Mbit 	(512kB)
#define SPI_FLASH_BLOCK_SIZE							0x010000U				//rozmiar bloku 	- 512kbit 	(64kB)
#define SPI_FLASH_SECTOR_SIZE							0x001000U				//rozmiar sektora	- 32kbit 	(4kB)
#define SPI_FLASH_PAGE_SIZE							0x000100U				//rozmiar strony	- 2kbit		(256B)
/* Exported types ------------------------------------------------------------*/

//	STRUCT FOR JEDEC-ID
//	Struktura do odczytu JEDEC-ID
typedef struct {
	char DevID;		//DEVICE ID
	char MemType;		//MEMORY TYPES
	char MemCap;		//MEMORY CAPACITY
	char ResCode;		//RESERVED CODE
} JEDEC;

//	Struktura do zapisu i odczytu pamieci flash,
//	przechowuje poczatkowy adres, ostatni adres i liczbe bajtow do odczytania/skopiowania
struct SpiFlash {
	unsigned int StartAddr;
	unsigned int ActualAddr;
	unsigned int NumberOfBytes;
	uint8_t * buff;
};

/* Exported variables --------------------------------------------------------*/
#ifndef __IN_SPI_FLASH_C

#endif

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/

// Konfiguracja portów i inicjalizacja interfejsu do obsługi pamięci flash
void SpiFlashInit(void);
void SpiFlashDeinit(void);

// Odczyt ID pamieci flash, funkcja zwraca 8 bitowa wartosc ID pamieci
unsigned char SpiFlashGetID(void);

// Odczyt JEDEC ID przez argument funkcji, ktory jest wskaznikiem na strukture, (zapis do struktury)
void SpiFlashReadJedecID(JEDEC *xJedec);

// Funkcja zapisuje SpiFlashPageProg do pamieci Flash stronami, maksymalna ilosc bajtow na strone to 256. Np. zapisanie 2 stron wymaga 2 krotnego wywolania.
// 1 argument: wskaznik do adresu
// 2 argument: liczba bajtów do zapisu(nie wieksza niz 256)
// 3 argument: źródło
int SpiFlashPageProg(unsigned int *address, unsigned int *num_of_bytes, unsigned char *src);

// Funkcja SpiFlashReadByte odczytuje jeden bajt z pamięci: Jako argument przyjmuje 24 bitowy adres
// Zwaraca: bajt spod adresu pamieci
unsigned char SpiFlashReadByte(unsigned int address);

//  Funkcja SpiFlashIsDataIn sprawdza, czy pod danym adresem pamieci sa dane, jesli pamiec jest zapisana zwroci 1, jesli nie to 0
unsigned int SpiFlashIsWrited(unsigned int address);

// Funkcja SpiFlashReadData odczytuje dane z pamięci Flash. 2 argumenty wywołania
// 1 argument: xFlash jest wskaznik na strukture z adresem i liczba bajtów do odczytu - musi byc zdefiniowany wczesniej
// 2 argument: dst jest wskaznikiem na bufor do ktorego beda zapisane odczytane dane z pamieci Flash
// Zwracajac 1 informuje o skonczonym odczycie danych
int SpiFlashReadData(struct SpiFlash *xFlash, unsigned char *dst);

// Funkcja SpiFlashWriteByte zapisuje 1 bajt do pamieci pod wskazany adres
// Zwraca:
//  1: udalo sie zapisac
// -1: pamiec jest zapisana w tym miejscu
// -2: poza obszarem adresowym pamieci
int SpiFlashWriteByte(unsigned int address, unsigned char byte);

// Funkcja SpiFlashWriteData zapisuje dane do pamięci Flash. 2 argumenty wywołania
// 1 argument: xFlash jest wskaznikiem do struktury z adresem i liczbą bajtów do zapisu - struktura musi byc zdefiniowania wczesniej
// 2 argument: src jest wskaznikiem na bufor z ktorego funkcja odczyta i zapisze dane do Flash.
// Funkcja zwraca:
// -2: podany adres jest poza dostepna przestrzenia adresowa
// -1: blad zapisu, pod adresem istnieja juz dane, nie mozna ich nadpisac bez wczesniejszego wyczyszczenia
//  1: zapis zakonczony sukcesem
int SpiFlashWriteData(struct SpiFlash *xFlash, unsigned char *src);

// Czyszczenie sektora pamieci, jako argument adres sektora
int SpiFlashSectorErase(unsigned int address);

// Czysczenie sektora pamieci, wystarczy podac numer sektora
int SpiFlashNumSectorErase(unsigned char number_of_sector);

// Czyszczenie bloku pamieci, jako argument adres bloku
int SpiFlashBlockErase(unsigned int address);

// Czyszczenie bloku pamieci, wystarczy podac numer bloku
int SpiFlashBlockNumErase(unsigned char number_of_block);

// Calkowite wyczyszczenie flasha
int SpiFlashFullErase(void);

int SpiFlashWriteSR(unsigned char set_STATUS_REG);								// zapisuje do Status Register
void SpiFlashWriteDisable(void);										// zablokowanie zapisu


#endif /* __SPI_FLASH_H_ */
