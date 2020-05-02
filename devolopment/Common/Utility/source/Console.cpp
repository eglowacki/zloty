#include "Config/Console.h"
#include "Logger/Log.h"
#include <sstream>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>


namespace
{

    //! Parse line into tokens separated by one or more spaces
    //! Use "some text with embedded spaces" to treat that as a one token
    std::vector<std::string> commandParser(const std::string& line)
    {
        boost::escaped_list_separator<char> sep('\\', ' ', '\"');
        boost::tokenizer<boost::escaped_list_separator<char> > tokenizer(line, sep);

        std::vector<std::string> result(tokenizer.begin(), tokenizer.end());
        return result;
    }

} // namespace

namespace eg {
namespace config {


//////////////////////////////////////////////////////////////////////////
// console default construtor & destructor
//////////////////////////////////////////////////////////////////////////

Konsole::Konsole()
{
    log_trace(tr_util) << "Konsole object created.";
    mDefaultCommand = boost::bind(&Konsole::onDefaultCommand, this, _1, _2);
}


Konsole::~Konsole()
{
    log_trace(tr_util) << "Konsole object deleted.";
}


bool Konsole::Execute(const std::string& commandText)
{
    if (commandText.empty())
    {
        return false;
    }

    log_debug << "Executing konsole command: '" << commandText << "'.";

    // tokenize
    std::vector<std::string> arguments = commandParser(commandText);

    // execute
    std::ostringstream out;
    for (CommandsDB_t::const_iterator iter = mCommands.begin(); iter != mCommands.end(); ++iter)
    {
        if ((*iter).name == arguments[0])
        {
            switch ((*iter).type)
            {
                case CTYPE_UCHAR:
                    if (arguments.size() > 2)
                    {
                        return true;
                    }
                    else if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << *((unsigned char *)(*iter).variable_pointer);
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    else if (arguments.size() == 2)
                    {
                        *((unsigned char *)(*iter).variable_pointer) = (unsigned char) atoi(arguments[1].c_str());
                        return true;
                    }
                    break;

                case CTYPE_CHAR:
                    if (arguments.size() > 2)
                    {
                        return true;
                    }
                    else if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << *((char *)(*iter).variable_pointer);
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    else if (arguments.size() == 2)
                    {
                        *((char *)(*iter).variable_pointer) = (char) atoi(arguments[1].c_str());
                        return true;
                    }
                    break;

                case CTYPE_UINT:
                    if (arguments.size() > 2)
                    {
                        return true;
                    }
                    else if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << *((unsigned int *)(*iter).variable_pointer);
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    if (arguments.size() == 2)
                    {
                        *((unsigned int *)(*iter).variable_pointer) = (unsigned int) atoi(arguments[1].c_str());
                        return true;
                    }
                    break;

                case CTYPE_INT:
                    if (arguments.size() > 2)
                    {
                        return true;
                    }
                    else if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << *((int *)(*iter).variable_pointer);
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    if (arguments.size() == 2)
                    {
                        *((int *)(*iter).variable_pointer) = (int) atoi(arguments[1].c_str());
                        return true;
                    }
                    break;

                case CTYPE_FLOAT:
                    if (arguments.size() > 2)
                    {
                        return true;
                    }
                    else if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << *((float *)(*iter).variable_pointer);
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    if (arguments.size() == 2)
                    {
                        *((float *)(*iter).variable_pointer) = (float)atof(arguments[1].c_str());
                        return true;
                    }
                    break;

                case CTYPE_STRING:
                    if (arguments.size() == 1)
                    {
                        out.str("");
                        out << (*iter).name << " = " << (std::string *)(*iter).variable_pointer;
                        sigCommandEcho(krInfo, "", out.str());
                        return true;
                    }
                    else if (arguments.size() > 1)
                    {
                        // reset variable
                        *((std::string *)(*iter).variable_pointer) = "";

                        // add new string
                        for (uint32_t i = 0; i < arguments.size(); ++i)
                        {
                            *((std::string *)(*iter).variable_pointer) += arguments[i];
                        }
                        return true;
                    }
                    break;

                case CTYPE_FUNCTION:
                    {
                        KosoleResult outResult = krOk;
                        std::string commandOutput = (*iter).function(arguments, outResult);
                        sigCommandEcho((KosoleResult)outResult, arguments[0], commandOutput);
                    }
                    return true;

                    /*
                default:
                    if (mDefaultCommand)
                    {
                        KosoleResult outResult = krOk;
                        std::string commandOutput = mDefaultCommand(arguments, outResult);
                        sigCommandEcho((KosoleResult)outResult, arguments[0], commandOutput);
                    }
                    return false;
                    */
            }
        }
    }

    if (mDefaultCommand)
    {
        KosoleResult outResult = krHelp;
        std::string commandOutput = mDefaultCommand(arguments, outResult);
        sigCommandEcho(outResult, "/help", commandOutput);
    }
    else
    {
        sigCommandEcho(krError, arguments[0], "No handler registered for '" + arguments[0] + "' command.");
    }
    return false;
}


std::string Konsole::onDefaultCommand(const std::vector<std::string>& /*command_parameters*/, KosoleResult& outResult)
{
    outResult = config::krHelp;
    std::string resultString = "Valid commands:";
    for (CommandsDB_t::const_iterator iter = mCommands.begin(); iter != mCommands.end(); ++iter)
    {
        resultString += "\n\t" + (*iter).name;
    }

    return resultString;
}


void Konsole::AddItem(const std::string & strName, void *pointer, ConsoleItemType type)
{
    log_trace(tr_util) << "Adding konsole item command handler '" << strName << "' for type '" << type << "'.";
    ConsoleItem it;

    // fill item properties
    it.name = strName;
    it.type = type;

    // address
    if (type != CTYPE_FUNCTION)
    {
        it.variable_pointer = pointer;
    }

    // add item
    mCommands.push_back(it);
}


void Konsole::AddItem(const std::string& strName, ConsoleFunctionCommand_t commandFunction)
{
    log_trace(tr_util) << "Adding konsole fuction command handler '" << strName << "'.";

    ConsoleItem it;
    // fill item properties
    it.name = "/" + strName;
    // all command are start with /
    it.type = CTYPE_FUNCTION;
    it.function = commandFunction;

    // add item
    mCommands.push_back(it);
}


void Konsole::DeleteItem(const std::string & strName)
{
    log_trace(tr_util) << "Removing konsole command handler '" << strName << "'.";

    // find item
    for (CommandsDB_t::iterator iter = mCommands.begin(); iter != mCommands.end(); ++iter)
    {
        if ((*iter).name == strName || (*iter).name == std::string("/" + strName))
        {
            mCommands.erase(iter);
            break;
        }
    }
}


std::vector<std::string> Konsole::GetCommandNames() const
{
    std::vector<std::string> commands;
    for (CommandsDB_t::const_iterator it = mCommands.begin(); it != mCommands.end(); ++it)
    {
        commands.push_back((*it).name);
    }

    return commands;
}


} // namespace config
} // namespace eg


