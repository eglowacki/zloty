///////////////////////////////////////////////////////////////////////
// DefensorGameTypes.h
//
//  Copyright 06/26/2024 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Specific game types needed for Defensor game
//
//
//  #include "DefensorGameTypes.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/ComponentTypes.h"
#include "Components/GameSystem.h"
#include "Components/InputComponent.h"
#include "Components/LocationComponent.h"
#include "Components/MenuComponent.h"
#include "Components/NameComponent.h"
#include "Components/PayloadStager.h"
#include "Components/ScriptComponent.h"
#include "Components/SystemsCoordinator.h"
#include "Components/UnitComponent.h"
#include "Components/VelocityComponent.h"
#include "GameSystem/Messaging.h"
#include "Items/StageComponent.h"
#include "Renders/RenderComponent.h"
#include "Renders/SceneComponent.h"


namespace defensor::game
{
    using namespace yaget;

    // setup basic mem buffer with write and (maybe) read pointers
    struct MessagingBuffer
    {
        MessagingBuffer(size_t bufferSize = 0)
            : mBuffer(io::CreateBuffer(bufferSize))
        {}

        void AssureWriteSize(size_t additionalSize)
        {
            const auto currentCapacity = io::BufferSize(mBuffer);
            if (mWriteOffset + additionalSize > currentCapacity)
            {
                // standard doubling of requitred memory allocation
                const size_t nextBufferSize = (currentCapacity * 2) + additionalSize;
                mBuffer = io::ResizeBuffer(mBuffer, nextBufferSize);
            }
        }

        void WriteDataChunk(const auto& dataChunk)
        {
            YAGET_ASSERT(mWriteOffset + sizeof(dataChunk) <= io::BufferSize(mBuffer), "Messaging buffer does not have enough space to write dataChunk out.");

            std::memcpy(io::BufferPointer(mBuffer), &dataChunk, sizeof(dataChunk));
            mWriteOffset += sizeof(dataChunk);
        }

        io::Buffer mBuffer;
        size_t mWriteOffset = 0;
        // Increament for every entity during frame. Allows us to allocate memory 
        // based on how many entities there were processed during frame
        size_t mNumEntities = 0;
    };

    using Messaging = comp::PayloadStager<MessagingBuffer>;
    using MessagingPayload = Messaging::Payload;

    struct StateCollectorComponent { static constexpr int Capacity = 64; };

    using GlobalEntity = comp::GlobalRowPolicy<comp::MenuComponent*, items::StageComponent*, StateCollectorComponent*>;
    using Entity = comp::RowPolicy<comp::LocationComponent3*, comp::InputComponent*, comp::UnitComponent*, comp::ScriptComponent*, comp::NameComponent*, comp::VelocityComponent*>;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    using GameCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;
}

namespace defensor::render
{
    using namespace yaget;

    using Messaging = game::Messaging;
            
    using GlobalEntity = comp::GlobalRowPolicy<SceneComponent*>;
    using Entity = comp::RowPolicy<RenderComponent*>;

    using GlobalCoordinator = comp::Coordinator<GlobalEntity>;
    using EntityCoordinator = comp::Coordinator<Entity>;

    using RenderCoordinatorSet = comp::CoordinatorSet<GlobalCoordinator, EntityCoordinator>;
}
