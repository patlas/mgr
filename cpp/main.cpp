/*
 * main.cpp
 *
 *  Created on: 7 lis 2016
 *      Author: PatLas
 */


#include "CommProtocol.h"
#include "PCinterface.h"
#include "CRC32.h"
#include <iostream>

using namespace comm_proto;
int main(void)
{
	CRC32 crc; //TODO dlaczego crc() nie dzia³¹!?
	PCinterface pcif; //TODO dlaczego pcif() nie dzia³a?!

	uint8_t buff[] = "Jakis dziwny napis, testujemy czy wszystko dziala jak powinno, hahaha!\n";

	CommProtocol cp(crc, pcif, (uint8_t)3,(uint16_t)3);

	cout<<cp.sendData(buff, 10);

	return 0;
}

