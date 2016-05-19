#pragma once

#include "Common.h"

#include "Image.h"

#include <array>
#include <mutex>
#include <vector>
#include <chrono>

class RenderTarget;
class DrawQueue;

class DrawSource;
class IDrawInterface;
class FloodFillDrawIF;
class ThreadPool;
class Renderer;

//TODO: Add user process handler (e.g. handling exit request from windows)

class FloodFill
{
public:
	FloodFill(uint32_t width, uint32_t height, uint32_t fps);
	FloodFill() = delete;

	~FloodFill();

	bool addRenderTarget(RenderTarget* renderTarget);
	void removeRenderTarget(RenderTarget* renderTarget);

	bool addDrawSource(DrawSource* drawSource);
	void removeDrawSource(DrawSource* drawSource);

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	void mainLoop();

	void stop();

private:

	DrawQueue* getCurrentWriteQueue();
	DrawQueue* getCurrentReadQueue();

	void exchangeQueues();

	void renderDrawQueue(DrawQueue* drawQueue);

	//Private members:

	uint32_t					_width;
	uint32_t					_height;

	uint32_t					_fps;
	double						_fpsReal;

	bool						_run;

	ThreadPool*					_threadPool;

	std::vector<RenderTarget*>	_renderTargets;

	std::vector<DrawSource*>	_drawSources;

	std::chrono::time_point<std::chrono::high_resolution_clock> _currentTimePoint;
	double						_time;

	Image						_image;

	FloodFillDrawIF*			_drawIF;
	Renderer*					_renderer;
};


class DrawSource
{
public:
	DrawSource(){}

	virtual ~DrawSource(){};

	virtual void start(IDrawInterface* drawInterface) = 0;
	virtual void stop() = 0;


private:

};
