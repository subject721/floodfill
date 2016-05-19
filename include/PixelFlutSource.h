#pragma once

#include "Common.h"
#include "FloodFill.h"


#include <string>

enum PixelFLutCmdType
{
	CMD_PX,
	CMD_SIZE
};

struct PixelFlutCmd
{
	PixelFLutCmdType	cmd;
	uint8_t				nargs;
	uint32_t			args[4];
};


class PixelFlutSource : public DrawSource
{
public:
	PixelFlutSource();

	virtual ~PixelFlutSource();

	//If you derive from this class and overwrite the following two functions ALWAYS call the functions of this base class!
	virtual void start(IDrawInterface* drawInterface);
	virtual void stop();

protected:

	int handleCmd(const char* cmdIn, int cmdInLen, char* cmdOut, int cmdOutBufferSize);

private:
	const static int TOKEN_BUFFER_SIZE = 32;

	bool parseCmd(PixelFlutCmd& cmdStruct, const char* cmd, int cmdInLen);

	int getCmdType(const char* cmdStr, int len);

	IDrawInterface* _drawInterface;
};
