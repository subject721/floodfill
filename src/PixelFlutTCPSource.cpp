#include "PixelFlutTCPSource.h"

#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

PixelFlutTCPSource::PixelFlutTCPSource(const std::string& listenAddress, uint16_t listenPort) :
	PixelFlutSource(),
	_listenAddress(listenAddress),
	_listenPort(listenPort)
{

}

PixelFlutTCPSource::~PixelFlutTCPSource()
{
}

void PixelFlutTCPSource::start(IDrawInterface* drawInterface)
{
	PixelFlutSource::start(drawInterface);

	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(_serverSocket < 0)
	{
		WThrow(Exception("Could not create socket"));
	}

	int rc, on = 1;

	rc = setsockopt(_serverSocket, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on));
	if(rc < 0)
	{
		close(_serverSocket);

		WThrow(Exception("Could not set socket to REUSEADDR"));
	}

	rc = ioctl(_serverSocket, FIONBIO, (char *)&on);
	if(rc < 0)
	{
		close(_serverSocket);

		WThrow(Exception("Could not set socket to nonblocking mode"));
	}

	struct sockaddr_in localAddr;

	memset(&localAddr, 0, sizeof(localAddr));

	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(_listenPort);

	rc = bind(_serverSocket, (struct sockaddr *)&localAddr, sizeof(localAddr));
	if(rc < 0)
	{
		close(_serverSocket);

		WThrow(Exception("Could not bind socket"));
	}

	rc = listen(_serverSocket, 32);
	if(rc < 0)
	{
		close(_serverSocket);

		WThrow(Exception("listen() failed"));
	}

	_run = true;
	_receiveThread = std::thread(std::bind(&PixelFlutTCPSource::receiveLoop, this));
}

void PixelFlutTCPSource::stop()
{
	//Shutdown receiver thread
	_run = false;
	_receiveThread.join();

	close(_serverSocket);

	PixelFlutSource::stop();
}

void PixelFlutTCPSource::receiveLoop()
{
	struct pollfd fds[MAX_NUM_CLIENTS + 1]; // Reserve on additional slot for the server socket
	int nfds = 1;
	int timeoutMs = 250; // More or less educated guess
	bool rebuildFdMap = false;

	fds[0].fd = _serverSocket;
	fds[0].events = POLLIN;

	for(uint32_t i = 0; i < MAX_NUM_CLIENTS; i++)
	{
		fds[i + 1].events = POLLIN;
	}

	while(_run)
	{
		if(rebuildFdMap)
		{
			nfds = 1;

			// This client cache thing makes it possible to access the std::map only if a client connects or disconnects. Otherwise only simple array dereferencing is done.

			for(auto client : _clients)
			{
				if(client.second != nullptr)
				{
					_clientCache[nfds - 1] = client.second;
					fds[nfds].fd = _clientCache[nfds - 1]->clientSocket;

					nfds++;
				}
				else
				{
					continue;
				}
			}

			rebuildFdMap = false;
		}

		int rc = poll(fds, nfds, timeoutMs);

		if(rc > 0)
		{
			for(uint32_t i = 0; i < (uint32_t)nfds; i++)
			{
				int pfd = fds[i].fd;

				if(pfd < 0)
					continue;

				if(fds[i].revents & POLLIN)
				{
					fds[i].revents = 0;

					if(i == 0)
					{
						int clientSocket = accept(_serverSocket, NULL, NULL);

						if(_clients.size() == MAX_NUM_CLIENTS)
						{
							close(clientSocket);
						}
						else
						{
							ClientInfo* client = new ClientInfo();

							client->clientSocket = clientSocket;

							LOG("Client connected: %d", clientSocket);

							_clients.insert(std::make_pair(clientSocket, client));

							rebuildFdMap = true;
						}
					}
					else
					{
						ClientInfo* client = _clientCache[i - 1];

						uint8_t buffer[NET_BUFFER_SIZE];
						int readStatus = 0;

						readStatus = recv(pfd, buffer, NET_BUFFER_SIZE, 0);

						if(readStatus == 0)
						{
							LOG("Client disconnected: %d", pfd);

							std::map<int, ClientInfo*>::iterator clientIt = _clients.find(pfd);

							close(pfd);

							_clients.erase(clientIt);

							delete client;

							rebuildFdMap = true;
						}
						else if(readStatus > 0)
						{
							client->dataBuffer.append((const char*)buffer, readStatus);

							//LOG("client %d data: %s", client->clientSocket, client->dataBuffer.c_str());

							handleClient(client);
						}
					}
				}
			}
		}
	}
}

void PixelFlutTCPSource::handleClient(ClientInfo* client)
{
	size_t lastNlIdx = 0;
	size_t lastValidNlIdx = 0;

	do
	{
		size_t nlIdx = client->dataBuffer.find('\n', lastNlIdx + 1);

		if(nlIdx != std::string::npos)
		{
			char outCmd[OUT_CMD_MAX_LEN];

			size_t offset = 0;
			if(lastValidNlIdx > 0)
			{
				offset = lastValidNlIdx + 1;
			}

			const char* cmdStr = client->dataBuffer.c_str() + offset;
			int cmdStrLen = nlIdx - lastNlIdx + 1;

			int rc = handleCmd(cmdStr, cmdStrLen, outCmd, OUT_CMD_MAX_LEN);

			if(rc > 0)
			{
				send(client->clientSocket, outCmd, rc, 0);
			}
			//else if(rc < 0)
			//{
			//	LOG("Invalid cmd: %s", cmdStr);
			//}

			lastValidNlIdx = nlIdx;
		}

		lastNlIdx = nlIdx;

	}while(lastNlIdx != std::string::npos);

	if(lastValidNlIdx > 0)
		client->dataBuffer.erase(0, lastValidNlIdx + 1);
}
