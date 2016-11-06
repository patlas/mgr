/*
 * CRC.h
 *
 *  Created on: 5 lis 2016
 *      Author: PatLas
 */

#ifndef CRC_H_
#define CRC_H_

namespace comm_proto {

class CRC {

public:
	uint8_t size;
	uint32_t crc_val;
	virtual ~CRC() {}

	virtual uint32_t calculate(const void const *, uint32_t) = 0;

	virtual bool compare(CRC& crc){
		if(crc_val == crc.crc_val) return true;
		return false;
	}

};

} /* namespace comm_proto */

#endif /* CRC_H_ */
