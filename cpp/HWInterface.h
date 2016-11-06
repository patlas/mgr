/*
 * Interface.h
 *
 *  Created on: 6 lis 2016
 *      Author: PatLas
 */

#ifndef HWINTERFACE_H_
#define HWINTERFACE_H_

namespace comm_proto {

class HWInterface {
public:
	virtual ~HWInterface() {}

	virtual void sendByte(uint8_t) = 0;
	virtual uint8_t recvByte() = 0;

	struct{
		uint32_t length;
		uint8_t *value;
		uint8_t type;
		uint32_t crc;
	}rawFrame;

// add init to current implementation

};

} /* namespace comm_proto */

#endif /* HWINTERFACE_H_ */
