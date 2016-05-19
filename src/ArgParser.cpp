#include "ArgParser.h"

ArgumentBase::ArgumentBase(const std::string shortOpt, const std::string& longOpt, const  std::string& description, bool isMandatory) :
	_shortOpt(shortOpt),
	_longOpt(longOpt),
	_description(description),
	_isMandatory(isMandatory),
	_isSet(false),
	_abortParsing(false)
{

}

ArgumentBase::~ArgumentBase()
{

}

const std::string& ArgumentBase::getShortOpt() const
{
	return _shortOpt;
}

const std::string& ArgumentBase::getLongOpt() const
{
	return _longOpt;
}

const std::string& ArgumentBase::getDescription() const
{
	return _description;
}

bool ArgumentBase::isMandatory() const
{
	return _isMandatory;
}

bool ArgumentBase::isSet() const
{
	return _isSet;
}

bool ArgumentBase::isComplete() const
{
	return ((!_isMandatory) || (_isMandatory && _isSet));
}

bool ArgumentBase::shouldAbortParsing() const
{
	return _abortParsing;
}

void ArgumentBase::setValueFlag()
{
	_isSet = true;
}

void ArgumentBase::abortParsing()
{
	_abortParsing = true;
}



ArgParser::ArgParser()
{

}

ArgParser::~ArgParser()
{

}

void ArgParser::parse(int argc, char** argv)
{
	//
	bool nextElementIsValue = false;
	ArgumentBase* currentArg = nullptr;

	for(int elementIndex = 1; elementIndex < argc; elementIndex++)
	{
		std::string currentParamElement = std::string(argv[elementIndex]);

		if(currentParamElement.find("--") == 0)
		{
			if(nextElementIsValue && (currentArg != nullptr))
			{
				currentArg->setValueFromStr("true");

				if(currentArg->shouldAbortParsing())
					return;

				nextElementIsValue = false;
			}

			currentArg = getArgByOpt(currentParamElement.substr(2), true);
			if(currentArg == nullptr)
				WThrow(Exception("No argument %s registered", currentParamElement.c_str()));

			//TODO: Check wether this is smart or not.
			nextElementIsValue = true;
		}
		else if(currentParamElement.find("-") == 0)
		{
			if(nextElementIsValue && (currentArg != nullptr))
			{
				currentArg->setValueFromStr("true");

				if(currentArg->shouldAbortParsing())
					return;

				nextElementIsValue = false;
			}

			currentArg = getArgByOpt(currentParamElement.substr(1), false);
			if(currentArg == nullptr)
				WThrow(Exception("No argument %s registered", currentParamElement.c_str()));

			nextElementIsValue = true;
		}
		else
		{
			if(nextElementIsValue)
			{
				if(currentArg != nullptr)
				{
					currentArg->setValueFromStr(currentParamElement);

					if(currentArg->shouldAbortParsing())
						return;

					nextElementIsValue = false;
				}
				else
				{
					WThrow(Exception("Expected value of argument but argument is missing."));
				}
			}
			else
			{
				_nonArgParams.push_back(currentParamElement);
			}
		}
	}

	if(nextElementIsValue && (currentArg != nullptr))
	{
		currentArg->setValueFromStr("true");

		if(currentArg->shouldAbortParsing())
			return;
	}

	//Now check if all arguments that are mandatory are set.

	for(ArgumentBase* currentArg : _arguments)
	{
		if(currentArg->isComplete() == false)
		{
			WThrow(Exception("Argument %s , %s not set", currentArg->getShortOpt().c_str(), currentArg->getLongOpt().c_str()));
		}
	}
}

ArgParser& ArgParser::addArgument(ArgumentBase& argument)
{
	if(!checkDuplicate(argument))
	{
		WThrow(Exception("Argument %s , %s already present", argument.getShortOpt().c_str(), argument.getLongOpt().c_str()));
	}

	_arguments.push_back(std::addressof(argument));

	return *this;
}

const std::list<ArgumentBase*>& ArgParser::getArguments() const
{
	return _arguments;
}

bool ArgParser::checkDuplicate(ArgumentBase& argument)
{
	for(ArgumentBase* currentArgument : _arguments)
	{
		if(!argument.getShortOpt().empty())
		{
			if(argument.getShortOpt() == currentArgument->getShortOpt())
				return false;
		}

		if(argument.getLongOpt() == currentArgument->getLongOpt())
			return false;
	}

	return true;
}

ArgumentBase* ArgParser::getArgByOpt(const std::string& argOpt, bool useLongOpt)
{
	for(ArgumentBase* currentArg : _arguments)
	{
		if(useLongOpt)
		{
			if(currentArg->getLongOpt() == argOpt)
			{
				return currentArg;
			}
		}
		else
		{
			if(currentArg->getShortOpt() == argOpt)
			{
				return currentArg;
			}
		}
	}

	return nullptr;
}



