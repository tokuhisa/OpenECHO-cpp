/*
 * EchoUDPProtocol.cpp
 *
 *  Created on: 2014/03/17
 *      Author: Fumiaki
 */

#include "EchoUDPProtocol.h"

const int EchoUDPProtocol::UDP_MAX_PACKET_SIZE = 65507;
const int EchoUDPProtocol::PORT = 3610;

EchoUDPProtocol::EchoUDPProtocol() {
	// TODO Auto-generated constructor stub
	mReceiverSock = 0;

}

EchoUDPProtocol::~EchoUDPProtocol() {
	// TODO Auto-generated destructor stub
}

void EchoUDPProtocol::openUDP() {
	mReceiverSock = socket(AF_INET, SOCK_DGRAM, 0);

	mReceiverSockAddr.sin_family = AF_INET;
	mReceiverSockAddr.sin_port = htons(PORT);
	mReceiverSockAddr.sin_addr.s_addr = INADDR_ANY;


	bind(mReceiverSock, (struct sockaddr *)&mReceiverSockAddr, sizeof(mReceiverSockAddr));

	int val = 1;
	ioctl(mReceiverSock, FIONBIO, &val);

	memset(&mReceiverIpMreq, 0, sizeof(mReceiverIpMreq));
	mReceiverIpMreq.imr_interface.s_addr = INADDR_ANY;
	mReceiverIpMreq.imr_multiaddr.s_addr = inet_addr(EchoSocket::MULTICAST_ADDRESS.c_str());

	u_char loop = 0; // 0 = invalid, 1 = valid(default);
	if(setsockopt(mReceiverSock
		, IPPROTO_IP
		, IP_MULTICAST_LOOP
		, &loop
		, sizeof(loop)) != 0) {
		perror("EchoUDPProtocol::open()[IP_MULTICAST_LOOP]");
		return;
	}
	if (setsockopt(mReceiverSock
		, IPPROTO_IP
		, IP_ADD_MEMBERSHIP
		, (char *)&mReceiverIpMreq
		, sizeof(mReceiverIpMreq)) != 0) {
		perror("EchoUDPProtocol::open()[IP_ADD_MEMBERSHIP]");
		return;
	}

}

void EchoUDPProtocol::closeUDP() {

	close(mReceiverSock);
}

void EchoUDPProtocol::sendUDP(const EchoFrame& frame) {

	if(frame.getDstEchoAddress() == EchoSocket::SELF_ADDRESS) {
		sendToSelf(frame);
		return;
	}
	int sock;
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(frame.getDstEchoAddress().c_str());

	if(frame.getDstEchoAddress() == EchoSocket::MULTICAST_ADDRESS) {
		in_addr_t ipaddr = inet_addr(EchoSocket::SELF_ADDRESS.c_str());

		if (setsockopt(sock,
				IPPROTO_IP,
				IP_MULTICAST_IF,
				(char *)&ipaddr, sizeof(ipaddr)) != 0) {
			perror("sendUDPFrame");
			return;
		 }
	}


	std::vector<unsigned char> byteArray = frame.getFrameByteArray();
	int size = byteArray.size();
	unsigned char buffer[size];
	for(int i = 0; i < size; i++) {
		buffer[i] = byteArray.at(i);
	}

	sendto(sock, buffer, 5, byteArray.size(), (struct sockaddr *)&addr, sizeof(addr));

	std::cerr << "sendUDPFrame:" << std::hex;
	for(int i = 0; i < size; i++) {
		std::cerr << (int)(buffer[i]) << " ";
	}
	std::cerr << std::endl;


	close(sock);
	if(frame.getDstEchoAddress() == EchoSocket::MULTICAST_ADDRESS) {
		EchoFrame f = frame;
		f.setDstEchoAddress(EchoSocket::SELF_ADDRESS);
		sendToSelf(f);
	}
}

void EchoUDPProtocol::sendToSelf(const EchoFrame& frame) {
}

void EchoUDPProtocol::receive() {

	int size;
	unsigned char buffer[UDP_MAX_PACKET_SIZE];

	struct sockaddr_in from;
	unsigned int addrSize = sizeof(struct sockaddr_in);
	memset(buffer, 0, sizeof(buffer));
	size = recvfrom(mReceiverSock, buffer, sizeof(buffer), 0,(struct sockaddr *)&from, &addrSize);
	if(size < EchoFrame::MIN_FRAME_SIZE) {
		return;
	}
	std::vector<unsigned char> data;
	for(int i = 0; i < size; i++) {
		data.push_back(buffer[i]);
	}

	std::cerr << "receiveUDPFrame:" << std::hex;
	for(int i = 0; i < size; i++) {
		std::cerr << (int)(buffer[i]) << " ";
	}
	std::cerr << std::endl;
	const char* caddress = inet_ntoa(from.sin_addr);
	std::string address = caddress;
	EchoFrame frame(address, data);

}

EchoUDPProtocol::UDPProtocolTask::UDPProtocolTask(EchoFrame frame,
		EchoProtocol& protocol) : mProtocol(protocol) {
}

EchoUDPProtocol::UDPProtocolTask::~UDPProtocolTask() {
}

void EchoUDPProtocol::UDPProtocolTask::respond(const EchoFrame& response) {
	mProtocol.sendUDP(response);
}

void EchoUDPProtocol::UDPProtocolTask::informAll(const EchoFrame& response) {
	mProtocol.sendUDP(response);
}