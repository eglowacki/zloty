///////////////////////////////////////////////////////////////////////
// StringHelperss.h
//
//  Copyright 11/28/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      String utilities and helper functions.
//
//
//  #include "StringHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Base.h"
#include "Fmt/format.h"
#include "MathFacade.h"
#include "Streams/Guid.h"
#include <vector>
#include <algorithm>
#include <sstream>


namespace yaget
{
    namespace conv
    {
        //----------------------------------------------------------------------------------------------------------------------------------
        inline void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
        {
            if (from.empty())
            {
                return;
            }

            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos)
            {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        inline std::string ReplaceString(std::string source, const std::string& search, const std::string& replace)
        {
            size_t pos = 0;
            while ((pos = source.find(search, pos)) != std::string::npos)
            {
                source.replace(pos, search.length(), replace);
                pos += replace.length();
            }

            return source;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        template <typename T>
        inline std::string ToThousandsSep(T value)
        {
            struct thousands_sep : std::numpunct<char>
            {
                char do_thousands_sep()   const { return ','; }  // separate with commas
                std::string do_grouping() const { return "\3"; } // groups of 3 digits
                static void imbue(std::ostream &os)
                {
                    os.imbue(std::locale(os.getloc(), new thousands_sep));
                }
            };

            std::stringstream ss;
            thousands_sep::imbue(ss);
            ss << value;
            return ss.str();
        }

        // Convert wide Unicode String to UTF8 string
        std::string wide_to_utf8(const wchar_t* wstr);
        // Convert an UTF8 string to a wide Unicode String
        std::wstring utf8_to_wide(const std::string &str);

        //----------------------------------------------------------------------------------------------------------------------------------
        inline std::vector<std::string> Split(const std::string& theString, const std::string& theDelimiter)
        {
            if (theString.empty())
            {
                return std::vector<std::string>();
            }
            else if (theDelimiter.empty())
            {
                return { theString };
            }

            std::vector<std::string> theStringVector;
            size_t  start = 0, end = 0;

            while (end != std::string::npos)
            {
                end = theString.find(theDelimiter, start);

                // If at end, use length=maxLength.  Else use length=end-start.
                theStringVector.push_back(theString.substr(start, (end == std::string::npos) ? std::string::npos : end - start));

                // If at end, use start=maxSize.  Else use start=end+delimiter.
                start = ((end > (std::string::npos - theDelimiter.size())) ? std::string::npos : end + theDelimiter.size());
            }

            return theStringVector;
        }

        ////----------------------------------------------------------------------------------------------------------------------------------
        template<typename T>
        std::string Combine(const T& values, const char* delimeter);

        //----------------------------------------------------------------------------------------------------------------------------------
        inline int64_t Atoll(const char* value)
        {
            return value ? atoll(value) : 0;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        inline uint64_t Atoull(const char* value)
        {
            return value ? std::stoull(value) : 0;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        inline int Atoi(const char* value)
        {
            return value ? atoi(value) : 0;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        // ascii to unsigned int
        inline uint32_t Atoui(const char* value)
        {
            return value ? static_cast<uint32_t>(std::stoul(value)) : 0;
        }
        
        //----------------------------------------------------------------------------------------------------------------------------------
        inline float Atof(const char* value)
        {
            return value ? static_cast<float>(atof(value)) : 0;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        inline bool Atob(const char* value)
        {
            return value && (value[0] == '1' || value[0] == 't' || value[0] == 'T');
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        typedef struct unused_marker
        {
            bool operator==(const unused_marker& /*other*/) const
            {
                return true;
            }
        } unused_marker_t;

        //----------------------------------------------------------------------------------------------------------------------------------
        template <typename T>
        struct Convertor;

        namespace internal
        {
            // Format for parsing, r, g, b and a is not used in parsing,
            // it looks only for []=,
            // it works with 1 or more elements
            // [r = { :.2f }, g = { :.2f }, b = { :.2f }, a = { :.2f }]
            template<typename T>
            std::vector<T> ParseValues(const std::string& text)
            {
                using R = std::vector<T>;
                using size_type = std::string::size_type;
                constexpr auto npos = std::string::npos;

                R results;

                if (!text.empty() && text.front() == '[' && text.back() == ']')
                {
                    size_type startingPosition = 0;
                    do
                    {
                        size_type startNumber = text.find('=', startingPosition);
                        size_type endNumber = text.find(',', startNumber);
                        if (startNumber != npos && endNumber == npos)
                        {
                            endNumber = text.find(']', startNumber);
                        }

                        if (startNumber == npos || endNumber == npos)
                        {
                            break;
                        }

                        std::string value(text.begin() + startNumber + 1, text.begin() + endNumber);

                        results.push_back(yaget::conv::Convertor<T>::FromString(value.c_str()));

                        startingPosition = endNumber;

                    } while (true);

                }

                return results;
            }

        } // namespace internal

        //----------------------------------------------------------------------------------------------------------------------------------
        inline std::string ToLower(const std::string& source)
        {
            std::string value(source.size(), ' ');
            std::transform(source.begin(), source.end(), value.begin(), [](const char& v)
            {
                return static_cast<char>(::tolower(v));
            });
            return value;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        inline std::string ToUpper(const std::string& source)
        {
            std::string value(source.size(), ' ');
            std::transform(source.begin(), source.end(), value.begin(), [](const char& v)
            {
                return static_cast<char>(::toupper(v));
            });
            return value;
        }

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<unused_marker_t>
        {
            static unused_marker_t FromString(const char* /*value*/)
            {
                return unused_marker_t();
            }

            static std::string ToString(unused_marker_t /*value*/)
            {
                return "";
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<char>
        {
            static char FromString(const char* value)
            {
                return value ? value[0] : 0;
            }

            static std::string ToString(char value)
            {
                char ReturnedValue[2];
                ReturnedValue[0] = value;
                ReturnedValue[1] = '\0';
                return std::string(ReturnedValue);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<uint8_t>
        {
            static uint8_t FromString(const char* value)
            {
                return static_cast<uint8_t>(yaget::conv::Atoui(value));
            }

            static std::string ToString(uint8_t value)
            {
                return std::to_string(value);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<int>
        {
            static int FromString(const char* value)
            {
                return yaget::conv::Atoi(value);
            }

            static std::string ToString(int value)
            {
                return std::to_string(value);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<uint32_t>
        {
            static uint32_t FromString(const char* value)
            {
                return yaget::conv::Atoui(value);
            }

            static std::string ToString(uint32_t value)
            {
                return std::to_string(value);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<size_t>
        {
            static size_t FromString(const char* value)
            {
                return yaget::conv::Atoull(value);
            }

            static std::string ToString(size_t value)
            {
                return std::to_string(value);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<bool>
        {
            static bool FromString(const char* value)
            {
                return yaget::conv::Atob(value);
            }

            static std::string ToString(bool value)
            {
                return value ? "1" : "0";
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<float>
        {
            static float FromString(const char* value)
            {
                return value ? ::yaget::conv::Atof(value) : 0.0f;
            }

            static std::string ToString(float value)
            {
                return fmt::format("{:.2f}", value);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<std::string>
        {
            static std::string FromString(const char* value)
            {
                return std::string(value);
            }

            static std::string ToString(const std::string& value)
            {
                return value;
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<const char*>
        {
            static const char* FromString(const char* value)
            {
                return value;
            }

            static std::string ToString(const char* value)
            {
                return std::string(value ? value : "");
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<Guid>
        {
            static Guid FromString(const char* value)
            {
                return value ? Guid(value) : NewGuid();
            }

            static std::string ToString(Guid value)
            {
                return value.str();
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<std::vector<std::string>>
        {
            static std::vector<std::string> FromString(const char* value)
            {
                return value ? conv::Split(value, ",") : std::vector<std::string>();
            }

            static std::string ToString(const std::vector<std::string>& value)
            {
                return conv::Combine(value, ",");
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template<> 
        struct Convertor<math3d::Vector3>
        {
            static math3d::Vector3 FromString(const char* value)
            {
                std::vector<float> values = internal::ParseValues<float>(value ? value : "");
                math3d::Vector3 v(&values[0]);
                return v;
            }
            static std::string ToString(const math3d::Vector3& value)
            {
                return fmt::format("[x = {:.2f}, y = {:.2f}, z = {:.2f}]", value.x, value.y, value.z);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template<> 
        struct Convertor<math3d::Vector2>
        {
            static math3d::Vector2 FromString(const char* value)
            {
                std::vector<float> values = internal::ParseValues<float>(value ? value : "");
                math3d::Vector2 v(&values[0]);
                return v;
            }
            static std::string ToString(const math3d::Vector2& value)
            {
                return fmt::format("[x = {:.2f}, y = {:.2f}]", value.x, value.y);
            }
        };

        //----------------------------------------------------------------------------------------------------------------------------------
        template<typename V1, typename V2>
        struct Convertor<std::pair<V1, V2>>
        {
            using ValueT = std::pair<V1, V2>;
            static ValueT FromString(const char* value)
            {
                ValueT v{};
                std::string text(value);
                Strings tokens = conv::Split(text, "x");
                if (!tokens.empty())
                {
                    v.first = Convertor<V1>::FromString(tokens[0].c_str());

                    if (tokens.size() > 1)
                    {
                        v.second = Convertor<V1>::FromString(tokens[1].c_str());
                    }
                }

                return v;
            }
            static std::string ToString(const ValueT& value)
            {
                return Convertor<V1>::ToString(value.first) + "x" + Convertor<V2>::ToString(value.second);
            }
        };


        
        //----------------------------------------------------------------------------------------------------------------------------------
        template<> 
        struct Convertor<math3d::Color>
        {
            static math3d::Color FromString(const char* value)
            {
                std::vector<float> values = internal::ParseValues<float>(value ? value : "");
                math3d::Color v(&values[0]);
                return v;
            }
            static std::string ToString(const math3d::Color& value)
            {
                return fmt::format("[r = {:.2f}, g = {:.2f}, b = {:.2f}, a = {:.2f}]", value.x, value.y, value.z, value.w);
            }
        };

        //size_t CalcSize(const std::string& nameType);

        inline std::string safe(const char* value) { return value ? value : ""; }

        //----------------------------------------------------------------------------------------------------------------------------------
        template<typename T>
        std::string Combine(const T& values, const char* delimeter)
        {
            //using std::to_string;
            std::string result;

            auto it_end = std::end(values);
            const size_t num = values.size();
            for (auto it = std::begin(values); it != it_end; ++it)
            {
                result += conv::Convertor<T::value_type>::ToString(*it) + ((delimeter && num > 1 && std::distance(it, it_end) > 1) ? delimeter : "");
            }

            return result;
        }

    } // namespace conv

    //! compare if sourceString can match filterString pattern
    //! c:/folder/bar.txt
    //! *.txt, b*.txt, ba?.txt
    bool WildCompare(const std::string& filterString, const std::string& sourceString);
    inline bool WildCompareI(const std::string& filterString, const std::string& sourceString)
    {
        std::string pattern = conv::ToLower(filterString);
        std::string fullString = conv::ToLower(sourceString);
        return WildCompare(pattern, fullString);
    }

    //! Compare 2 strings case insensitive
    inline bool CompareI(const std::string& elem1, const std::string& elem2)
    {
        return conv::ToLower(elem1) == conv::ToLower(elem2);
    }

    //inline bool WildCompareISafe(const std::string& wild, const std::string& string)
    //{
    //    std::string pattern = NormalizePath(wild, false, false);
    //    std::string fullString = NormalizePath(string, false, false);
    //    return WildCompareI(pattern, fullString);
    //}


    ////! Return string which will have ending path seperator added if one does not exist
    //std::string NormalizePath(const std::string& pathName, bool bAddSeperator = true, bool bCheckAsWild = true);

    ////! Checks if pString null or pString[0] is null
    //bool IsNull(const char *pString);

    ////! Retunr true if text has any of DOS wild chars' ('*', '?') only so far
    //bool IsWildString(const std::string& text);

    ////! Return TRUE if string matches wild pattern which follows DOS style wild card file format
    //bool WildCompare(const char *wild, const char *string);

    //inline bool WildCompare(const std::string& wild, const std::string& string)
    //{
    //    return WildCompare(wild.c_str(), string.c_str());
    //}

    //inline bool WildCompareI(const std::string& wild, const std::string& string)
    //{
    //    std::string pattern = boost::to_lower_copy(wild);
    //    std::string fullString = boost::to_lower_copy(string);
    //    return WildCompare(pattern.c_str(), fullString.c_str());
    //}

    //inline bool WildCompareISafe(const std::string& wild, const std::string& string)
    //{
    //    std::string pattern = NormalizePath(wild, false,  false);
    //    std::string fullString = NormalizePath(string, false,  false);
    //    return WildCompareI(pattern, fullString);
    //}

    ////! Helper function to convert string into number
    ////! string can be in hex form (0x...) or decimal notation
    //template <class T>
    //T from_string(const std::string& s)
    //{
    //    std::istringstream iss(s);
    //    T t(0);
    //    if (boost::istarts_with(s, std::string("0x")))
    //    {
    //        (iss >> std::hex >> t);
    //    }
    //    else
    //    {
    //        (iss >> std::dec >> t);
    //    }
    //    return t;
    //}

    //// ------------------------------------------------------------
    //// inline implementation
    //inline bool IsNull(const char *pString)
    //{
    //    // return true if pString is NULL or pString points to NULL string
    //    return (pString && *pString != 0) == false;
    //}
} // namespace yaget
