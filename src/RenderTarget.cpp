#include "RenderTarget.h"

RenderTarget::RenderTarget(uint32_t width, uint32_t height) :
	_width(width),
	_height(height)
{

}

RenderTarget::~RenderTarget()
{

}

uint32_t RenderTarget::getWidth() const
{
	return _width;
}

uint32_t RenderTarget::getHeight() const
{
	return _height;
}

