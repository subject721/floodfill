#include "PixelFlutUDPSource.h"

#include "ThreadPool.h"
#include "DrawIF.h"
#include "DrawOperations.h"

#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

//TODO: Implement according to PixelFlutSource class!!!!1111elfelf11!!s

PixelFlutUDPSource::PixelFlutUDPSource(const std::string& listenAddress, uint16_t listenPort) :
	DrawSource(),
	_currentDrawIF(nullptr),
	_listenAddress(listenAddress),
	_listenPort(listenPort),
	_socket(-1),
	_receiveThread(),
	_threadPool(nullptr),
	_run(false)
{

}

PixelFlutUDPSource::~PixelFlutUDPSource()
{

}

void PixelFlutUDPSource::start(IDrawInterface* drawInterface)
{
	LOG((const char*)"Starting PixelFlutUDPSource");

	_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(_socket < 0)
	{
		WThrow(Exception("Could not create UDP socket."));
	}

	struct sockaddr_in localAddr;

	memset(&localAddr, 0, sizeof(localAddr));

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(_listenPort);

	if (bind(_socket, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0)
	{
		close(_socket);

		WThrow(Exception("Could not bind socket to listen on %s:%u", _listenAddress.c_str(), _listenPort));
	}

	_currentDrawIF = drawInterface;
	_run = true;

	//_receiveThread = std::thread(std::bind(&PixelFlutUDPSource::receiveLoop, this));
}

void PixelFlutUDPSource::stop()
{
	LOG((const char*)"Stopping PixelFlutUDPSource");

	_run = false;

	_receiveThread.join();

	_currentDrawIF = nullptr;

	close(_socket);

}

void PixelFlutUDPSource::receiveLoop()
{

}

void PixelFlutUDPSource::parseAndQueueCmd(const char* cmd, uint32_t len)
{

}
