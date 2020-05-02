///////////////////////////////////////////////////////////////////////
// ICollision.h
//
//  Copyright 10/8/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to render Object
//
//
//  #include "Interface/ICollision.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef INTERFACE_I_COLLISION_H
#define INTERFACE_I_COLLISION_H
#pragma once



#include "Hash/Hash.h"
#include "Plugin/IPluginObject.h"
#include "MessageInterface.h"
//#include "Registrate.h"
#include "ObjectID.h"
#include "Math/Ray.h"
#include "Math/Matrix.h"
#include <vector>


namespace eg
{
    namespace collision
    {
        //! This is passed to any collision methods and specify
        //! what kind of collision user needs
        struct Request
        {
            Request()
            : lenght(0), allHits(false)
            {}

            //! Ray to test other object against
            Ray ray;
            //! If this is 0, then ray is infinite
            //! otherwise it specifies length of the ray
            float lenght;
            //! if true, then return all hit object
            //! otherwise closest to Ray origin.
            bool allHits;
        };

        //! Any collision hits will generate this response structure
        struct Response
        {
            //! Represents each individual hit
            struct Record
            {
                Record() : oId(), hitPoint(v3::ZERO()), hitNormal(v3::YONE()) {}

                component::ObjectId oId;
                Vector3 hitPoint;
                Vector3 hitNormal;
            };

            //! list of hit objects which where included in collision
            //! \note we would need to have another structure within vector
            //! to provide more data per each individual hit
            std::vector<Record> Hits;
        };

    } // namespace collision

    //! ICollision object
    class DLLIMPEXP_UTIL_CLASS ICollision : public IPluginObject
    {
    public:
        //! Return list of objects hit by ray.
        //! \param ray ray in world coordinates
        //! \param bAllHits if true, then return all objects along the hit ray ordered by distance
        //!        first will be closest to ray origin, if false then return the closest one
        //! \return Response structure
        virtual collision::Response HitObjects(const collision::Request& request) const = 0;
    };


} // namespace eg

#endif // INTERFACE_I_COLLISION_H

