#pragma once

#include "Common.h"
#include "Image.h"

class RenderTarget
{
public:
	RenderTarget(uint32_t width, uint32_t height);

	RenderTarget() = delete;

	virtual ~RenderTarget();

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	virtual void init() = 0;

	virtual void setOutImage(Image& image) = 0;

	virtual void process() = 0;

	virtual bool requestsExit() const = 0;

	RenderTarget& operator = (const RenderTarget&) = delete;

private:
	uint32_t		_width;
	uint32_t		_height;
};