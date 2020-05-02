///////////////////////////////////////////////////////////////////////
// Hash.h
//
//  Copyright 10/16/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Hash/Hash.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef HASH_HASH_H
#define HASH_HASH_H
#pragma once

#include "Base.h"
#include <string>


namespace eg
{
    /*!
    Simple hash class to convert string into hash value
    */
    class Hash
    {
    public:
        Hash();
        Hash(uint32_t hashValue);
        explicit Hash(const char *pString);

        Hash(const Hash& rhs);
        Hash &operator=(const Hash& rhs);

        void Set(const uint32_t hash);

        bool IsValid() const;
        operator unsigned int() const { return mHashValue; }
        operator uint32_t() const { return mHashValue; }
        operator uint64_t() const { return mHashValue; }
        uint32_t GetValue() const { return mHashValue; }

        const bool operator<(const Hash &rhs) const;
        const bool operator>(const Hash &rhs) const;
        const bool operator<=(const Hash &rhs) const;
        const bool operator>=(const Hash &rhs) const;
        const bool operator==(const Hash &rhs) const;
        const bool operator!=(const Hash &rhs) const;

    private:
        friend std::ostream& operator <<(std::ostream& out, const Hash& value);
        friend std::istream& operator >>(std::istream& in, Hash& value);

        uint32_t MakeHash(const char *pString);
        uint32_t mHashValue;
    };


    inline std::ostream& operator <<(std::ostream& out, const Hash& value)
    {
        //return out << value.mHashValue;
        return out << std::string(" ") << value.mHashValue;
    }

    inline std::istream& operator >>(std::istream& in, Hash& value)
    {
        return in >> value.mHashValue;
    }


} // namespace eg


#define CONVERT_BACKSLASH
#define CASE_INSENSITIVE


namespace eg {


    const uint32_t kInvalidHash = 0xffffffff;
    const uint32_t kHashInit = 0x811c9dc5;
    const uint32_t kHashPrime = 0x01000193;


    inline Hash::Hash() : mHashValue(kInvalidHash)
    {
    }


    inline Hash::Hash(uint32_t hashValue) : mHashValue(hashValue)
    {
    }


    inline Hash::Hash(const Hash &rhs) : mHashValue(rhs.mHashValue)
    {

    }


    inline Hash::Hash(const char *pString)
    {
        mHashValue = MakeHash(pString);
    }


    inline Hash &Hash::operator=(const Hash &rhs)
    {
        if (&rhs != this)
        {
            mHashValue = rhs.mHashValue;
        }
        return *this;
    }


    inline bool Hash::IsValid() const
    {
        return (mHashValue != kInvalidHash);
    }


    inline uint32_t Hash::MakeHash(const char *hashString)
    {
        if (!hashString || !hashString[0])
            return kInvalidHash;

        const unsigned char* string = (const unsigned char*)hashString;
        uint32_t hash = kHashInit;

        while (*string)
        {
            hash *= kHashPrime;

            char c = *string++;

#ifdef CONVERT_BACKSLASH
            if (c == '\\')
            {
                c = '/';
            }
#endif // CONVERT_BACKSLASH

#ifdef CASE_INSENSITIVE
            if ((c >= 'a') && (c <= 'z'))
            {
                c -= 'a' - 'A';
            }
#endif // CASE_INSENSITIVE
            hash ^= (uint32_t)c;
        }
        return hash;
    }


    inline void Hash::Set(const uint32_t hash)
    {
        mHashValue = hash;
    }


    inline const bool Hash::operator<(const Hash &rhs) const
    {
        return mHashValue < rhs.mHashValue;
    }


    inline const bool Hash::operator>(const Hash &rhs) const
    {
        return mHashValue > rhs.mHashValue;
    }


    inline const bool Hash::operator<=(const Hash &rhs) const
    {
        return mHashValue <= rhs.mHashValue;
    }


    inline const bool Hash::operator>=(const Hash &rhs) const
    {
        return mHashValue >= rhs.mHashValue;
    }


    inline const bool Hash::operator==(const Hash &rhs) const
    {
        return mHashValue == rhs.mHashValue;
    }


    inline const bool Hash::operator!=(const Hash &rhs) const
    {
        return (!(*this == rhs));
    }

} // namespace eg

#endif // HASH_HASH_H