///////////////////////////////////////////////////////////////////////
// PropertyDataConversion.h
//
//  Copyright 8/9/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Conversion from and to string and specific types
//
//
//  #include "Property/PropertyDataConversion.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef PROPERTY_PROPERTY_DATA_CONVERSION_H
#define PROPERTY_PROPERTY_DATA_CONVERSION_H
#pragma once

#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "Math/Quat.h"
#include "ObjectID.h"
#include "Logger/Log.h"
#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>


namespace eg
{

    //-------------------------------------------------------------------------------------------------------
    // Convert from String to Value
    //-------------------------------------------------------------------------------------------------------
    // functions for converting string into type T representation
    //! this will convert string value into requested type.
    template <typename T>
    inline T ConvertProperty(const std::string& value)
    {
        T convertedValue;
        std::istringstream iss(value);
        (iss >> convertedValue);

        return convertedValue;
    }

    //-------------------------------------------------------------------------------------------------------
    //! Specialized from std::string to std::string conversion
    template <>
    inline std::string ConvertProperty(const std::string& value)
    {
        return value;
    }

    //-------------------------------------------------------------------------------------------------------
    //! Specialized from std::string to bool
    template <>
    inline bool ConvertProperty(const std::string& value)
    {
        std::string newValue = value;
        if (boost::iequals(newValue, std::string("true")) || boost::iequals(newValue, std::string("1")))
        {
            return true;
        }

        return false;
    }


    //-------------------------------------------------------------------------------------------------------
    //! Specialized from Vector3 to std::string conversion
    template <>
    inline Vector3 ConvertProperty(const std::string& value)
    {
        std::string newValue = value;
        boost::replace_all(newValue, std::string(","), std::string(" "));

        Vector3 convertedValue = v3::ZERO();
        std::istringstream iss(newValue);
        (iss >> convertedValue);

        return convertedValue;
    }

    //-------------------------------------------------------------------------------------------------------
    //! Specialized from std::string to InstanceId conversion
    template <>
    inline component::InstanceId ConvertProperty(const std::string& value)
    {
        std::string newValue = value;
        std::istringstream iss(newValue);
        uint64_t oId;
        uint64_t cId;
        (iss >> oId);
        (iss >> cId);

        return component::InstanceId(static_cast<uint32_t>(oId), static_cast<uint32_t>(cId));
    }

    //-------------------------------------------------------------------------------------------------------
    //! Specialized from Vector4 to std::string conversion
    template <>
    inline Vector4 ConvertProperty(const std::string& value)
    {
        std::string newValue = value;
        boost::replace_all(newValue, std::string(","), std::string(" "));

        Vector4 convertedValue = v4::ZERO();
        std::istringstream iss(newValue);
        (iss >> convertedValue);

        return convertedValue;
    }

