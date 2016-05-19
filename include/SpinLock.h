#pragma once

#include <atomic>

class SpinLock
{
public:
	SpinLock();
	~SpinLock();

	void lock();
	void unlock();

private:
	std::atomic_uint _lockState;
};