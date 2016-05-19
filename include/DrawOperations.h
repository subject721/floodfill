#pragma once

#include "Common.h"


#define OP_DRAW_PIXEL		0x00000001


/*
*	DrawOpDescr stores one draw operation with all required parameters.
*	Rather ugly coding style but it needs to be fucking fast. So no fancy C++ stuff
*/

struct DrawOpDescr
{
	uint32_t	size; // Total size of structure
	uint32_t	operationType; //Type of operation => e.g. draw pixel/primitive, ...
	uint8_t		opData[]; //Byte array with unspecified size => contains all parameters
}__attribute__((packed));


struct DrawPixelParams
{
	uint32_t x;
	uint32_t y;
	uint32_t color;
}__attribute__((packed));

#define DRAW_PIXEL_OP_SIZE (sizeof(DrawOpDescr) + sizeof(DrawPixelParams))



