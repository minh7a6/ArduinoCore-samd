#include "SERCOM.h"


SERCOM::SERCOM(Sercom* sercom)
{
	this->sercom = sercom;
	sercomUart = sercom->UART;
	sercomSpi = sercom->SPI;
}

/* 	=========================
 *	===== Sercom UART
 *	=========================
*/
void SERCOM::initUART(SercomUartMode mode, SercomUartSampleRate sampleRate, uint32_t baudrate)
{
	resetUART();
	
	//Setting the CTRLA register
	sercom->USART.CTRLA.reg =	SERCOM_UART_CTRLA_MODE(mode) |
								SERCOM_USART_CTRLA_SAMPR(sampleRate);

	//
	if(mode == UART_INT_CLOCK)
	{
		uint32_t sampleRateValue;	
		
		if(sampleRate == SAMPLE_RATE_x16)
			sampleRateValue = 16;
		else if(sampleRate == SAMPLE_RATE_x8)
			sampleRateValue = 8;
		else
			sampleRateValue = 3;
		
		//Asynchronous arithmetic mode
		sercom->USART.BAUD.reg = 65535 * ( 1 - sampleRateValue * (baudrate / SERCOM_FREQ_REF));
	}
}
void initFrame(SercomUartCharSize charSize, SercomDataOrder dataOrder, SercomParityMode parityMode, SercomNumberStopBit nbStopBits)
{
	//Setting the CTRLA register
	sercom->USART.CTRLA.reg |=	SERCOM_UART_CTRLA_FORM( (parityMode == NO_PARITY ? 0 : 1) ) |
								dataOrder << SERCOM_UART_CTRLA_DORD_Pos;

	//Setting the CRTLB register
	sercom->USART.CTRLB.reg |=	SERCOM_UART_CTRLB_CHSIZE(charSize) |
								nbStopBits << SERCOM_UART_CTRLB_SBMODE_Pos |
								(parityMode == NO_PARITY ? 0 : parityMode) << SERCOM_UART_CTRLB_PMODE_Pos; //If no parity use default value
}

void initPads(SercomUartTXPad txPad, SercomRXPad rxPad)
{
	//Setting the CTRLA register
	sercom->USART.CTRLA.reg |=	SERCOM_UART_CTRLA_TXPO(txPad) |
								SERCOM_UART_CTRLA_RXPO(rxPad);

	//Setting the CRTLB register (Enabling Transceiver and Receiver)
	sercom->USART.CTRLB.reg |=	SERCOM_UART_CTRLB_TXEN |
								SERCOM_UART_CTRLB_RXEN;
}

void SERCOM::resetUART()
{
	//Setting  the Software bit to 1
	sercom->USART.CTRLA.bit.SWRST = 0x1u;
	
	//Wait for both bits Software Reset from CTRLA and SYNCBUSY are equal to 0
	while(sercom->USART.CTRLA.bit.SWRST || sercom->USART.SYNCBUSY.SWRST);
}

void SERCOM::enableUART()
{
	//Setting  the enable bit to 1
	sercom->USART.CTRLA.bit.ENABLE = 0x1u;
	
	//Wait for then enable bit from SYNCBUSY is equal to 0;
	while(sercom->USART.SYNCBUSY.bit.ENABLE);
}

void SERCOM::flushUART()
{
	// Wait for transmission to complete
	while(sercom->USART.INTFLAG.bit.DRE != SERCOM_USART_INTFLAG_DRE);
}

void SERCOM::clearStatusUART()
{
	//Reset (with 0) the STATUS register
	sercom->USART.STATUS.reg = SERCOM_USART_STATUS_RESETVALUE;
}

bool SERCOM::availableDataUART()
{
	return sercom->USART.INTFLAG.bit.RXC;
}

bool SERCOM::isBufferOverflowErrorUART()
{
	return sercom->USART.STATUS.bit.BUFOVF;
}

bool SERCOM::isFrameErrorUART()
{
	return sercom->USART.STATUS.bit.FERR;
}

bool SERCOM::isParityErrorUART()
{
	return sercom->USART.STATUS.bit.PERR;
}

uint8_t SERCOM::readDataUART()
{
	return sercom->USART.DATA.bit.DATA;
}

int SERCOM::writeDataUART(uint8_t data)
{
	//Flush UART buffer
	flushUART();

	//Put data into DATA register
	sercom->USART.DATA.bit.DATA = data;
	return 1;
}

/*	=========================
 *	===== Sercom SPI
 *	=========================
*/
void SERCOM::initSPI(SercomSpiTXPad mosi, SercomRXPad miso, SercomSpiCharSize charSize, SercomDataOrder dataOrder)
{
	resetSPI();
	
	//Setting the CTRLA register
	sercom->SPI.CTRLA.reg =	SERCOM_SPI_CTRLA_MODE(SPI_MASTER_OPERATION) |
							SERCOM_SPI_CTRLA_DOPO(mosi) |
							SERCOM_SPI_CTRLA_DIPO(miso) |
							dataOrder << SERCOM_SPI_CTRLA_DORD_Pos;
	
	//Setting the CTRLB register
	sercom->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_CHSIZE(charSize) |
							(0x1ul) << SERCOM_SPI_CTRLB_RXEN_Pos;	//Active the SPI receiver.
}

void SERCOM::initClock(SercomSpiClockMode clockMode, uint32_t baudrate)
{
	int cpha, cpol;
	
	if((clockMode & (0x1ul)) == 0 )	
		cpha = 0;
	else
		cpha = 1;
		
	if((clockMode & (0x2ul)) == 0)
		cpol = 0;
	else
		cpol = 1;
		
	//Setting the CTRLA register
	sercom->SPI.CTRLA.reg |=	cpha << SERCOM_SPI_CTRLA_CPHA_Pos |
								cpol << SERCOM_SPI_CTRLA_CPOL_Pos;
	
	//Synchronous arithmetic
	sercom->SPI.BAUD.reg = SERCOM_FREQ_REF / (2 * baudrate) - 1;
}

void SERCOM::resetSPI()
{
	//Set the Software bit to 1
	sercom->SPI.CTRLA.bit.SWRST = 0x1u;

	//Wait both bits Software Reset from CTRLA and SYNCBUSY are equal to 0
	while(sercom->SPI.CTRLA.bit.SWRST || sercom->SPI.SYNCBUSY.SWRST);
}
	
void SERCOM::enableSPI()
{
	//Set the enable bit to 1
	sercom->SPI.CTRLA.bit.ENABLE = 0x1ul;
	
	//Waiting then enable bit from SYNCBUSY is equal to 0;
	while(sercom->SPI.SYNCBUSY.bit.ENABLE);
}

bool SERCOM::isBufferOverflowErrorSPI()
{
	return sercom->SPI.STATUS.bit.BUFOVF;
}
