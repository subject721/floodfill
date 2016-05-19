#pragma once

#include "Common.h"
#include "FloodFill.h"


#include <string>
#include <thread>

class ThreadPool;

class PixelFlutUDPSource : public DrawSource
{
public:
	PixelFlutUDPSource(const std::string& listenAddress, uint16_t listenPort);

	PixelFlutUDPSource() = delete;

	virtual ~PixelFlutUDPSource();

	virtual void start(IDrawInterface* drawInterface);
	virtual void stop();


private:
	const static size_t NET_BUFFER_SIZE = 1500;

	void receiveLoop();

	void parseAndQueueCmd(const char* cmd, uint32_t len);

	IDrawInterface*		_currentDrawIF;

	std::string			_listenAddress;
	uint16_t			_listenPort;
	int					_socket;

	std::thread			_receiveThread;
	ThreadPool*			_threadPool;

	bool				_run;
};

