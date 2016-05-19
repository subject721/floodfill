#include "PixelFlutSource.h"

#include "DrawIF.h"
#include "DrawOperations.h"

#include <cstring>
#include <cstdlib>

PixelFlutSource::PixelFlutSource()
{

}

PixelFlutSource::~PixelFlutSource()
{

}

void PixelFlutSource::start(IDrawInterface* drawInterface)
{
	_drawInterface = drawInterface;
}

void PixelFlutSource::stop()
{

}

int PixelFlutSource::handleCmd(const char* cmdIn, int cmdInLen, char* cmdOut, int cmdOutBufferSize)
{
	PixelFlutCmd cmdStruct;

	if(!parseCmd(cmdStruct, cmdIn, cmdInLen))
	{
		return -1;
	}

	if(cmdStruct.cmd == CMD_PX)
	{
		//LOG("CMD_PX %u %u %x", cmdStruct.args[0], cmdStruct.args[1], cmdStruct.args[2]);

		if(cmdStruct.nargs == 3)
		{
			//Fucking dirty hack! But in this prealpharandomexcusewhyitmaynotbeworking version i don't give a shit!
			uint8_t drawOpBuf[DRAW_PIXEL_OP_SIZE];
			DrawOpDescr* drawPixelCmd = (DrawOpDescr*)drawOpBuf;
			drawPixelCmd->size = DRAW_PIXEL_OP_SIZE;
			drawPixelCmd->operationType = OP_DRAW_PIXEL;

			DrawPixelParams* drawPixelParams = (DrawPixelParams*)drawPixelCmd->opData;
			drawPixelParams->x		= cmdStruct.args[0];
			drawPixelParams->y		= cmdStruct.args[1];
			drawPixelParams->color	= cmdStruct.args[2];

			_drawInterface->queueDrawCmd(drawPixelCmd);
		}
		else if(cmdStruct.nargs == 2)
		{
			uint32_t pixelColor = _drawInterface->readPixel(cmdStruct.args[0], cmdStruct.args[1]);

			int len = snprintf(cmdOut, cmdOutBufferSize, "PX %u %u %06x\n", cmdStruct.args[0], cmdStruct.args[1], pixelColor);

			return len;
		}

		return 0;
	}
	else if(cmdStruct.cmd == CMD_SIZE)
	{
		uint32_t width  = _drawInterface->getWidth();
		uint32_t height = _drawInterface->getHeight();

		int len = snprintf(cmdOut, cmdOutBufferSize, "SIZE %u %u\n", width, height);

		return len;
	}

	return 0;
}

bool PixelFlutSource::parseCmd(PixelFlutCmd& cmdStruct, const char* cmd, int cmdInLen)
{
	char tokenBuffer[TOKEN_BUFFER_SIZE];
	int	currentTokenLen = 0;

	uint32_t currentTokenIndex = 0;

	for(int charIndex = 0; charIndex < cmdInLen; charIndex++)
	{
		char currentChar = cmd[charIndex];

		if((currentChar == ' ') || (currentChar == '\n'))
		{
			tokenBuffer[currentTokenLen] = '\0';

			//LOG("token %d: %s", currentTokenIndex, tokenBuffer);

			if(currentTokenIndex == 0)
			{
				int cmdType = getCmdType(tokenBuffer, currentTokenLen);
				if(cmdType == -1)
					return false;

				cmdStruct.cmd = (PixelFLutCmdType)cmdType;
			}
			else
			{
				//Hardcoded argument parsing.
				if(cmdStruct.cmd == CMD_PX)
				{
					switch(currentTokenIndex)
					{
						case 1:
						{
							cmdStruct.args[0] = (uint32_t)atoi(tokenBuffer);
							break;
						}
						case 2:
						{
							cmdStruct.args[1] = (uint32_t)atoi(tokenBuffer);
							break;
						}
						case 3:
						{
							cmdStruct.args[2] = (uint32_t)strtoul(tokenBuffer, nullptr, 16);

							if(currentTokenLen == 6)
							{
								cmdStruct.args[2] <<= 8;
								cmdStruct.args[2] |= 0xff;
							}
							//else if(currentTokenLen == 8)
							//{

							//}

							break;
						}
					}
				}
			}

			currentTokenLen = 0;
			currentTokenIndex++;
		}
		else
		{
			tokenBuffer[currentTokenLen] = currentChar;
			currentTokenLen++;
		}
	}

	cmdStruct.nargs = currentTokenIndex - 1;

	if(cmdStruct.cmd == CMD_PX)
	{
		if(!((currentTokenIndex == 4) || (currentTokenIndex == 3)))
			return false;
	}
	else if(cmdStruct.cmd == CMD_SIZE)
	{
		if(currentTokenIndex != 1)
			return false;
	}

	return true;
}

int PixelFlutSource::getCmdType(const char* cmdStr, int len)
{
	if(strncmp(cmdStr, "PX", len) == 0)
		return CMD_PX;
	else if(strncmp(cmdStr, "SIZE", len) == 0)
		return CMD_SIZE;

	return -1;
}
