///////////////////////////////////////////////////////////////////////
// IScript.h
//
//  Copyright 11/14/2008 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to Script Object
//
//
//  #include "Interface/IScript.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef INTERFACE_I_SCRIPT_H
#define INTERFACE_I_SCRIPT_H
#pragma once

#include "Plugin/IPluginObject.h"


namespace eg
{
    //! IScript object
    class DLLIMPEXP_UTIL_CLASS IScript : public IPluginObject
    {
    public:
		//! Helper function to return just function name
		//! without any parameters
		//! string format is function_name(param0, param1, ...)
		//! where (param0, param1, ...) is optional.
		static std::string FindFunctionName(const std::string& functionSignature)
		{
			size_t pos = functionSignature.find_first_of('(');
			if (pos != std::string::npos)
			{
				// there is opening parenthesis, which means
				// separation between function name and parameters
				std::string functionName = functionSignature.substr(0, pos);
				return functionName;
			}

			return functionSignature;
		}



		//! Register function name and signature
		//! \param functionSignature full function name and parameters, myFoo(int)
		//! \return true if function registered, or false if not
		//! \note if parameters are empty, then this will also return false
		//!       Functions are keyed on name, so there is no support for function overloading
		virtual bool RegisterFunction(const std::string& functionSignature) = 0;
    };


} // namespace eg

#endif // INTERFACE_I_SCRIPT_H

