#include <iostream>

#include <signal.h>

#include "ArgParser.h"

#include "DrawQueue.h"
#include "DrawOperations.h"

#include "FloodFill.h"
#include "X11RenderTarget.h"
#include "PixelFlutUDPSource.h"
#include "PixelFlutTCPSource.h"

static FloodFill* floodFillInst = nullptr;

void signal_handler(int signal)
{
	if(floodFillInst != nullptr)
	{
		floodFillInst->stop();
	}
}

int main(int argc, char** argv)
{
	ArgParser argParser;

	signal(SIGINT, signal_handler);

	CallbackArgument helpArg(
		[&argParser] (const std::string& arg) -> bool
		{
			for(ArgumentBase* currentArg : argParser.getArguments())
			{
				std::string shortArg = (currentArg->getShortOpt().empty()) ? "" : (std::string("-") + currentArg->getShortOpt());
				std::string longArg  = (currentArg->getLongOpt().empty()) ? "" : (std::string("--") + currentArg->getLongOpt());
				LOG("%3s , %16s : %s", shortArg.c_str(), longArg.c_str(), currentArg->getDescription().c_str());
			}

			return true;
		},
		"h",
		"help",
		"prints help how to use floodfill"
	);

	ValueArgument<std::string>  x11DisplayArg("d", "display", "Sets the X11 window ( e.g. :0 ).");
	ValueArgument<unsigned int> widthArg("", "width", "Sets the window width.", true, RangeValidator<unsigned int>::generate(1, 6000));
	ValueArgument<unsigned int> heightArg("", "height", "Sets the window width.", true, RangeValidator<unsigned int>::generate(1, 6000));
	ValueArgument<bool>			fullscreenArg("f", "fullscreen", "Makes the window a fullscreen window.");
	ValueArgument<unsigned int> fpsArg("", "fps", "Sets the frames per second of the underlying render system.", false, RangeValidator<unsigned int>::generate(1, 200));
	ValueArgument<unsigned int> tcpPortArg("p", "port", "Sets the port on which the TCP PixelFlut interface should listen.", true);

	x11DisplayArg.setDefault(":0");
	fullscreenArg.setDefault(false);
	fpsArg.setDefault(30);

	WTry
	{
		argParser
			.addArgument(helpArg)
			.addArgument(x11DisplayArg)
			.addArgument(widthArg)
			.addArgument(heightArg)
			.addArgument(fullscreenArg)
			.addArgument(fpsArg)
			.addArgument(tcpPortArg)
			.parse(argc, argv);
	}
	WCatch(Exception, e)
	{
		std::cerr << e.GetFileName() << ":" << e.GetLineNumber() << " => " << e.GetMessage() << std::endl;

		return 1;
	}

	//Check if help was called:
	if(helpArg.wasCalled())
		return 0;


	floodFillInst = new FloodFill(widthArg.getValue(), heightArg.getValue(), fpsArg.getValue());


	X11RenderTarget* defaultRenderTarget = new X11RenderTarget(floodFillInst->getWidth(),
		floodFillInst->getHeight(),
		x11DisplayArg.getValue(),
		0,
		fullscreenArg.getValue()
	);

	PixelFlutTCPSource* pixelFlutTCPSource = new PixelFlutTCPSource("0.0.0.0", tcpPortArg.getValue());

	WTry
	{
		defaultRenderTarget->init();

		floodFillInst->addRenderTarget(defaultRenderTarget);

		floodFillInst->addDrawSource(pixelFlutTCPSource);

		floodFillInst->mainLoop();

		floodFillInst->removeDrawSource(pixelFlutTCPSource);

		floodFillInst->removeRenderTarget(defaultRenderTarget);
	}
	WCatch(Exception, e)
	{
		std::cerr << e.GetMessage() << std::endl;

		return 1;
	}

	delete pixelFlutTCPSource;

	delete defaultRenderTarget;

	delete floodFillInst;

	return 0;
}