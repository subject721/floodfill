#include "Renderer.h"


class RendererOpBase
{
public:
	virtual ~RendererOpBase() {}

	virtual uint32_t getPixel(uint32_t x, uint32_t y) = 0;

	virtual void drawPoint(uint32_t x, uint32_t y, uint32_t color) = 0;

protected:

};


static inline void pixelAlphaBlend(FormatRGBA32::PixelType& dst, uint32_t newColor)
{
	uint32_t alpha = (newColor) & 0xff;

	uint32_t red 	= (newColor >>  8) & 0xff;
	uint32_t green 	= (newColor >> 16) & 0xff;
	uint32_t blue	= (newColor	>> 24) & 0xff;

	dst.red		= ((dst.red	 	* (255 - alpha)) + red		* alpha) / 255;
	dst.green	= ((dst.green 	* (255 - alpha)) + green	* alpha) / 255;
	dst.blue	= ((dst.blue	* (255 - alpha)) + blue		* alpha) / 255;
}

static inline uint32_t pixelToRgba(const FormatRGBA32::PixelType& px)
{
	uint32_t color = 0;

	color |= px.blue;
	color <<= 8;
	color |= px.green;
	color <<= 8;
	color |= px.red;

	return color;
}

template < typename _TFormat >
class RendererOpTypeImpl : public RendererOpBase
{
public:

	typedef _TFormat FormatType;

	RendererOpTypeImpl(Image& drawBuffer) :
		_drawBuffer(drawBuffer),
		_pixelWrapper((void*)_drawBuffer.getImageDataPtr(), _drawBuffer.getWidth())
	{

	}

	virtual ~RendererOpTypeImpl()
	{

	}

	virtual uint32_t getPixel(uint32_t x, uint32_t y)
	{
		typename FormatType::PixelType& pixelRef = _pixelWrapper.getPixel(y, x);

		return pixelToRgba(pixelRef);
	}

	virtual void drawPoint(uint32_t x, uint32_t y, uint32_t color)
	{
		typename FormatType::PixelType& pixelRef = _pixelWrapper.getPixel(y, x);

		pixelAlphaBlend(pixelRef, color);
	}

private:
	Image								_drawBuffer;

	PixelWrapper<FormatType, void*>		_pixelWrapper;
};



//Helper stuff:
RendererOpBase* createRenderOpImpl(Image& image)
{
	if(image.getFormat() == IMG_FORMAT_RGBA32)
	{
		return new RendererOpTypeImpl<FormatRGBA32>(image);
	}
	return nullptr;
}



Renderer::Renderer()
{
	_renderOpImpl = nullptr;
}

Renderer::~Renderer()
{
	if(_renderOpImpl != nullptr)
	{
		delete _renderOpImpl;
	}
}

void Renderer::setDrawBuffer(Image& image)
{
	if(image.isComplete() == false)
		WThrow(Exception("You have to set an image with a valid format, size and storage. You dumb fuck!"));

	if(image.getFormat() != _drawBuffer.getFormat())
	{
		if(_renderOpImpl != nullptr)
		{
			delete _renderOpImpl;
		}

		_drawBuffer = image;

		_renderOpImpl = createRenderOpImpl(_drawBuffer);
	}
	else
	{
		_drawBuffer = image;
	}

}

uint32_t Renderer::getPixel(uint32_t x, uint32_t y)
{
	return _renderOpImpl->getPixel(x, y);
}

void Renderer::drawPoint(uint32_t x, uint32_t y, uint32_t color)
{
	_renderOpImpl->drawPoint(x, y, color);
}
