#pragma once

#include "Common.h"

class BulkData : public RefCountedObject
{
public:
	BulkData(); //Empty container
	BulkData(const void* bufferPtr, size_t size); //Initialization from pointer and size
	BulkData(size_t size); //Only storage allocation but no initialization
	BulkData(const ObjectRef<BulkData>& otherRef); //Copy from existing container

	~BulkData();

	size_t getSize() const;

	const uint8_t* getPtr() const;
	uint8_t* getPtr();

	void copyFromBuffer(const void* srcPtr, size_t size);
	void copyToBuffer(void* dstPtr, size_t size) const;

private:

	uint8_t* storagePtr;
	size_t size;
};

typedef ObjectRef<BulkData> BulkDataRef;
