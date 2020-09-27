/////////////////////////////////////////////////////////////////////////
// LocationComponent.h
//
//  Copyright 7/27/2016 Edgar Glowacki.
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
#include "Components/Component.h"
#include "HashUtilities.h"


namespace yaget
{
    namespace comp
    {
        class LocationComponent;

        namespace db
        {
            struct Position {};
            struct Orientation {};
            struct Scale {};

            template <>
            struct ComponentProperties<comp::LocationComponent>
            {
                using Row = std::tuple<Position, Orientation, Scale>;
                using Types = std::tuple<math3d::Vector3, math3d::Quaternion, math3d::Vector3>;
                static Types DefaultRow() { return Types{math3d::Vector3{}, math3d::Quaternion{}, math3d::Vector3{1.0f, 1.0f, 1.0f}}; }
            };

        }

        class LocationComponent : public Component
        {
        public:
            LocationComponent(Id_t id, const math3d::Vector3& position = math3d::Vector3(), const math3d::Quaternion& orientation = math3d::Quaternion(), const math3d::Vector3& scale = math3d::Vector3::One);
            virtual ~LocationComponent();

            const math3d::Vector3& GetPosition() const { return mPosition; }
            const math3d::Quaternion& GetOrientation() const { return mOrientation; }
            const math3d::Vector3& GetScale() const { return mScale; }

            // TODO how to make them private, so only Scene can get to it (without using friends)
            void SetPosition(const math3d::Vector3& position) { mPosition = position; mStateHashDirty = true; }
            void SetOrientation(const math3d::Quaternion& orientation) { mOrientation = orientation; mStateHashDirty = true; }
            void SetScale(const math3d::Vector3& scale) { mScale = scale; mStateHashDirty = true; }
            void Set(const math3d::Vector3& position, const math3d::Quaternion& orientation, const math3d::Vector3& scale = math3d::Vector3::One)
            { 
                mPosition = position; 
                mOrientation = orientation; 
                mScale = scale;
                mStateHashDirty = true;
            }

            math3d::Matrix Matrix() const
            {
                return math3d::CreateMatrix(GetPosition(), GetOrientation(), GetScale());
            }

        private:
            size_t CalculateStateHash() const override;

            math3d::Vector3 mPosition;
            math3d::Quaternion mOrientation;
            math3d::Vector3 mScale;
        };


        class LocationComponentPool : public ComponentPool<LocationComponent, 1000>
        {
        public:
            LocationComponentPool()
                : ComponentPool<LocationComponent, 1000>()
            {}

            virtual void Tick(const time::GameClock& /*gameClock*/) override { YAGET_ASSERT(false); };

            struct TokenState
            {
                Id_t mId = 0;
                math3d::Vector3 mPosition;
                math3d::Quaternion mOrientation;
            };

            // NOTE: How do we deal with data from other components to be intertwine with this
            // do we just provide stride?
            //
            // Tokenizes data from location components as an array of TokenState struct,
            // stride is used to offset where the TokenState starts for each element
            // returns number of tokens generated, this can be called with nullptr and 0 to just get number valid tokens
            size_t TokenizeState(char* buffer, size_t size, size_t stride = sizeof(LocationComponentPool::TokenState), std::function<bool(comp::Id_t id)> isValidCB = [](comp::Id_t /*id*/) { return true; });
            Ptr New(Id_t id);
        };

    } // namespace comp
} // namespace yaget


namespace std
{
    // Has function for LocationComponent
    template <>
    struct hash<yaget::comp::LocationComponent>
    {
        size_t operator()(const yaget::comp::LocationComponent& location) const
        {
            const math3d::Vector3& loc = location.GetPosition();
            const math3d::Quaternion& orient = location.GetOrientation();
            const math3d::Vector3& scale = location.GetScale();
            return yaget::conv::GenerateHash(loc, orient, scale);
        }
    };

} // namespace std


