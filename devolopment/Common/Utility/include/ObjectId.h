///////////////////////////////////////////////////////////////////////
// ObjectID.h
//
//  Copyright 10/16/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "ObjectID.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef OBJECT_ID_H
#define OBJECT_ID_H
#pragma once

#include <Hash/Hash.h>
#include <map>
#include <boost/lexical_cast.hpp>


namespace eg
{
    class IComponent;

    //! New id system for components, where ObjectId will be converted to uint64_t.
    namespace component
    {
        //! Represents unique id of collection of IComponent's, like object, group, etc.
        typedef Hash ObjectId;

        //! Represents Component Type which acutely is Hash of IComponent derived class name.
        typedef Hash ComponentId;

        //! Represents unique IComponent with-in object collection.
        typedef std::pair<ObjectId, ComponentId> InstanceId;

        //! Helper function to return InstanceId object based in build in types
        inline InstanceId make(uint64_t oId, uint64_t cId)
        {
            return InstanceId(ObjectId(static_cast<uint32_t>(oId)), ComponentId(static_cast<uint32_t>(cId)));
        }

        inline InstanceId make(uint64_t oId, const ComponentId& cId)
        {
            return InstanceId(ObjectId(static_cast<uint32_t>(oId)), cId);
        }

        inline std::string make_string(const InstanceId& iId)
        {
            return "{" + boost::lexical_cast<std::string>(iId.first.GetValue()) + ":" + boost::lexical_cast<std::string>(iId.second.GetValue()) + "}";
        }


        inline std::string make_string(uint64_t oId, uint32_t cId)
        {
            return make_string(make(oId, cId));
        }

        inline InstanceId ZER0()
        {
            return InstanceId();
        }

    } // namespace component

} // namespace eg

#endif //OBJECT_ID_H