    //-------------------------------------------------------------------------------------------------------
    //! Specialized from Quaternion to std::string conversion
    template <>
    inline Quaternion ConvertProperty(const std::string& value)
    {
        std::string newValue = value;
        boost::replace_all(newValue, std::string(","), std::string(" "));

        Quaternion convertedValue = v4::WONE();
        std::istringstream iss(newValue);
        (iss >> convertedValue);

        return convertedValue;
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::vector<component::InstanceId> ConvertProperty(const std::string& value)
    {
        std::vector<component::InstanceId> reValue;

        boost::char_separator<char> sep(" ,");
        boost::tokenizer<boost::char_separator<char> > tokenizer(value, sep);
        std::vector<std::string> result(tokenizer.begin(), tokenizer.end());

        std::vector<std::string>::const_iterator it = result.begin();
        while (it != result.end())
        {
            try
            {
                uint32_t objectIdValue = boost::lexical_cast<uint32_t>(*it);
                uint32_t compTypeValue = boost::lexical_cast<uint32_t>(*(it+1));
                reValue.push_back(component::InstanceId(component::ObjectId(objectIdValue), component::ComponentId(compTypeValue)));
            }
            catch (boost::bad_lexical_cast &)
            {
                log_error << "Could not convert number to InstanceId.";
            }

            it += 2;
        }

        return reValue;
    }


    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::vector<std::string> ConvertProperty(const std::string& value)
    {
        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char> > tokenizer(value, sep);
        std::vector<std::string> reValue(tokenizer.begin(), tokenizer.end());

        return reValue;
    }


    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::pair<std::string, std::string> ConvertProperty(const std::string& value)
    {
        std::pair<std::string, std::string> reValue;
        std::vector<std::string> tmpValue = ConvertProperty<std::vector<std::string> >(value);
        if (!tmpValue.empty())
        {
            reValue.first = tmpValue[0];
            if (tmpValue.size() > 1)
            {
                reValue.second = tmpValue[1];
            }
       }

        return reValue;
    }

    inline std::pair<std::string, std::string> ConvertProperty(const std::string& value, const std::pair<std::string, std::string>& defValue)
    {
        std::pair<std::string, std::string> reValue = ConvertProperty<std::pair<std::string, std::string> >(value);
        if (reValue.first.empty())
        {
            reValue.first = defValue.first;
        }
        if (reValue.second.empty())
        {
            reValue.second = defValue.second;
        }

        return reValue;
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::vector<component::ObjectId> ConvertProperty(const std::string& value)
    {
        std::vector<component::ObjectId> reValue;

        boost::char_separator<char> sep(",");
        boost::tokenizer<boost::char_separator<char> > tokenizer(value, sep);
        std::vector<std::string> result(tokenizer.begin(), tokenizer.end());

        for (std::vector<std::string>::const_iterator it = result.begin(); it != result.end(); ++it)
        {
            try
            {
                uint32_t objectIdValue = boost::lexical_cast<uint32_t>(*it);
                reValue.push_back(component::ObjectId(objectIdValue));
            }
            catch (boost::bad_lexical_cast &)
            {
                log_error << "Could not convert number to ObjectId.";
            }
        }

        return reValue;
    }


    /*
    //-------------------------------------------------------------------------------------------------------
    template <>
    inline wxPoint ConvertProperty(const std::string& value)
    {
        wxPoint p(0, 0);
        std::istringstream is(std::string(value.c_str()));
        is >> p.x >> p.y;

        return p;
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline wxSize ConvertProperty(const std::string& value)
    {
        wxSize s(0, 0);
        std::istringstream is(std::string(value.c_str()));
        is >> s.x >> s.y;

        return s;
    }
    */

    //-------------------------------------------------------------------------------------------------------
    // Convert from Value to String
    //-------------------------------------------------------------------------------------------------------
    // functions for converting Type T to string representation
    //! Convert value of type T to std::string representation
    template <typename T>
    inline std::string ConvertProperty(const T& value)
    {
        std::ostringstream os;
        os << value;
        return os.str();
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::string ConvertProperty(const std::vector<component::InstanceId>& value)
    {
        typedef std::vector<component::InstanceId>::const_iterator it_CI;
        std::ostringstream os;
        for (it_CI it = value.begin(); it != value.end(); ++it)
        {
            os << (*it).first.GetValue() << " " << (*it).second.GetValue();

            if (it + 1 != value.end())
            {
                // only add this if we are not the last entry
                os << ",";
            }
        }

        return os.str();
    }


    //-------------------------------------------------------------------------------------------------------
    //! Specialized from InstanceId to std::string conversion
    template <>
    inline std::string ConvertProperty(const component::InstanceId& value)
    {
        std::ostringstream os;
        os << static_cast<uint64_t>(value.first.GetValue());
        os << " ";
        os << static_cast<uint64_t>(value.second.GetValue());
        return os.str();
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::string ConvertProperty(const std::vector<std::string>& value)
    {
        typedef std::vector<std::string>::const_iterator it_CI;
        std::ostringstream os;
        for (it_CI it = value.begin(); it != value.end(); ++it)
        {
            os << *it;
            if (it + 1 != value.end())
            {
                // only add this if we are not the last entry
                os << ",";
            }
        }

        return os.str();
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::string ConvertProperty(const std::vector<component::ObjectId>& value)
    {
        typedef std::vector<component::ObjectId>::const_iterator it_CI;
        std::ostringstream os;
        for (it_CI it = value.begin(); it != value.end(); ++it)
        {
            os << (*it).GetValue();

            if (it + 1 != value.end())
            {
                // only add this if we are not the last entry
                os << ",";
            }
        }

        return os.str();
    }

    /*
    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::string ConvertProperty(const wxPoint& value)
    {
        std::ostringstream os;
        os << value.x << std::string(" ") << value.y;

        return os.str();
    }

    //-------------------------------------------------------------------------------------------------------
    template <>
    inline std::string ConvertProperty(const wxSize& value)
    {
        std::ostringstream os;
        os << value.x << std::string(" ") << value.y;

        return os.str();
    }
    */

} // namespace eg

#endif // PROPERTY_PROPERTY_DATA_CONVERSION_H

