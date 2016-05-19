#pragma once

#include <stdint.h>

//Debugging stuff
#include <cstdarg>
#include <iostream>

#if defined(__i386__)
// IA-32
#define ARCH_IA32
#elif defined(__x86_64__)
// AMD64
#define ARCH_AMD64
#else
# error Unsupported architecture
#endif

template < typename _T1, typename _T2 >
struct SameModifier
{
	typedef _T1 First;
	typedef _T2 Second;
};

template < typename _T1, typename _T2 >
struct SameModifier<const _T1, _T2>
{
	typedef _T1 First;
	typedef const _T2 Second;
};

template < typename _T1, typename _T2 >
struct SameModifier<_T1, const _T2>
{
	typedef const _T1 First;
	typedef _T2 Second;
};


class LogMessage
{
public:
	LogMessage()
	{

	}

	LogMessage(const std::string& msg) :
		_msg(msg) {	}

	LogMessage(const char* fmt, ...)
	{
		va_list Vl;
		char TempBuffer[1024];

		va_start(Vl, fmt);

		vsnprintf(TempBuffer, 1024, fmt, Vl);

		_msg = std::string(TempBuffer);
	}

	operator const char* () const
	{
		return _msg.c_str();
	}

	operator const std::string& () const
	{
		return _msg;
	}

	const std::string& get() const
	{
		return _msg;
	}

private:
	std::string _msg;
};

inline std::ostream& operator << (std::ostream& lhs, const LogMessage& msg)
{
	lhs << msg.get();

	return lhs;
}

#define LOG(...) std::cerr << LogMessage(__VA_ARGS__) << std::endl

//Exception stuff

class Exception
{
public:
	Exception() {}

	Exception(const std::string& Message) :
		Message(Message) {}

	Exception(const char* Format, ...)
	{
		va_list Vl;
		char TempBuffer[1024];

		va_start(Vl,Format);

		vsnprintf(TempBuffer, 1024, Format, Vl);

		Message = std::string(TempBuffer);
	}

	const std::string& GetMessage() const
	{
		return Message;
	}

private:
	std::string Message;
};

template < typename _TExceptionBase >
class TExceptionWrapper : public _TExceptionBase
{
public:
	TExceptionWrapper(const _TExceptionBase& BaseInstance, const char* FileName, uint32_t LineNumber) :
		_TExceptionBase(BaseInstance),
		FileName(FileName),
		LineNumber(LineNumber)
	{

	}

	const char* GetFileName() const
	{
		return FileName;
	}

	uint32_t GetLineNumber() const
	{
		return LineNumber;
	}

private:
	const char* FileName;
	uint32_t LineNumber;
};

template < typename _TExceptionBase >
void inline WThrowRelay(const _TExceptionBase& BaseInstance, const char* FileName, uint32_t LineNumber)
{
	throw TExceptionWrapper<_TExceptionBase>(BaseInstance, FileName, LineNumber);
}

#define WThrow(E) WThrowRelay(E, __FILE__, __LINE__)
#define WTry try
#define WCatch(T, N) catch(const TExceptionWrapper<T>& N)

class RefCountedObject
{
public:
	RefCountedObject()
	{
		refCount = 0;
	}

	void incRefCount()
	{
		refCount++;
	}

	void decRefCount()
	{
		refCount--;

		if(refCount == 0)
		{
			delete this;
		}
	}

private:
	uint32_t refCount;
};


template < typename T >
class ObjectRef
{
public:
	typedef T ObjectType;

	ObjectRef() : objectPtr(nullptr){}

	explicit ObjectRef(ObjectType* objectPtr)
	{
		addLocalRef(objectPtr);
	}

	ObjectRef(const ObjectRef<T>& other)
	{
		addLocalRef(other.objectPtr);
	}

	~ObjectRef()
	{
		removeLocalRef();
	}

	ObjectRef<T>& operator = (const ObjectRef<T>& other)
	{
		removeLocalRef();

		addLocalRef(other.objectPtr);

		return *this;
	}

	bool valid() const
	{
		return (objectPtr != nullptr);
	}

	void release()
	{
		removeLocalRef();
	}

	const ObjectType* operator -> () const
	{
		return objectPtr;
	}

	ObjectType* operator -> ()
	{
		return objectPtr;
	}

	const ObjectType& operator * () const
	{
		return *objectPtr;
	}

	ObjectType& operator * ()
	{
		return *objectPtr;
	}

private:
	void addLocalRef(ObjectType* objectPtr)
	{
		if(objectPtr != nullptr)
		{
			objectPtr->incRefCount();

			this->objectPtr = objectPtr;
		}
	}

	void removeLocalRef()
	{
		if(objectPtr != nullptr)
		{
			objectPtr->decRefCount();

			objectPtr = nullptr;
		}
	}

	ObjectType* objectPtr;
};
