/*
 * Rs485Bus.h
 *
 * Singleton wrapper for sending data through the configured 485 UART.
 */

#ifndef _UART_RS485BUS_H_
#define _UART_RS485BUS_H_

#include "uart/CommDef.h"
#include "uart/ProtocolSender.h"
#include "uart/UartContext.h"

class Rs485Bus {
public:
	static Rs485Bus* getInstance() {
		static Rs485Bus sBus;
		return &sBus;
	}

	bool open(const char *pFileName, UINT baudRate) {
		if (UARTCONTEXT->isOpen()) {
			return true;
		}
		return UARTCONTEXT->openUart(pFileName, baudRate);
	}

	void close() {
		UARTCONTEXT->closeUart();
	}

	bool isOpen() const {
		return UARTCONTEXT->isOpen();
	}

	bool sendRaw(const BYTE *pData, UINT len) {
		if (!pData || len == 0) {
			return false;
		}
		return UARTCONTEXT->send(pData, len);
	}

	bool sendByte(BYTE data) {
		return sendRaw(&data, 1);
	}

	bool sendFrame(UINT16 cmdID, const BYTE *pData, BYTE len) {
		return sendProtocol(cmdID, pData, len);
	}

	bool sendFrame(UINT16 cmdID) {
		return sendProtocol(cmdID, NULL, 0);
	}

private:
	Rs485Bus() {}
	Rs485Bus(const Rs485Bus&);
	Rs485Bus& operator=(const Rs485Bus&);
};

#define RS485BUS Rs485Bus::getInstance()

#endif /* _UART_RS485BUS_H_ */
