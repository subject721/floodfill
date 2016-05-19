#include "ThreadPool.h"


#include <thread>
#include <mutex>
#include <condition_variable>

#include <deque>
#include <vector>


struct Task
{
	Task()
	{
		_r = nullptr;
		//_id = 0;
		_subId = 0;
	}

	size_t										_id;
	size_t										_subId;
	IRunnable*									_r;
	std::function<void (const TaskContext&)> 	_f;
};

class ThreadPoolGuard
{
public:
	ThreadPoolGuard()
	{

	}

	~ThreadPoolGuard()
	{

	}

	void notifyOne()
	{
		_cond.notify_one();
	}

	void notifyAll()
	{
		_cond.notify_all();
	}

	std::mutex& getLock()
	{
		return _mutex;
	}

	std::condition_variable& getConditionVariable()
	{
		return _cond;
	}

private:
	std::mutex _mutex;
	std::condition_variable _cond;

};

class ThreadPoolGuardLock
{
public:
	ThreadPoolGuardLock() = delete;

	ThreadPoolGuardLock(ThreadPoolGuard* guard) :
		_guard(guard),
		_lock(guard->getLock())
	{

	}

	~ThreadPoolGuardLock()
	{

	}

	void wait(unsigned int ms)
	{
		_guard->getConditionVariable().wait_for(_lock, std::chrono::milliseconds(ms));
	}

private:
	ThreadPoolGuard*				_guard;
	std::unique_lock<std::mutex>	_lock;
};

class TaskQueue
{
public:
	TaskQueue()
	{
		//_queueLock = std::unique_lock<std::mutex>(_queueMutex, std::defer_lock);
	}

	~TaskQueue()
	{

	}

	void reserve(unsigned int numElements)
	{
		//TODO: Not implemented
	}

	void pushTask(const Task& task, unsigned int copies)
	{
		if(copies == 1)
			_queue.push_back(task);
		else
		{
			Task taskCopy = task;

			for (size_t index = 0; index < copies; index++)
			{
				taskCopy._subId = index;
				_queue.push_back(taskCopy);
			}
		}
			//_queue.insert(_queue.end(), copies, task);
	}

	unsigned int getSize() const
	{
		return _queue.size();
	}

	bool hasElements() const
	{
		return !_queue.empty();
	}

	bool popTask(Task& task)
	{
		if(_queue.empty())
			return false;

		task = _queue.front();

		_queue.pop_front();

		return true;
	}

private:
	std::deque<Task>				_queue;
	//std::mutex						_queueMutex;
	//std::unique_lock<std::mutex>	_queueLock;
};

class ThreadPoolWorker
{
public:
	enum eWorkerState
	{
		WS_STOPPED,
		WS_STARTUP,
		WS_WAITING,
		WS_EXECUTING,
		WS_SHUTDOWN
	};

	ThreadPoolWorker()
	{
		_workerState = WS_STOPPED;
	}

	ThreadPoolWorker(TaskQueue* taskQueue, ThreadPoolGuard* guard, unsigned int id) :
		_id(id),
		_taskQueue(taskQueue),
		_guard(guard)
	{
		_workerState = WS_STOPPED;
	}

	~ThreadPoolWorker()
	{

	}

	void start()
	{
		if(_taskQueue == nullptr)
		{
			WThrow(Exception("Could not start worker because no task queue is available."));
		}

		if(_workerState != WS_STOPPED)
		{
			WThrow(Exception("Could not start worker because worker thread is already running."));
		}

		_shutdown = false;

		_workerThread = std::thread(std::bind(&ThreadPoolWorker::threadEntry, this));
	}

	void stop(bool noWait = false)
	{
		if(_workerState != WS_STOPPED)
		{

			{
				ThreadPoolGuardLock guardLock(_guard);
				//std::lock_guard< std::unique_lock<std::mutex> > scopeLock(_taskQueue->getQueueLock());
				//LOCK_GUARD(_taskQueue->getQueueLock());

				_shutdown = true;

				if(_workerState == WS_WAITING)
				{

					//_queueCondition.notify_one();
					_guard->notifyAll();
				}
			}

			_workerThread.join();
		}
	}


private:
	void threadEntry()
	{

		_workerState = WS_STARTUP;

		Task task;

		TaskContext taskContext;

		taskContext.workerId = _id;

		while(_workerState != WS_SHUTDOWN)
		{
			if(waitQueue(task))
			{
				//LOG("ThreadPoolWorker[%d]: executing task", _id);
				taskContext.taskId = task._id;
				taskContext.subId = task._subId;

				if(task._r != nullptr)
				{
					task._r->run(taskContext);
				}
				else if(task._f)
				{
					task._f(taskContext);
				}
			}
			else
			{
				if(_shutdown)
				{
					_workerState = WS_SHUTDOWN;
				}
			}
		}


		_workerState = WS_STOPPED;
	}

