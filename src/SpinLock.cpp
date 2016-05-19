#include "SpinLock.h"


SpinLock::SpinLock()
{
	_lockState = 0;
}

SpinLock::~SpinLock()
{

}

void SpinLock::lock()
{
	unsigned int expected = 0;

	while(_lockState.compare_exchange_weak(expected, 1, std::memory_order_relaxed, std::memory_order_relaxed));
}

void SpinLock::unlock()
{
	_lockState.store(0, std::memory_order_relaxed);
}

