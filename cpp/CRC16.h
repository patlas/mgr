/*
 * CRC16.h
 *
 *  Created on: 5 lis 2016
 *      Author: PatLas
 */

#ifndef CRC16_H_
#define CRC16_H_

#include "CRC.h"

namespace comm_proto {

class CRC16: public CRC {
public:
	CRC16() : size(16), crc_val(0) {};
	virtual ~CRC16() {};

	uint32_t calculate(const void * const data, uint32_t len)
	{
		crc_val = 0xAAAA; //TODO crc 16 calculation
		return crc_val;
	}


};

} /* namespace comm_proto */

#endif /* CRC16_H_ */
