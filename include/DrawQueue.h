#pragma once

#include "Common.h"

#include "DrawOperations.h"

#include <atomic>

/*
* This queue for pending draw commands is thread safe without locking. It relies completely on atomic integer arithmetic.
*/
class DrawQueue
{
public:
	static const uint32_t INVALID_INDEX = 0xffffffff;

	class Iterator
	{
	public:
		Iterator();
		Iterator(const DrawQueue* queue, uint32_t index);
		~Iterator();

		DrawOpDescr* operator* () const;

		Iterator& operator++();
		Iterator operator++(int);

		bool operator == (const Iterator& other) const;
		bool operator != (const Iterator& other) const;

	private:
		void increment();

		const DrawQueue*	_queue;
		uint32_t			_index;
	};

	DrawQueue(uint32_t maxQueueSize);
	DrawQueue() = delete;
	~DrawQueue();

	uint32_t getMaxQueueSize() const;
	uint32_t getCurrentQueueSize() const;

	Iterator begin() const;
	Iterator end() const;

	void clear();

	//Allocates a new draw op. "dataSize" does not contain size of DrawOpDescr structure
	DrawOpDescr* allocateDrawOp(uint32_t dataSize, uint32_t type);
	DrawOpDescr* allocateDrawOpRaw(uint32_t totalSize);

private:
	DrawOpDescr* getData(uint32_t index) const;
	uint32_t getNext(uint32_t index);

	void hookInLast(uint32_t elementBaseIndex);

	uint32_t			_maxQueueSize;
	uint8_t*			_queuePtr;
	std::atomic_uint	_allocIndex;
	uint32_t			_lastElementIndex;

	friend class Iterator;
};
