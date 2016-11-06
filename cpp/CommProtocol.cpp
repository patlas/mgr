/*
 * UartProtocol.cpp
 *
 *  Created on: 5 lis 2016
 *      Author: PatLas
 */

#include "CommProtocol.h"

#include <string.h>

namespace comm_proto {



void CommProtocol::sendACK(bool ack)
{
	if(ack)
		interface.sendByte(0x06);
	else
		interface.sendByte(0x15);
}

bool CommProtocol::getACK()
{
	uint8_t ack = interface.recvByte();
	if(ack != 0x06)
		return false;

	return true;
}

void CommProtocol::buildFrame(DataType t, const uint8_t const* dat, const uint32_t count)
{
	interface.rawFrame.type = t;
	memcpy(interface.rawFrame.value, dat, count);
	interface.rawFrame.crc = crc.calculate(dat, value_byte_count);
}

bool CommProtocol::sendFrame()
{
	interface.sendByte(interface.rawFrame.type);

	interface.sendByte((uint8_t)interface.rawFrame.length>>24);
	interface.sendByte((uint8_t)interface.rawFrame.length>>16);
	interface.sendByte((uint8_t)interface.rawFrame.length>>8);
	interface.sendByte((uint8_t)interface.rawFrame.length);

	for(uint8_t i=0; i<value_byte_count; i++)
		interface.sendByte(*(interface.rawFrame.value+i));

	interface.sendByte((uint8_t)interface.rawFrame.crc>>24);
	interface.sendByte((uint8_t)interface.rawFrame.crc>>16);
	interface.sendByte((uint8_t)interface.rawFrame.crc>>8);
	interface.sendByte((uint8_t)interface.rawFrame.crc);

	return getACK();
}

bool CommProtocol::receiveFrame()
{
	uint8_t byte;

	interface.rawFrame.type = interface.recvByte();

	interface.rawFrame.length = interface.recvByte();
	interface.rawFrame.length <<= 8;
	interface.rawFrame.length |= interface.recvByte();
	interface.rawFrame.length <<= 8;
	interface.rawFrame.length |= interface.recvByte();
	interface.rawFrame.length <<= 8;
	interface.rawFrame.length |= interface.recvByte();

	for(uint8_t i=0; i<value_byte_count; i++)
		*(interface.rawFrame.value+i) = interface.recvByte();

	interface.rawFrame.crc = interface.recvByte();
	interface.rawFrame.crc <<= 8;
	interface.rawFrame.crc |= interface.recvByte();
	interface.rawFrame.crc <<= 8;
	interface.rawFrame.crc |= interface.recvByte();
	interface.rawFrame.crc <<= 8;
	interface.rawFrame.crc |= interface.recvByte();

	if(crc.calculate(interface.rawFrame.value, value_byte_count) == interface.rawFrame.crc)
	{
		sendACK(true);
		return true;
	}
	else
	{
		sendACK(false);
		return false;
	}
}



} /* namespace comm_proto */
