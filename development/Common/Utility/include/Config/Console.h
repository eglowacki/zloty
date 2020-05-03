/////////////////////////////////////////////////////////////////////
// Console.h
//
//  Copyright 2/27/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Console ala quake
//      \note add support for help keyward and display help text.
//      This needs to be done at Konsole level to minimize user interaction
//      for thinks like that.
//
//
//
//  #include "Config/Console.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef CONFIG_CONSOLE_H
#define CONFIG_CONSOLE_H
#pragma once

#include "Base.h"
#include <string>
#include <vector>
#include <list>

#pragma warning(push)
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include <boost/signal.hpp>
#pragma warning(pop)



namespace eg
{
    namespace config
    {
        //! This enum is passed to sigCommandEcho callback after command execution
        enum KosoleResult
        {
            krOk,   ///< execution of command was OK
            krInfo, ///< info from command execution
            krHelp, ///< help for this command
            krError ///< error from command execution
        };


        //////////////////////////////////////////////////////////////////////////
        // Console item type constants
        //////////////////////////////////////////////////////////////////////////
        enum ConsoleItemType
        {
            CTYPE_NONE,
            CTYPE_UCHAR,
            CTYPE_CHAR,
            CTYPE_CHARP,
            CTYPE_UINT,
            CTYPE_INT,
            CTYPE_FLOAT,
            CTYPE_STRING,
            CTYPE_FUNCTION
        };

        //////////////////////////////////////////////////////////////////////////
        // Console function pointer
        //////////////////////////////////////////////////////////////////////////

        //! Command execute function
        //! \param std::vector<std::string> parameters to this command function
        //! \param KosoleResult [OUT] this will contain result value, it can be one of the KosoleResult enums
        //! \return command output
        //typedef std::string (*console_function)(const std::vector<std::string> &);

        typedef boost::function<std::string (const std::vector<std::string> & /*command_parameters*/, KosoleResult&)> ConsoleFunctionCommand_t;


        //////////////////////////////////////////////////////////////////////////
        // Item structure
        //////////////////////////////////////////////////////////////////////////
        struct ConsoleItem
        {
            ConsoleItem()
            : variable_pointer(0)
            , type(CTYPE_NONE)
            {}

            // item name
            std::string name;

            // item type
            ConsoleItemType type;

            // function callback or pointer to the value
            ConsoleFunctionCommand_t function;
            void *variable_pointer;
        };

        //////////////////////////////////////////////////////////////////////////
        // console class
        //////////////////////////////////////////////////////////////////////////
        class Konsole
        {
        public:
            // constructor & destructor
            Konsole();
            virtual ~Konsole();

            // adds an item to the console
            void AddItem(const std::string& strName, void *pointer, ConsoleItemType type);
            void AddItem(const std::string& strName, ConsoleFunctionCommand_t commandFunction);

            // deletes an item from the console
            void DeleteItem(const std::string & strName);

            // sets the command that is to be executed when no commandline match is found
            void SetDefaultCommand(ConsoleFunctionCommand_t def) {mDefaultCommand = def;}

            // parses command line and executes
            bool Execute(const std::string& commandText);

            //! Triggered for any output of executed commands. String
            //! \paarm KosoleResult - one of the enums specifying how to interpret command results
            //! \param std::string command_name - command_name
            //! \param std::string - command_output_string output from executed command or empty string if none
            boost::signal<void (KosoleResult, const std::string& /*command_name*/, const std::string& /*command_output_string*/)> sigCommandEcho;

            //! Return list of all available commands
            std::vector<std::string> GetCommandNames() const;

        private:
            //! define item database type
            typedef std::list<ConsoleItem> CommandsDB_t;
            //! This is default catcher for any unhanded commands.
            //! It will just enumerate all available commands
            std::string onDefaultCommand(const std::vector<std::string>& command_parameters, KosoleResult& outResult);
            //void onDefaultCommand(KosoleResult result, const std::string& cmd, const std::string& text);
            // command to be executed by default
            ConsoleFunctionCommand_t mDefaultCommand;

            // holds the list of items
            CommandsDB_t mCommands;
        };

    } // namespace config
} // namespace eg

#endif // CONFIG_CONSOLE_H