	bool waitQueue(Task& task)
	{
		//std::unique_lock guardLock = _guard->getLock();
		//LOCK_GUARD(guardLock);
		//std::lock_guard< std::unique_lock<std::mutex> >(_taskQueue->getQueueLock());
		//LOCK_GUARD(_taskQueue->getQueueLock());
		ThreadPoolGuardLock guardLock(_guard);

		//LOG("ThreadPoolWorker[%d]: waitQueue() enter", _id);

		if(_taskQueue->hasElements())
		{
			_taskQueue->popTask(task);

			return true;
		}

		eWorkerState oldState = _workerState;
		_workerState = WS_WAITING;

		//LOG("ThreadPoolWorker[%d]: waitQueue() wait", _id);
		//_queueCondition.wait(_taskQueue->getQueueLock());
		guardLock.wait(1000);
		//LOG("ThreadPoolWorker[%d]: waitQueue() wakeup", _id);

		_workerState = oldState;

		//Early abort
		if(_shutdown)
			return false;

		if(_taskQueue->hasElements())
		{
			_taskQueue->popTask(task);

			return true;
		}

		return false;
	}

	unsigned int			_id;
	std::thread				_workerThread;
	eWorkerState			_workerState;
	bool					_shutdown;
	TaskQueue*				_taskQueue;
	ThreadPoolGuard*		_guard;
	std::condition_variable _queueCondition;

};

class ThreadPool::ThreadPoolIntern
{
public:
	ThreadPoolIntern()
	{

	}

	~ThreadPoolIntern()
	{
		for(ThreadPoolWorker* worker : _workers)
		{
			worker->stop();

			delete worker;
		}

		_workers.clear();
	}

	void init(unsigned int numWorkers, unsigned int reserveQueueSize)
	{
		ThreadPoolGuardLock guardLock(&_guard);

		for(unsigned int i = 0; i < numWorkers; i++)
		{
			startWorker();
		}

		if(reserveQueueSize > 0)
			_taskQueue.reserve(reserveQueueSize);
	}

	inline unsigned int getNumWorkers() const
	{
		return _workers.size();
	}

	inline unsigned int getNumQueuedTasks()
	{
		//ThreadPoolGuardLock guardLock(&_guard);

		return _taskQueue.getSize();
	}

	inline void queueTask(const Task& task, unsigned int copies)
	{
		//std::lock_guard< std::unique_lock<std::mutex> >(_taskQueue.getQueueLock());
		{
			ThreadPoolGuardLock guardLock(&_guard);

			//bool listEmpty = !_taskQueue.hasElements();

			_taskQueue.pushTask(task, copies);
		}
		//if(listEmpty)
		//{
		_guard.notifyAll();
		//}
	}

	inline void queueMultipleTasks(const std::vector<IRunnable*>& runnables, size_t taskId)
	{
		{
			ThreadPoolGuardLock guardLock(&_guard);

			Task task;

			for(size_t index = 0; index < runnables.size(); index++)
			{
				IRunnable* runnable = runnables.at(index);

				task._r = runnable;
				task._id = taskId;
				task._subId = index;

				_taskQueue.pushTask(task, 1);
			}
		}

		_guard.notifyAll();
	}

private:

	void startWorker()
	{
		unsigned int thisId = _workers.size();

		ThreadPoolWorker* worker = new ThreadPoolWorker(&_taskQueue, &_guard, thisId);

		_workers.push_back(worker);

		worker->start();
	}

	TaskQueue						_taskQueue;
	ThreadPoolGuard					_guard;
	std::vector<ThreadPoolWorker*>	_workers;
};

/*
ThreadPool::ThreadPool()
{

}
*/

ThreadPool::ThreadPool(unsigned int numWorkers, unsigned int reserveQueueSize)
{
	_threadPoolIntern = new ThreadPoolIntern();

	_threadPoolIntern->init(numWorkers, reserveQueueSize);
}

ThreadPool::~ThreadPool()
{
	delete _threadPoolIntern;
}

void ThreadPool::shutdown()
{

}

unsigned int ThreadPool::getNumWorkers() const
{
	return _threadPoolIntern->getNumWorkers();
}

unsigned int ThreadPool::getNumQueuedTasks()
{
	return _threadPoolIntern->getNumQueuedTasks();
}

void ThreadPool::queue(IRunnable* runnable, size_t taskId, unsigned int copies)
{
	Task task;

	task._r = runnable;
	task._id = taskId;

	_threadPoolIntern->queueTask(task, copies);
}

void ThreadPool::queue(const std::function<void (const TaskContext&)>& runnable, size_t taskId, unsigned int copies)
{
	Task task;

	task._f = runnable;
	task._id = taskId;

	_threadPoolIntern->queueTask(task, copies);
}

void ThreadPool::queue(const std::vector<IRunnable*>& runnables, size_t taskId)
{
	_threadPoolIntern->queueMultipleTasks(runnables, taskId);
}

void ThreadPool::queue(const std::vector<std::function<void (const TaskContext&)> >& runnables)
{

}

void ThreadPool::queue(std::vector<std::function<void (const TaskContext&)> >& runnables)
{

}
