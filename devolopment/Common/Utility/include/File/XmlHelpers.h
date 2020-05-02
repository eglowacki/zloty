/////////////////////////////////////////////////////////////////////////
// XmlHelpers.h
//
//  Copyright 7/5/2009 Edgar Glowacki.
//
// NOTES:
//
//
// #include "File/XmlHelpers.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_XML_HELPERS_H
#define FILE_XML_HELPERS_H
#pragma once

#include "Exception/Exception.h"
#include <tinyxml.h>
#include <string>
#include <boost/lexical_cast.hpp>

namespace eg
{
    namespace xml
    {

        //! Get inner value element from name and cast it to T
        //! Throw bad_param exception for any error
        template<typename T>
        T get_value(TiXmlNode *node, const std::string& name)
        {
            if (TiXmlElement *node_elem = node->ToElement())
            {
                TiXmlHandle node_elem_handle(node_elem);
                if (TiXmlText *node_value = node_elem_handle.FirstChild(name).FirstChild().ToText())
                {
                    try
                    {
                        const std::string& value = node_value->Value();
                        return boost::lexical_cast<T>(value);
                    }
                    catch (const boost::bad_lexical_cast& e)
                    {
                        log_error << "Could not lexical_cast for xml node: '" << name << "'. Exception: " << e.what();
                    }
                }
            }

            using namespace boost;
            throw ex::bad_param(str(format("Could not get value for xml node: '%1%'.") % name).c_str());
        }

        //! Set inner value element from T
        //! Throw bad_param exception for any error
        template<typename T>
        void set_value(TiXmlNode *node, const T& value)
        {
            /*
            if (TiXmlElement *node_elem = node->ToElement())
            {
                TiXmlHandle node_elem_handle(node_elem);
                if (TiXmlText *node_value = node_elem_handle.FirstChild(name).FirstChild().ToText())
                {
                    try
                    {
                        const std::string& value = node_value->Value();
                        return boost::lexical_cast<T>(value);
                    }
                    catch (const boost::bad_lexical_cast& e)
                    {
                        log_error << "Could not lexical_cast for xml node: '" << name << "'. Exception: " << e.what();
                    }
                }
            }
            */

            //using namespace boost;
            //throw ex::bad_param(str(format("Could not set value for xml node: '%1%'.") % name).c_str());
        }


        //! Get property from name and cast it to T
        //! Throw bad_param exception for any error
        template<typename T>
        T get_prop(TiXmlNode *node, const std::string& name)
        {
            if (TiXmlElement *node_elem = node->ToElement())
            {
                T value = T();
                int result = node_elem->QueryValueAttribute<T>(name, &value);
                if (result == TIXML_SUCCESS)
                {
                    return value;
                }
            }

            using namespace boost;
            throw ex::bad_param(str(format("Could not get property '%1%' from xml node '%2%'.") % name % node->ValueStr()).c_str());
        }

        template<typename T>
        T get_prop(TiXmlNode *node, const std::string& name, const T& defaultValue)
        {
            if (TiXmlElement *node_elem = node->ToElement())
            {
                T value(defaultValue);
                int result = node_elem->QueryValueAttribute<T>(name, &value);
                if (result == TIXML_SUCCESS || result == TIXML_NO_ATTRIBUTE)
                {
                    return value;
                }
            }

            using namespace boost;
            throw ex::bad_param(str(format("Could not get property '%1%' from xml node '%2%'") % name % node->ValueStr()).c_str());
        }


    } // namespace xml
} // namespace eg

#endif // FILE_XML_HELPERS_H
