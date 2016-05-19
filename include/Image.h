#pragma once

#include "Common.h"
#include "BulkData.h"

enum ImageFormat
{
	IMG_FORMAT_NONE,
	IMG_FORMAT_RGB24, //24bit rgb image
	IMG_FORMAT_RGBA32, //32bit rgba image
	IMG_FORMAT_GS8    //8bit greyscale image
};

struct FormatRGB24
{
	const static ImageFormat FORMAT_ID = IMG_FORMAT_RGB24;

	const static size_t BYTES_PER_PIXEL = 3;

	const static size_t NUM_COMPONENTS = 3;

	const static size_t BYTES_PER_COMPONENT = 1;

	typedef uint8_t ComponentType;

	typedef struct
	{
		ComponentType red;
		ComponentType green;
		ComponentType blue;
	}PixelType;
};

struct FormatRGBA32
{
	const static ImageFormat FORMAT_ID = IMG_FORMAT_RGBA32;

	const static size_t BYTES_PER_PIXEL = 4;

	const static size_t NUM_COMPONENTS = 4;

	const static size_t BYTES_PER_COMPONENT = 1;

	typedef uint8_t ComponentType;

	typedef struct
	{
		ComponentType red;
		ComponentType green;
		ComponentType blue;
		ComponentType alpha;
	}PixelType;
};

inline void rgba32FromPackedUint(FormatRGBA32::PixelType& pixel, uint32_t color)
{
	*((uint32_t*) &pixel) = color;
}

struct FormatGS8
{
	const static ImageFormat FORMAT_ID = IMG_FORMAT_GS8;

	const static size_t BYTES_PER_PIXEL = 1;

	const static size_t NUM_COMPONENTS = 1;

	const static size_t BYTES_PER_COMPONENT = 1;

	typedef uint8_t ComponentType;

	typedef struct
	{
		ComponentType y;
	}PixelType;
};

template < typename TFormatDescr, typename TPointer = const void*>
class PixelWrapper
{
public:
	typedef TFormatDescr FormatDescriptor;
	typedef typename SameModifier<TPointer, uint8_t*>::Second BytePointerType;
	typedef typename SameModifier<TPointer, typename TFormatDescr::PixelType>::Second PixelType;

	PixelWrapper(TPointer imagePtr, size_t lineLength) :
		imagePtr(imagePtr),
		lineLength(lineLength)
	{
		bytesPerLine = lineLength * FormatDescriptor::BYTES_PER_PIXEL;
	}

	inline TPointer getLinePtr(uint32_t lineIndex)
	{
		return ((BytePointerType)imagePtr) + (lineIndex * bytesPerLine);
	}

	inline PixelType& getPixel(uint32_t lineIndex, uint32_t pixelOffset)
	{
		BytePointerType linePtr = (BytePointerType)getLinePtr(lineIndex);

		BytePointerType pixelPtr = linePtr + (pixelOffset * FormatDescriptor::BYTES_PER_PIXEL);

		return *((PixelType*)pixelPtr);
	}

private:
	TPointer imagePtr;
	size_t lineLength;
	size_t bytesPerLine; //Precalculated value
};

class Image
{
public:
	Image();
	Image(const Image& other);
	Image(uint32_t width, uint32_t height, ImageFormat format, bool allocateStorage = false);
	Image(uint32_t width, uint32_t height, ImageFormat format, const BulkDataRef& data);

	~Image();

	Image& operator = (const Image& other);

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	ImageFormat getFormat() const;

	const void* getImageDataPtr() const;

	const BulkDataRef& getData();

	size_t getPixelSize() const;

	void clear();

	bool isComplete(); //True if image has a valid format, width/height > 0 and allocated pixel storage.

private:
	void detach();

	size_t calcImageSize() const;

	uint32_t width;
	uint32_t height;
	ImageFormat format;

	BulkDataRef imageData;


};
