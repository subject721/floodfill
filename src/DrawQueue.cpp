#include "DrawQueue.h"



DrawQueue::DrawQueue(uint32_t maxQueueSize) :
	_maxQueueSize(maxQueueSize)
{
	_queuePtr = new uint8_t[_maxQueueSize];

	clear();
}

DrawQueue::~DrawQueue()
{
	delete[] _queuePtr;
}

uint32_t DrawQueue::getMaxQueueSize() const
{
	return _maxQueueSize;
}

uint32_t DrawQueue::getCurrentQueueSize() const
{
	return _allocIndex;
}

DrawQueue::Iterator DrawQueue::begin() const
{
	return Iterator(this, 0);
}

DrawQueue::Iterator DrawQueue::end() const
{
	return Iterator(this, INVALID_INDEX);
}

void DrawQueue::clear()
{
	_allocIndex = 0;
	_lastElementIndex = INVALID_INDEX;
}

DrawOpDescr* DrawQueue::allocateDrawOp(uint32_t dataSize, uint32_t type)
{
	DrawOpDescr* elementPtr = allocateDrawOpRaw(sizeof(DrawOpDescr) + dataSize);

	if(elementPtr == nullptr)
		return nullptr;

	elementPtr->operationType = type;

	return elementPtr;
}

DrawOpDescr* DrawQueue::allocateDrawOpRaw(uint32_t totalSize)
{
	if(_allocIndex >= _maxQueueSize)
		return nullptr;

	//Reserve space for element and preceding index to next element.
	uint32_t elementSize = totalSize + sizeof(uint32_t);

	uint32_t elementIndex = _allocIndex.fetch_add(elementSize, std::memory_order_relaxed);

	if((elementIndex + elementSize) >= _maxQueueSize)
		return nullptr;

	uint8_t* elementBasePtr = _queuePtr + elementIndex;

	//Init index to next element:
	*((uint32_t*)elementBasePtr) = INVALID_INDEX;

	hookInLast(elementIndex);

	DrawOpDescr* elementPtr = (DrawOpDescr*)(elementBasePtr + sizeof(uint32_t));


	return elementPtr;
}

DrawOpDescr* DrawQueue::getData(uint32_t index) const
{
	if(index == INVALID_INDEX)
		return nullptr;

	uint8_t* elementBasePtr = _queuePtr + index;

	return (DrawOpDescr*)(elementBasePtr + sizeof(uint32_t));
}

uint32_t DrawQueue::getNext(uint32_t index)
{
	if(index == INVALID_INDEX)
		return INVALID_INDEX;

	if(_lastElementIndex == INVALID_INDEX)
		return INVALID_INDEX;

	if(index >= _lastElementIndex)
		return INVALID_INDEX;

	uint32_t* nextIndexPtr = (uint32_t*)(_queuePtr + index);

	uint32_t nextIndex = *nextIndexPtr;

	if(nextIndex != DrawQueue::INVALID_INDEX)
	{
		if(nextIndex >= _allocIndex)
		{
			nextIndex = DrawQueue::INVALID_INDEX;
		}
	}

	return nextIndex;
}

void DrawQueue::hookInLast(uint32_t elementBaseIndex)
{
	if(_lastElementIndex != INVALID_INDEX)
	{
		uint8_t* elementBasePtr = _queuePtr + _lastElementIndex;

		uint32_t* indexPtr = (uint32_t*)elementBasePtr;

		//Write "_lastElementIndex" first so iterating nevers leads to an invalid element
		_lastElementIndex = elementBaseIndex;

		*indexPtr = elementBaseIndex;
	}
	else
	{
		//In case of first queued element
		if(elementBaseIndex == 0)
		{
			_lastElementIndex = 0;
		}
	}
}

DrawQueue::Iterator::Iterator() :
	_queue(nullptr),
	_index(DrawQueue::INVALID_INDEX)
{

}

DrawQueue::Iterator::Iterator(const DrawQueue* queue, uint32_t index) :
	_queue(queue),
	_index(index)
{

}

DrawQueue::Iterator::~Iterator()
{

}

DrawOpDescr* DrawQueue::Iterator::operator* () const
{
	return _queue->getData(_index);
}

DrawQueue::Iterator& DrawQueue::Iterator::operator++()
{
	increment();

	return *this;
}

DrawQueue::Iterator DrawQueue::Iterator::operator++(int)
{
	Iterator preIt(_queue, _index);

	increment();

	return preIt;
}

bool DrawQueue::Iterator::operator == (const DrawQueue::Iterator& other) const
{
	if(_queue != other._queue)
		return false;

	if(_index == other._index)
		return true;

	if((_index >= _queue->_maxQueueSize) && (other._index >= _queue->_maxQueueSize))
		return true;

	return false;
}

bool DrawQueue::Iterator::operator != (const DrawQueue::Iterator& other) const
{
	return !(*this == other);
}

void DrawQueue::Iterator::increment()
{
	uint32_t* nextIndexPtr = (uint32_t*)(_queue->_queuePtr + _index);

	_index = *nextIndexPtr;

	if(_index != DrawQueue::INVALID_INDEX)
	{
		if(_index >= _queue->_allocIndex)
		{
			_index = DrawQueue::INVALID_INDEX;
		}
	}
}


