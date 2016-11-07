/*
 * UartInterface.h
 *
 *  Created on: 6 lis 2016
 *      Author: PatLas
 */

#ifndef UARTINTERFACE_H_
#define UARTINTERFACE_H_

#include "HWInterface.h"

namespace comm_proto {

class UartInterface: public HWInterface {
public:
	virtual ~UartInterface() {}

	UartInterface(uint32_t baud, uint8_t dataBits, bool parity)
	{

	}
	void sendByte(uint8_t data)
	{

	}

	uint8_t recvByte()
	{
		//if timeout than return 0??
		return 0x00;
	}



};

} /* namespace comm_proto */

#endif /* UARTINTERFACE_H_ */
