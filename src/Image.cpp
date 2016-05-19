#include "Image.h"

#include <cstring>

Image::Image()
{
	width = 0;
	height = 0;
	format = IMG_FORMAT_NONE;
}

Image::Image(const Image& other)
{
	width = other.width;
	height = other.height;
	format = other.format;

	imageData = other.imageData;
}

Image::Image(uint32_t width, uint32_t height, ImageFormat format, bool allocateStorage)
{
	this->width = width;
	this->height = height;
	this->format = format;

	if(allocateStorage)
	{
		imageData = BulkDataRef(new BulkData(calcImageSize()));
	}
}

Image::Image(uint32_t width, uint32_t height, ImageFormat format, const BulkDataRef& data)
{
	this->width = width;
	this->height = height;
	this->format = format;

	if(data.valid())
		imageData = BulkDataRef(new BulkData(data->getPtr(), data->getSize()));
}

Image::~Image()
{

}

Image& Image::operator = (const Image& other)
{
	if(this != (&other))
	{
		imageData.release();

		width = other.width;
		height = other.height;
		format = other.format;

		imageData = other.imageData;
	}

	return *this;
}

uint32_t Image::getWidth() const
{
	return width;
}

uint32_t Image::getHeight() const
{
	return height;
}

ImageFormat Image::getFormat() const
{
	return format;
}

const void* Image::getImageDataPtr() const
{
	if(imageData.valid())
		return (void*)imageData->getPtr();
	return nullptr;
}

const BulkDataRef& Image::getData()
{
	return imageData;
}

void Image::clear()
{
	if(imageData.valid() && (imageData->getSize() > 0))
	{
		size_t imageSize = calcImageSize();

		if(imageSize > 0)
			memset(imageData->getPtr(), 0, imageSize);
	}
}

bool Image::isComplete()
{
	if(format != IMG_FORMAT_NONE)
	{
		if((width > 0) && (height > 0))
		{
			if(imageData.valid())
				return true;
		}
	}

	return false;
}

void Image::detach()
{
	if(imageData.valid())
	{
		BulkDataRef currentContainer = imageData;

		imageData = BulkDataRef(new BulkData(currentContainer->getPtr(), currentContainer->getSize()));
	}
}

size_t Image::getPixelSize() const
{
	switch(format)
	{
		case IMG_FORMAT_NONE:
			return 0;
		case IMG_FORMAT_RGB24:
			return 3;
		case IMG_FORMAT_RGBA32:
			return 4;
		case IMG_FORMAT_GS8:
			return 1;
	}

	return 0;
}

size_t Image::calcImageSize() const
{
	size_t numPixels = width * height;
	size_t pixelSize = getPixelSize();

	return numPixels * pixelSize;
}