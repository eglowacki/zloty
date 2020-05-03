#include "Hash/Hash.h"

namespace
{
    // just to silence linker warning
    int z = 0;
} // namespace

namespace eg {

#if 0

#define CONVERT_BACKSLASH
#define CASE_INSENSITIVE


const uint32_t kInvalidHash = 0xffffffff;
const uint32_t kHashInit = 0x811c9dc5;
const uint32_t kHashPrime = 0x01000193;


Hash::Hash() : mHashValue(kInvalidHash)
{
}


Hash::Hash(uint32_t hashValue) : mHashValue(hashValue)
{
}


Hash::Hash(const Hash &rhs) : mHashValue(rhs.mHashValue)
{

}


Hash::Hash(const char *pString)
{
    mHashValue = MakeHash(pString);
}


Hash &Hash::operator=(const Hash &rhs)
{
    if (&rhs != this)
    {
        mHashValue = rhs.mHashValue;
    }
    return *this;
}


bool Hash::IsValid() const
{
    return (mHashValue != kInvalidHash);
}


uint32_t Hash::MakeHash(const char *hashString)
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


void Hash::Set(const uint32_t hash)
{
    mHashValue = hash;
}


const bool Hash::operator<(const Hash &rhs) const
{
    return mHashValue < rhs.mHashValue;
}


const bool Hash::operator>(const Hash &rhs) const
{
    return mHashValue > rhs.mHashValue;
}


const bool Hash::operator<=(const Hash &rhs) const
{
    return mHashValue <= rhs.mHashValue;
}


const bool Hash::operator>=(const Hash &rhs) const
{
    return mHashValue >= rhs.mHashValue;
}


const bool Hash::operator==(const Hash &rhs) const
{
    return mHashValue == rhs.mHashValue;
}


const bool Hash::operator!=(const Hash &rhs) const
{
    return (!(*this == rhs));
}
#endif // 0

} // namespace eg
