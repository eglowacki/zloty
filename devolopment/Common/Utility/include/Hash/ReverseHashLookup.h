///////////////////////////////////////////////////////////////////////
// ReverseHashLookup.h
//
//  Copyright 1/21/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides templatized class to reverse uniqu id (like hash)
//      and resolve to names
//
//
//  #include "Hash/ReverseHashLookup.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef HASH_REVERSE_HASH_LOOKUP_H
#define HASH_REVERSE_HASH_LOOKUP_H
#pragma once

#include "Base.h"
#include <map>


namespace eg
{

    /*!
    This allows us to reverse unique id to name
    */
    template <typename T>
    class ReverseHashLookup
    {
    public:

        template <typename T>
        void Add(const std::string& textName, const T& uniqueID)
        {
            if (mReverseMap.find(uniqueID) == mReverseMap.end())
            {
                mReverseMap.insert(std::make_pair(uniqueID, textName));
            }
        }

        template <typename T>
        void Remove(const T& uniqueID)
        {
            it_RM it = mReverseMap.find(uniqueID);
            if (it != mReverseMap.end())
            {
                mReverseMap.erase(it);
            }
        }

        template <typename T>
        std::string GetName(const T& uniqueID) const
        {
            cit_RM it = mReverseMap.find(uniqueID);
            if (it != mReverseMap.end())
            {
                return (*it).second;
            }

            return "";
        }

    private:
        typedef std::map<T, std::string> ReverseMap_t;
        typedef typename ReverseMap_t::iterator it_RM;
        typedef typename ReverseMap_t::const_iterator cit_RM;

        ReverseMap_t mReverseMap;
    };

} // namespace eg

#endif // HASH_REVERSE_HASH_LOOKUP_H

