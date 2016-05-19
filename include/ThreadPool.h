#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "Common.h"

struct TaskContext
{
	size_t taskId;
	size_t subId;//When queuing multiple copied tasks
	size_t workerId;
};

class IRunnable
{
public:
	virtual void run(const TaskContext& taskContext) = 0;
};

class ThreadPool
{
public:
	ThreadPool() = delete;
	ThreadPool(const ThreadPool& other) = delete;

	ThreadPool(unsigned int numWorkers, unsigned int reserveQueueSize = 0);

	~ThreadPool();

	void shutdown();

	unsigned int getNumWorkers() const;

	unsigned int getNumQueuedTasks();

	void queue(IRunnable* runnable, size_t taskId, unsigned int copies = 1);
	void queue(const std::function<void (const TaskContext&)>& runnable, size_t taskId, unsigned int copies = 1);

	void queue(const std::vector<IRunnable*>& runnables, size_t taskId);
	void queue(const std::vector<std::function<void (const TaskContext&)> >& runnables);
	void queue(std::vector<std::function<void (const TaskContext&)> >& runnables);//This function doesn't copy the function objects but moves them instead. Therefore are they invalid after calling this function.

private:
	class ThreadPoolIntern;

	ThreadPoolIntern* _threadPoolIntern;
};
