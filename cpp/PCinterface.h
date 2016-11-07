/*
 * PCinterface.h
 *
 *  Created on: 7 lis 2016
 *      Author: PatLas
 */

#ifndef PCINTERFACE_H_
#define PCINTERFACE_H_

#include <string>
#include <fstream>
#include <ostream>
namespace comm_proto {

using namespace std;
class PCinterface : public HWInterface {
public:
	std::ofstream w;
	std::ifstream r;
	virtual ~PCinterface() {};
	PCinterface(){
		w.open("write.txt");
		r.open("read.txt");
	};

	void sendByte(uint8_t dat){
		w<<dat<<std::flush;
	};

	uint8_t recvByte(){
		return r.get();
	};
};

} /* namespace comm_proto */

#endif /* PCINTERFACE_H_ */
