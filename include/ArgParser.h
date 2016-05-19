#pragma once

#include "Common.h"

#include <string>
#include <algorithm>
#include <list>
#include <vector>
#include <functional>

class ArgumentBase
{
public:
	ArgumentBase(const std::string shortOpt, const std::string& longOpt, const  std::string& description, bool isMandatory);
	virtual ~ArgumentBase();

	const std::string& getShortOpt() const;
	const std::string& getLongOpt() const;
	const std::string& getDescription() const;

	bool isMandatory() const;
	bool isSet() const;
	bool isComplete() const;

	bool shouldAbortParsing() const;

	virtual bool setValueFromStr(const std::string& valueStr){return false;}

protected:

	void setValueFlag();

	void abortParsing();

private:
	std::string		_shortOpt;
	std::string		_longOpt;
	std::string		_description;

	bool			_isMandatory;
	bool			_isSet;
	bool			_abortParsing;
};

template < typename _T >
struct ArgTypeConverter
{
	static bool convert(_T& value, const std::string& str)
	{
		return false;
	}
};

template < >
struct ArgTypeConverter<int>
{
	static bool convert(int& value, const std::string& str)
	{
		value = (int)strtoul(str.c_str(), nullptr, 10);

		return true;
	}
};

template < >
struct ArgTypeConverter<unsigned int>
{
	static bool convert(unsigned int& value, const std::string& str)
	{
		if(str.find("-") != std::string::npos)
			return false;

		bool isHex = false;
		std::string strCpy = str;

		size_t idx = strCpy.find("0x");
		if(idx == 0)
		{
			isHex = true;

			strCpy.erase(0, 2);
		}

		value = (int)strtoul(strCpy.c_str(), nullptr, isHex ? 16 : 10);

		return true;
	}
};

template < >
struct ArgTypeConverter<bool>
{
	static bool convert(bool& value, const std::string& str)
	{
		std::string strCpy = str;

		std::transform(strCpy.begin(), strCpy.end(), strCpy.begin(), ::tolower);

		if(strCpy == "true")
			value = true;
		else
			value = false;

		return true;
	}
};

template < >
struct ArgTypeConverter<std::string>
{
	static bool convert(std::string& value, const std::string& str)
	{
		value = str;

		return true;
	}
};



template < typename _TValue >
class ValueArgument : public ArgumentBase
{
public:
	typedef _TValue ValueType;

	ValueArgument(const std::string shortOpt,
		const std::string& longOpt,
		const std::string& description = "",
		bool isMandatory = false,
		std::function<bool (const ValueType&)> validator = nullptr
	) :
		ArgumentBase(shortOpt, longOpt, description, isMandatory),
		_value(),
		_validator(validator)
	{

	}

	virtual ~ValueArgument()
	{

	}

	operator const ValueType& ()
	{
		return _value;
	}

	const ValueType& getValue() const
	{
		return _value;
	}

	void setDefault(const ValueType& defaultValue)
	{
		_value = defaultValue;
	}

protected:
	virtual bool setValueFromStr(const std::string& valueStr)
	{
		if(!ArgTypeConverter<ValueType>::convert(_value, valueStr))
		{
			return false;
		}

		if(_validator)
		{
			if(!_validator(_value))
				return false;
		}

		setValueFlag();

		return true;
	}

private:
	ValueType	_value;

	std::function<bool (const ValueType&)> _validator;
};

template < typename _TValue >
struct RangeValidator
{
	typedef _TValue ValueType;

	static std::function<bool (const ValueType&)> generate(const ValueType& lowerBound, const ValueType& upperBound)
	{
		return [lowerBound, upperBound] (const ValueType& value) -> bool
		{
			if(value < lowerBound)
				return false;
			if(value > upperBound)
				return false;

			return true;
		};
	}
};

class CallbackArgument : public ArgumentBase
{
public:
	CallbackArgument(std::function<bool (const std::string&)> callback,
		const std::string shortOpt,
		const std::string& longOpt,
		const std::string& description = "") :
		ArgumentBase(shortOpt, longOpt, description, false),
		_callback(callback),
		_wasCalled(false)
	{

	}

	virtual ~CallbackArgument()
	{

	}

	bool wasCalled() const
	{
		return _wasCalled;
	}

protected:
	virtual bool setValueFromStr(const std::string& valueStr)
	{
		if(!_callback)
			return false;

		if(_callback(valueStr))
		{
			abortParsing();
		}

		_wasCalled = true;

		return true;
	}

private:

	std::function<bool (const std::string&)> _callback;

	bool _wasCalled;
};

class ArgParser
{
public:
	ArgParser();
	~ArgParser();

	void parse(int argc, char** argv);

	//Allows chained calls (argParser.addArgument().addArgument(). ...)
	//In case it is not possible to add the specified argument an exception is thrown
	ArgParser& addArgument(ArgumentBase& argument);

	const std::list<ArgumentBase*>& getArguments() const;

private:
	bool checkDuplicate(ArgumentBase& argument);

	ArgumentBase* getArgByOpt(const std::string& argOpt, bool useLongOpt);

	std::list<ArgumentBase*> _arguments;
	std::vector<std::string> _nonArgParams;
};
