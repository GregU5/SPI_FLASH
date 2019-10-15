
//Ustawienie portów i inicjalizacja interfejsu SPI2

#ifndef __SPI_H
#define __SPI_H

#include "stm32L1xx.h"

/* Exported define -----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/

#ifndef __IN_SPI_C

extern unsigned int TEMPLATE_INFORMATION_FLAGS;
extern unsigned int TEMPLATE_CONTROL_FLAGS;

#endif

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/* PORT PINS CONFIGURATION FOR SPI MODE */
void Spi2PinsSet(void);
/* SPI CONFIGURATION */
void Spi2InterfaceSet(void);

unsigned char SpiSendByte(SPI_TypeDef *spi, unsigned char byte);
unsigned char SpiReadByte(SPI_TypeDef *spi);

#endif /* __SPI_H_ */
