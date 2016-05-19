#pragma once

#include "Common.h"
#include "DrawOperations.h"

class IDrawInterface
{
public:
	virtual ~IDrawInterface(){}

	//This function queues a draw command and returns nearly immediately. If you call this function during a buffer switch it might lock for a very short period (few microseconds).
	//Additionally this function is completely thread safe.
	virtual bool queueDrawCmd(DrawOpDescr* drawCmd) = 0;

	//Function for reading pixels. This function is completely synchronous since there is no mechanism to associate the caller with the response.
	//This might be changed in the future but for now use this function with caution
	virtual uint32_t readPixel(uint32_t x, uint32_t y) = 0;

	virtual uint32_t getWidth() = 0;
	virtual uint32_t getHeight() = 0;
};