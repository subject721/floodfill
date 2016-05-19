#include "FloodFill.h"

#include "DrawQueue.h"
#include "RenderTarget.h"
#include "DrawIF.h"
#include "SpinLock.h"
#include "ThreadPool.h"
#include "Renderer.h"

#include <cstring>
#include <thread>

class FloodFillDrawIF : public IDrawInterface
{
public:
	FloodFillDrawIF(uint32_t width, uint32_t height, Renderer* renderer) :
		_width(width),
		_height(height),
		_renderer(renderer)
	{
		_drawQueues[0] = new DrawQueue(DEFAULT_DRAW_QUEUE_SIZE);
		_drawQueues[1] = new DrawQueue(DEFAULT_DRAW_QUEUE_SIZE);
	}

	virtual ~FloodFillDrawIF()
	{
		delete _drawQueues[0];
		delete _drawQueues[1];
	}

	virtual bool queueDrawCmd(DrawOpDescr* drawCmd)
	{
		std::lock_guard<SpinLock> queueGuard(_drawQueueLock);

		DrawQueue* writeQueue = getCurrentWriteQueue();

		DrawOpDescr* queuedDrawCmd = writeQueue->allocateDrawOpRaw(drawCmd->size);

		memcpy(queuedDrawCmd, drawCmd, drawCmd->size);

		return true;
	}

	virtual uint32_t readPixel(uint32_t x, uint32_t y)
	{
		if(x >= _width)
			return 0;
		if(y >= _height)
			return 0;

		return _renderer->getPixel(x, y);
	}

	virtual uint32_t getWidth()
	{
		return _width;
	}

	virtual uint32_t getHeight()
	{
		return _height;
	}

	DrawQueue* getCurrentWriteQueue()
	{
		return _drawQueues[0];
	}

	DrawQueue* getCurrentReadQueue()
	{
		return _drawQueues[1];
	}

	void exchangeQueues()
	{
		std::lock_guard<SpinLock> queueGuard(_drawQueueLock);

		std::swap(_drawQueues[0], _drawQueues[1]);

		_drawQueues[0]->clear();
	}


private:
	const static uint32_t		DEFAULT_DRAW_QUEUE_SIZE = (1<<30); // 1MByte

	uint32_t					_width;
	uint32_t					_height;

	std::array<DrawQueue*, 2>	_drawQueues; // Simple double buffering scheme
	SpinLock					_drawQueueLock;

	Renderer*					_renderer;
};

FloodFill::FloodFill(uint32_t width, uint32_t height, uint32_t fps) :
	_width(width),
	_height(height),
	_fps(fps)
{
	LOG("Initializing FloodFill");

	_run = false;

	_fpsReal = (double)_fps;

	_threadPool = new ThreadPool(4);

	_image = Image(width, height, IMG_FORMAT_RGBA32, true);

	PixelWrapper<FormatRGBA32, void*> pixelWrapper((void*)_image.getData()->getPtr(), width);

	//Init image
	for(uint32_t y = 0; y < height; y++)
	{
		for(uint32_t x = 0; x < width; x++)
		{
			pixelWrapper.getPixel(y, x).alpha = 0xff;
			pixelWrapper.getPixel(y, x).red = 0x00;
			pixelWrapper.getPixel(y, x).green = 0x00;
			pixelWrapper.getPixel(y, x).blue = 0x00;
		}
	}

	_renderer = new Renderer();

	_renderer->setDrawBuffer(_image);

	_drawIF = new FloodFillDrawIF(_width, _height, _renderer);

}

FloodFill::~FloodFill()
{
	delete _threadPool;

	delete _drawIF;

	delete _renderer;
}

bool FloodFill::addRenderTarget(RenderTarget* renderTarget)
{
	_renderTargets.push_back(renderTarget);

	renderTarget->setOutImage(_image);

	return true;
}

