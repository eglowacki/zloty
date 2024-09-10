
/////////////////////////////////////////////////////////////////////////
// LocationComponent.h
//
//  Copyright 6/30/2024 Edgar Glowacki.
//
// NOTES:
//      Location components, which represents position and orientation
//
//
// #include "Components/LocationComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "MathFacade.h"

#include "Components/PersistentBaseComponent.h"

namespace yaget::comp
{
    namespace db_location
    {
        struct Position { using Types = math3d::Vector3; };
        struct Orientation { using Types = math3d::Quaternion; };
        struct Scale { using Types = math3d::Vector3; };

        using ValueTypes = std::tuple<Position, Orientation, Scale>;

    } // namespace db_location


    class LocationComponent3 : public db::PersistentBaseComponent<db_location::ValueTypes>
    {
    public:
        LocationComponent3(Id_t id, const math3d::Vector3& position = math3d::Vector3::Zero, const math3d::Quaternion orientation = math3d::Quaternion::Identity, const math3d::Vector3& scale = math3d::Vector3::One)
            : PersistentBaseComponent(id, std::tie(position, orientation, scale))
        {
        }

        math3d::Matrix Matrix() const
        {
            return math3d::CreateMatrix(GetValue<db_location::Position>(), GetValue<db_location::Orientation>(), GetValue<db_location::Scale>());
        }
        
    };

} // namespace yaget::comp
