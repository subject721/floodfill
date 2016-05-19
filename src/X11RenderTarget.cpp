#include "X11RenderTarget.h"


//X11 includes:
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

struct X11RenderTarget::PrivateData
{
	Display*		display;
	Window			window;
	GC				gc;
	Atom			wmDeleteMessage;

	uint32_t		pixelDepth;

	Pixmap			mainBufferPixmap;
	XImage*			mainBufferImage;
};


X11RenderTarget::X11RenderTarget(uint32_t width, uint32_t height, const std::string& displayName, uint32_t screenNum, bool fullscreen) :
	RenderTarget(width, height),
	_privateData(nullptr),
	_displayName(displayName),
	_screenNum(screenNum),
	_fullscreen(fullscreen)
{
	_privateData = new PrivateData();

	_shouldExit = false;
}

X11RenderTarget::~X11RenderTarget()
{
	destruct();

	delete _privateData;
}

void X11RenderTarget::init()
{
	LOG("Creating X11 render target");

	if(_displayName.empty())
	{
		WThrow(Exception("X11 display name must be specified."));
	}

	_privateData->display = XOpenDisplay(_displayName.c_str());

	if(_privateData->display == nullptr)
	{
		WThrow(Exception("Could not connect to X server"));
	}

	//uint32_t screenWidth	= DisplayWidth(_privateData->display, _screenNum);
	//uint32_t screenHeight	= DisplayHeight(_privateData->display, _screenNum);

	uint32_t borderWidth = 0;
	XVisualInfo vinfo;
    if (!XMatchVisualInfo(_privateData->display, XDefaultScreen(_privateData->display), 32, TrueColor, &vinfo))
	{
		WThrow(Exception("Could not get visual"));
	}

	_privateData->pixelDepth = vinfo.depth;

	XSetWindowAttributes attrs;

	const long translucent = 0xff<<24;

	Window rootWindow = XDefaultRootWindow(_privateData->display);
	attrs.colormap = XCreateColormap(_privateData->display, rootWindow, vinfo.visual, AllocNone);
	attrs.background_pixel = translucent | BlackPixel(_privateData->display, _screenNum);
	attrs.border_pixel = BlackPixel(_privateData->display, _screenNum);
	attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
            StructureNotifyMask;

	if(_fullscreen)
		attrs.override_redirect = True;
	else
		attrs.override_redirect = False;

	_privateData->window = XCreateWindow(_privateData->display, rootWindow,
                              0, 0, getWidth(), getHeight(), borderWidth,
							  vinfo.depth,
							  InputOutput,
							  vinfo.visual,
							  CWEventMask | CWBackPixel | CWColormap | CWBorderPixel | CWOverrideRedirect, &attrs);

	XSizeHints hints;

	hints.min_width  = getWidth();
	hints.max_width  = getWidth();
	hints.min_height = getHeight();
	hints.max_height = getHeight();

	hints.flags = PMinSize | PMaxSize;

	XSetWMNormalHints(_privateData->display, _privateData->window, &hints);

    /* make the window actually appear on the screen. */
    XMapWindow(_privateData->display, _privateData->window);

    /* flush all pending requests to the X server. */
    XFlush(_privateData->display);

	if(_fullscreen)
	{
		Atom atoms[2] = { XInternAtom(_privateData->display, "_NET_WM_STATE_FULLSCREEN", False), None };
		XChangeProperty(
		  _privateData->display,
		  _privateData->window,
		  XInternAtom(_privateData->display, "_NET_WM_STATE", False),
		  XA_ATOM, 32, PropModeReplace, (const uint8_t*)atoms, 1
		);
	}

	unsigned long valuemask = 0;
	XGCValues values;

	_privateData->gc = XCreateGC(_privateData->display, _privateData->window, valuemask, &values);
	if(_privateData->gc < 0)
	{
		WThrow(Exception("Could not create graphics context"));
	}

	XMapRaised(_privateData->display, _privateData->window);

	XSetForeground(_privateData->display, _privateData->gc, BlackPixel(_privateData->display, _screenNum));
    XSetBackground(_privateData->display, _privateData->gc, WhitePixel(_privateData->display, _screenNum));

	XSync(_privateData->display, False);

	_privateData->wmDeleteMessage = XInternAtom(_privateData->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(_privateData->display, _privateData->window, &(_privateData->wmDeleteMessage), 1);

	_privateData->mainBufferPixmap = XCreatePixmap(_privateData->display, _privateData->window, getWidth(), getHeight(), vinfo.depth);


	XFlush(_privateData->display);
}

void X11RenderTarget::setOutImage(Image& image)
{
	if(image.getFormat() != IMG_FORMAT_RGBA32)
	{
		WThrow(Exception("Image must be 32bit rgba image"));
	}

	BulkDataRef imageData = image.getData();

	_privateData->mainBufferImage = XCreateImage(_privateData->display, nullptr, _privateData->pixelDepth, ZPixmap, 0, (char*)imageData->getPtr(), getWidth(), getHeight(), 32, 0);

	if(_privateData->mainBufferImage == nullptr)
	{
		WThrow(Exception("Could not create main buffer image"));
	}

	//TEST:
	XPutImage (_privateData->display, _privateData->mainBufferPixmap, _privateData->gc, _privateData->mainBufferImage, 0, 0, 0, 0, getWidth(), getHeight());

	XCopyArea(_privateData->display, _privateData->mainBufferPixmap, _privateData->window, _privateData->gc, 0, 0, getWidth(), getHeight(), 0, 0);

	XFlush(_privateData->display);
}

void X11RenderTarget::process()
{
	XEvent event;

	if(_privateData->mainBufferImage)
	{
		XPutImage (_privateData->display, _privateData->mainBufferPixmap, _privateData->gc, _privateData->mainBufferImage, 0, 0, 0, 0, getWidth(), getHeight());
		XCopyArea(_privateData->display, _privateData->mainBufferPixmap, _privateData->window, _privateData->gc, 0, 0, getWidth(), getHeight(), 0, 0);
	}

	while (XPending(_privateData->display) > 0)
	{
	    // Fetch next event:
	    XNextEvent(_privateData->display, &event);

	    // Process the event:
		switch (event.type)
		{
			case Expose:
			{
				//LOG(std::string("Exposure"));
				XCopyArea(_privateData->display, _privateData->mainBufferPixmap, _privateData->window, _privateData->gc, 0, 0, getWidth(), getHeight(), 0, 0);

				break;
			}
			case KeyPress:
			{
				if ( event.xkey.keycode == 0x09 )
				{
					_shouldExit = true;
				}
				break;
			}
			case ButtonPress:
			{
				break;
			}
			case ClientMessage:
			{
				if((int32_t)event.xclient.data.l[0] == (int32_t)_privateData->wmDeleteMessage)
				{
					_shouldExit = true;
				}
				break;
			}
			default: break;
		}
	}
}

bool X11RenderTarget::requestsExit() const
{
	return shouldExit();
}

bool X11RenderTarget::shouldExit() const
{
	return _shouldExit;
}

void X11RenderTarget::destruct()
{
	if(_privateData != nullptr)
	{
		if(_privateData->display != nullptr)
		{
			if(_privateData->mainBufferImage)
			{
				XDestroyImage(_privateData->mainBufferImage);
			}

			XFreePixmap(_privateData->display, _privateData->mainBufferPixmap);

			XDestroyWindow(_privateData->display, _privateData->window);


			XCloseDisplay(_privateData->display);

			_privateData->display = nullptr;
		}
	}
}
