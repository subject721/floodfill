#pragma once


#include "Common.h"

#include "PixelFlutSource.h"

#include <string>
#include <map>
#include <thread>

class PixelFlutTCPSource : public PixelFlutSource
{
public:

	PixelFlutTCPSource(const std::string& listenAddress, uint16_t listenPort);

	PixelFlutTCPSource() = delete;

	virtual ~PixelFlutTCPSource();

	virtual void start(IDrawInterface* drawInterface);
	virtual void stop();

private:
	const static size_t NET_BUFFER_SIZE = 512;
	const static uint32_t MAX_NUM_CLIENTS = 128;
	const static uint32_t OUT_CMD_MAX_LEN = 32;

	struct ClientInfo
	{
		int clientSocket;
		std::string dataBuffer;
	};

	void receiveLoop();

	void handleClient(ClientInfo* client);


	std::string		_listenAddress;
	uint16_t		_listenPort;

	int				_serverSocket;

	bool			_run;

	std::thread		_receiveThread;

	std::map<int, ClientInfo*>	_clients;
	ClientInfo*					_clientCache[MAX_NUM_CLIENTS];
};