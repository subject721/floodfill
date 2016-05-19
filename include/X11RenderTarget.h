#pragma once

#include <string>

#include "Common.h"

#include "RenderTarget.h"

class X11RenderTarget : public RenderTarget
{
public:
	X11RenderTarget(uint32_t width, uint32_t height, const std::string& displayName, uint32_t screenNum = 0, bool fullscreen = false);

	X11RenderTarget() = delete;

	virtual ~X11RenderTarget();

	virtual void init();

	virtual void setOutImage(Image& image);

	virtual void process();

	virtual bool requestsExit() const;

	X11RenderTarget& operator = (const X11RenderTarget&) = delete;

	bool shouldExit() const;

private:
	void destruct();

	//Avoids revealing X11 internal data structures
	struct PrivateData;

	PrivateData*	_privateData;

	std::string		_displayName;
	uint32_t		_screenNum;
	bool			_fullscreen;

	bool			_shouldExit;
};