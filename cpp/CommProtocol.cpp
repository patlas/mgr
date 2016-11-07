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

void CommProtocol::buildFrame(DataType t, const uint8_t * const dat, const uint32_t count)
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


bool CommProtocol::sendData(const uint8_t * const dat, const uint32_t size)
{
	uint8_t retry_count = 0;
	uint32_t frameCount = size/value_byte_count; //without uncomplete rest if exist
	uint8_t remainder = size%value_byte_count;

	if(!size) return true; //no data in buffer

	if(frameCount) //ommit in case if size is lower that one frame data count
	{
		uint32_t index=0;
		do
		{
			do
			{
				buildFrame(DataType::UNKNOWN, &dat[index*value_byte_count], value_byte_count);
			} while(!sendFrame() && ++retry_count<trans_retry);

			if(retry_count >= trans_retry)
			{
				return false; // transmission error
			}

			retry_count=0;

		} while(++index < frameCount);
	}

	if(remainder > 0) //transmit remainder frame -> data end which do not fill frame value buffer
	{
		do
		{
			buildFrame(DataType::UNKNOWN, &dat[frameCount*value_byte_count], remainder);
		} while(!sendFrame() && ++retry_count<trans_retry);

		if(retry_count >= trans_retry)
		{
			return false;
		}
	}

	return true;
}

uint32_t CommProtocol::receiveData(const uint32_t size, uint8_t* dat, const uint16_t timeout_ms)
{
	uint8_t retry_count = 0;
	uint32_t frame_count = size/value_byte_count;
	uint8_t remainder = size%value_byte_count;
	uint32_t frame_index = 0;

	if(!size) return 0; //nothing to receive

	if(frame_count)
	{
		do
		{
			while(!receiveFrame() && ++retry_count<trans_retry);
			if(retry_count >= trans_retry) return ((frame_index/*-1*/)*value_byte_count); //check if -1 is necessary
			memcpy(&dat[frame_index*value_byte_count], interface.rawFrame.value, value_byte_count);
			retry_count=0;
		} while(++frame_index < frame_count);
	}

	if(remainder)
	{
		while(!receiveFrame() && ++retry_count<trans_retry);
		if(retry_count >= trans_retry) return ((frame_index/*-1*/)*value_byte_count); //check if -1 is necessary
		memcpy(&dat[frame_index*value_byte_count], interface.rawFrame.value, remainder);

		return ((frame_index/*-1*/)*value_byte_count) + remainder;
	}

	return ((frame_index/*-1*/)*value_byte_count);
}

} /* namespace comm_proto */
