#include "BulkData.h"

#include <cstring>

BulkData::BulkData()
{
	storagePtr = nullptr;
	size = 0;
}

BulkData::BulkData(const void* bufferPtr, size_t size)
{
	if(bufferPtr != nullptr)
	{
		this->size = size;
		storagePtr = new uint8_t[size];

		memcpy(storagePtr, bufferPtr, size);
	}
	else
	{
		this->size = 0;
		storagePtr = nullptr;
	}
}

BulkData::BulkData(size_t size)
{
	this->size = size;
	storagePtr = new uint8_t[size];

	//memset(storagePtr, 0, size);
}

BulkData::BulkData(const ObjectRef<BulkData>& otherRef)
{
	if(otherRef.valid())
	{
		if(otherRef->getSize() > 0)
		{
			const uint8_t* otherPtr = otherRef->getPtr();

			size = otherRef->getSize();
			storagePtr = new uint8_t[size];

			memcpy(storagePtr, otherPtr, size);

			return;
		}
	}

	size = 0;
	storagePtr = nullptr;
}

BulkData::~BulkData()
{
	if(size != 0)
	{
		delete[] storagePtr;

		storagePtr = nullptr;
		size = 0;
	}
}

size_t BulkData::getSize() const
{
	return size;
}

const uint8_t* BulkData::getPtr() const
{
	return storagePtr;
}

uint8_t* BulkData::getPtr()
{
	return storagePtr;
}

void BulkData::copyFromBuffer(const void* srcPtr, size_t size)
{
	if(size > (this->size))
		size = this->size;

	memcpy(storagePtr, srcPtr, size);
}

void BulkData::copyToBuffer(void* dstPtr, size_t size) const
{
	if(size > (this->size))
		size = this->size;

	memcpy(dstPtr, storagePtr, size);
}
