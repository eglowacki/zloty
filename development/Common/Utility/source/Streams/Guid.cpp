/*
The MIT License (MIT)

Copyright (c) 2014 Graeme Hill (http://graemehill.ca)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "Streams/Guid.h"


#ifdef GUID_WINDOWS
#include <objbase.h>
#endif
             

namespace
{
    // converts a single hex char to a number (0 - 15)
    unsigned char hexDigitToChar(char ch)
    {
        // 0-9
        if (ch > 47 && ch < 58)
            return ch - 48;

        // a-f
        if (ch > 96 && ch < 103)
            return ch - 87;

        // A-F
        if (ch > 64 && ch < 71)
            return ch - 55;

        return 0;
    }

    bool isValidHexChar(char ch)
    {
        // 0-9
        if (ch > 47 && ch < 58)
            return true;

        // a-f
        if (ch > 96 && ch < 103)
            return true;

        // A-F
        if (ch > 64 && ch < 71)
            return true;

        return false;
    }

    // converts the two hexadecimal characters to an unsigned char (a byte)
    unsigned char hexPairToChar(char a, char b)
    {
        return hexDigitToChar(a) * 16 + hexDigitToChar(b);
    }

} // namespace


// overload << so that it's easy to convert to a string
std::ostream &yaget::operator<<(std::ostream &s, const yaget::Guid &guid)
{
    return s << std::hex << std::setfill('0')
        << std::setw(2) << (int)guid._bytes[0]
        << std::setw(2) << (int)guid._bytes[1]
        << std::setw(2) << (int)guid._bytes[2]
        << std::setw(2) << (int)guid._bytes[3]
        << "-"
        << std::setw(2) << (int)guid._bytes[4]
        << std::setw(2) << (int)guid._bytes[5]
        << "-"
        << std::setw(2) << (int)guid._bytes[6]
        << std::setw(2) << (int)guid._bytes[7]
        << "-"
        << std::setw(2) << (int)guid._bytes[8]
        << std::setw(2) << (int)guid._bytes[9]
        << "-"
        << std::setw(2) << (int)guid._bytes[10]
        << std::setw(2) << (int)guid._bytes[11]
        << std::setw(2) << (int)guid._bytes[12]
        << std::setw(2) << (int)guid._bytes[13]
        << std::setw(2) << (int)guid._bytes[14]
        << std::setw(2) << (int)guid._bytes[15];
}


bool yaget::Guid::IsValid() const
{
    Guid empty;
    return *this != empty;
}


// convert to string using std::snprintf() and std::string
std::string yaget::Guid::str() const
{
    char one[10], two[6], three[6], four[6], five[14];

    snprintf(one, 10, "%02x%02x%02x%02x",
        _bytes[0], _bytes[1], _bytes[2], _bytes[3]);
    snprintf(two, 6, "%02x%02x",
        _bytes[4], _bytes[5]);
    snprintf(three, 6, "%02x%02x",
        _bytes[6], _bytes[7]);
    snprintf(four, 6, "%02x%02x",
        _bytes[8], _bytes[9]);
    snprintf(five, 14, "%02x%02x%02x%02x%02x%02x",
        _bytes[10], _bytes[11], _bytes[12], _bytes[13], _bytes[14], _bytes[15]);
    const std::string sep("-");
    std::string out(one);

    out += sep + two;
    out += sep + three;
    out += sep + four;
    out += sep + five;

    return out;
}


// conversion operator for std::string
yaget::Guid::operator std::string() const
{
    return str();
}


// Access underlying bytes
const yaget::Guid::DataBuffer& yaget::Guid::bytes() const
{
    return _bytes;
}


// create a guid from vector of bytes
yaget::Guid::Guid(const std::array<unsigned char, 16> &bytes) : _bytes(bytes)
{}


// create a guid from array of bytes
yaget::Guid::Guid(const unsigned char *bytes)
{
    std::copy(bytes, bytes + 16, std::begin(_bytes));
}


// create a guid from string
yaget::Guid::Guid(const std::string &fromString)
{
    char charOne = '\0';
    char charTwo = '\0';
    bool lookingForFirstChar = true;
    unsigned nextByte = 0;

    for (const char &ch : fromString)
    {
        if (ch == '-')
            continue;

        if (nextByte >= 16 || !isValidHexChar(ch))
        {
            // Invalid string so bail
            zeroify();
            return;
        }

        if (lookingForFirstChar)
        {
            charOne = ch;
            lookingForFirstChar = false;
        }
        else
        {
            charTwo = ch;
            auto byte = hexPairToChar(charOne, charTwo);
            _bytes[nextByte++] = byte;
            lookingForFirstChar = true;
        }
    }

    // if there were fewer than 16 bytes in the string then guid is bad
    if (nextByte < 16)
    {
        zeroify();
        return;
    }
}


// create empty guid
yaget::Guid::Guid() : _bytes{ 0 }
{}


// copy constructor
yaget::Guid::Guid(const Guid &other) : _bytes(other._bytes)
{}


// set all bytes to zero
void yaget::Guid::zeroify()
{
    std::fill(_bytes.begin(), _bytes.end(), '\0');
}


// overload assignment operator
yaget::Guid &yaget::Guid::operator=(const Guid &other)
{
    Guid(other).swap(*this);
    return *this;
}


// overload equality operator
bool yaget::Guid::operator==(const Guid &other) const
{
    return _bytes == other._bytes;
}


// overload inequality operator
bool yaget::Guid::operator!=(const Guid &other) const
{
    return !((*this) == other);
}

// overload less then operator
bool yaget::Guid::operator<(const Guid &other) const
{
    return _bytes < other._bytes;
}

// member swap function
void yaget::Guid::swap(Guid &other)
{
    _bytes.swap(other._bytes);
}


// obviously this is the windows version
#ifdef GUID_WINDOWS
yaget::Guid yaget::NewGuid()
{
    GUID newId;
    (void)CoCreateGuid(&newId);

    std::array<unsigned char, 16> bytes =
    {
        (unsigned char)((newId.Data1 >> 24) & 0xFF),
        (unsigned char)((newId.Data1 >> 16) & 0xFF),
        (unsigned char)((newId.Data1 >> 8) & 0xFF),
        (unsigned char)((newId.Data1) & 0xff),

        (unsigned char)((newId.Data2 >> 8) & 0xFF),
        (unsigned char)((newId.Data2) & 0xff),

        (unsigned char)((newId.Data3 >> 8) & 0xFF),
        (unsigned char)((newId.Data3) & 0xFF),

        (unsigned char)newId.Data4[0],
        (unsigned char)newId.Data4[1],
        (unsigned char)newId.Data4[2],
        (unsigned char)newId.Data4[3],
        (unsigned char)newId.Data4[4],
        (unsigned char)newId.Data4[5],
        (unsigned char)newId.Data4[6],
        (unsigned char)newId.Data4[7]
    };

    return bytes;
}
#endif


// Specialization for std::swap<Guid>() --
// call member swap function of lhs, passing rhs
namespace std
{
    template <>
    void swap(yaget::Guid &lhs, yaget::Guid &rhs)
    {
        lhs.swap(rhs);
    }
}
