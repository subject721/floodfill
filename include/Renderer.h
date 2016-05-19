#pragma once

#include "Common.h"
#include "Image.h"
#include "DrawOperations.h"

class RendererOpBase;

class Renderer
{
public:
	Renderer();

	~Renderer();

	void setDrawBuffer(Image& image);

	uint32_t getPixel(uint32_t x, uint32_t y);

	//Draw operations:
	void drawPoint(uint32_t x, uint32_t y, uint32_t color);

private:

	Image					_drawBuffer;

	RendererOpBase* 		_renderOpImpl;
};