void FloodFill::removeRenderTarget(RenderTarget* renderTarget)
{
	for(auto it = _renderTargets.begin(); it != _renderTargets.end(); ++it)
	{
		if(*it == renderTarget)
		{
			_renderTargets.erase(it);

			return;
		}
	}
}

bool FloodFill::addDrawSource(DrawSource* drawSource)
{
	WTry
	{
		drawSource->start(_drawIF);
	}
	WCatch(Exception, e)
	{
		LOG("adding draw source failed @ %s:%u: %s", e.GetFileName(), e.GetLineNumber(), e.GetMessage().c_str());

		return false;
	}

	_drawSources.push_back(drawSource);

	return true;
}

void FloodFill::removeDrawSource(DrawSource* drawSource)
{
	drawSource->stop();

	for(auto it = _drawSources.begin(); it != _drawSources.end(); ++it)
	{
		if(*it == drawSource)
		{
			_drawSources.erase(it);

			return;
		}
	}
}

uint32_t FloodFill::getWidth() const
{
	return _width;
}

uint32_t FloodFill::getHeight() const
{
	return _height;
}

void FloodFill::mainLoop()
{

	_run = true;

	_currentTimePoint = std::chrono::high_resolution_clock::now();

	double cycleTime = 1.0 / ((double)_fps);

	double lastLogTime = 0;

	while(_run)
	{
		if(_time > (lastLogTime + 2.0))
		{
			LOG("%08f : %08f", _time, _fpsReal);

			lastLogTime = _time;
		}

		while(_threadPool->getNumQueuedTasks() > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		_drawIF->exchangeQueues();

		DrawQueue* currentReadQueue = _drawIF->getCurrentReadQueue();
		if(currentReadQueue != nullptr)
		{
			renderDrawQueue(currentReadQueue);
		}

		for(auto renderTarget : _renderTargets)
		{
			renderTarget->process();

			if(renderTarget->requestsExit())
			{
				stop();
			}
		}


		//Really ugly timing

		std::chrono::time_point<std::chrono::high_resolution_clock> nextTimeStep = std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> elapsed_seconds = nextTimeStep - _currentTimePoint;
		_currentTimePoint = nextTimeStep;

		double currentDelta = (double)elapsed_seconds.count();

		double timeOld = _time;

		_time += currentDelta;

		if(currentDelta < cycleTime)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)((cycleTime - currentDelta) * 1000.0)));
		}

		nextTimeStep = std::chrono::high_resolution_clock::now();

		elapsed_seconds = nextTimeStep - _currentTimePoint;
		_currentTimePoint = nextTimeStep;

		currentDelta = (double)elapsed_seconds.count();

		_time += currentDelta;

		double totalCycleTime = _time - timeOld;
		double fpsCurrent = 1.0 / totalCycleTime;

		_fpsReal = 0.9 * _fpsReal + 0.1 * fpsCurrent;
	}
}

void FloodFill::stop()
{
	_run = false;
}

void FloodFill::renderDrawQueue(DrawQueue* drawQueue)
{
	//PixelWrapper<FormatRGBA32, void*> pixelWrapper((void*)_image.getData()->getPtr(), _image.getWidth());

	DrawQueue::Iterator it = drawQueue->begin();
	DrawQueue::Iterator end = drawQueue->end();

	while(it != end)
	{
		DrawOpDescr* drawOpDescr = *it;

		if(drawOpDescr)
		{
			if(drawOpDescr->operationType == OP_DRAW_PIXEL)
			{
				DrawPixelParams* drawPixelParams = (DrawPixelParams*)drawOpDescr->opData;

				if(drawPixelParams->x >= _width)
					drawPixelParams->x = _width - 1;

				if(drawPixelParams->y >= _height)
					drawPixelParams->y = _height - 1;

				//rgba32FromPackedUint(pixelWrapper.getPixel(drawPixelParams->y, drawPixelParams->x), drawPixelParams->color);
				_renderer->drawPoint(drawPixelParams->x, drawPixelParams->y, drawPixelParams->color);
			}
		}

		++it;
	}
}
