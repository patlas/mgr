/*
 * CRC32.h
 *
 *  Created on: 5 lis 2016
 *      Author: PatLas
 */

#ifndef CRC32_H_
#define CRC32_H_

#include "CRC.h"

namespace comm_proto {

class CRC32: public CRC {
public:
	CRC32() : CRC(32, 0) {};
	virtual ~CRC32() {};

	uint32_t calculate(const uint8_t*  data, uint32_t len)
	{
		crc_val = 0xAAAABBBB; //TODO crc 32 calculation
		return crc_val;
	}


};

} /* namespace comm_proto */




#endif /* CRC32_H_ */
