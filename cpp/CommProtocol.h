/*
 * UartProtocol.h
 *
 *  Created on: 5 lis 2016
 *      Author: PatLas
 */

#ifndef COMMPROTOCOL_H_
#define COMMPROTOCOL_H_


namespace comm_proto {

enum class DataType {COMMAND, UNKNOWN};

class CommProtocol{
public:
	CRC& crc;
	HWInterface& interface;
	uint8_t value_byte_count;

	virtual ~CommProtocol()
	{
		delete[] interface.rawFrame.value;
	}

	CommProtocol(CRC& crc, HWInterface& interface, uint8_t dataFieldSize) : crc(crc), interface(interface), value_byte_count(dataFieldSize)
	{
		interface.rawFrame.value = new uint8_t[dataFieldSize];
	}

	bool sendData(const uint8_t const *, const uint32_t);
	uint32_t receiveData(uint32_t, uint8_t*);

private:


	void sendACK(bool);
	bool getACK();
	bool sendFrame();
	bool receiveFrame();
	void buildFrame(DataType,const uint8_t const*, const uint32_t);
	bool checkCRC(); // unnecessary?
	bool retransmitFrame(); // unnecessary retransmit accomplished by sendData




};

} /* namespace comm_proto */

#endif /* COMMPROTOCOL_H_ */
