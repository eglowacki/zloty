/////////////////////////////////////////////////////////////////////////
// RenderCommandTypes.h
//
//  Copyright 03/24/2026 Edgar Glowacki.
//
// NOTES:
//      This provides common data for render commands
//
// #include "Render/Commands/RenderCommandTypes.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "Render/RenderCore.h"

namespace yaget::render::commands
{
    // We prime the fence with the queue type shifted by 56. 
    // I've seen this trick in a number of samples and have found it pretty handy for lazy queue lookups.
    // The idea is that all we need is the fence value itself to know which queue type it came from.
    static constexpr uint64_t QueueTypeOffset = 56;


    //-------------------------------------------------------------------------------------------------
    enum class Type : uint32_t
    {
        Direct = 0,     // D3D12_COMMAND_LIST_TYPE_DIRECT
        Compute = 2,    // D3D12_COMMAND_LIST_TYPE_COMPUTE
        Copy = 3,       // D3D12_COMMAND_LIST_TYPE_COPY
        Max             // used to create an array of last fence values in Device
    };


    //-------------------------------------------------------------------------------------------------
    struct FenceValues
    {
        void Initialize(Type queueType)
        {
            mLastCompletedFenceValue = static_cast<uint64_t>(queueType) << QueueTypeOffset;
            mNextFenceValue = mLastCompletedFenceValue + 1;
        }

        uint64_t PollCurrentFenceValue(UINT64 completedFenceValue)
        {
            mLastCompletedFenceValue = std::max(mLastCompletedFenceValue, completedFenceValue);
            return mLastCompletedFenceValue;
        }

        uint64_t mLastCompletedFenceValue{};
        uint64_t mNextFenceValue{};
    };


    //-------------------------------------------------------------------------------------------------
    struct QueueFenceValues
    {
        FenceValues mFenceValues[static_cast<uint32_t>(Type::Max)] = {};
    };


    //-------------------------------------------------------------------------------------------------
    // Keep track of which states get set each pass, so we can minimize state changes
    struct RenderPassState
    {
        enum class HashType
        {
            Texture = 0,    // this and all Texture... enum values below need to start at 0.
            Texture1,       // It is used as index value into mTexturesHash array
            Texture2,
            Texture3,
            Texture4,
            Texture5,
            Texture6,
            Texture7,
            Texture8,
            Texture9,
            Texture10,
            Texture11,
            Texture12,
            Texture13,
            Texture14,
            Texture15,
            RootSignature,
            PipelineState,
            Topology,
            VertexBuffer,
            IndexBuffer
        };

        size_t mTexturesHash[16]{};
        size_t mRootSignatureHash{};
        size_t mPipelineStateHash{};
        size_t mTopologyHash{};
        size_t mVertexBufferHash{};
        size_t mIndexBufferHash{};

        // If cached state is different from stateObject, return true and update cached state,
        // otherwise return false.
        bool CheckNewHash(auto stateObject, HashType hashType)
        {
            size_t newHash = std::hash<decltype(stateObject)>{}(stateObject);
            size_t* currentHash = nullptr;

            if (hashType >= HashType::Texture && hashType <= HashType::Texture15)
            {
                currentHash = &mTexturesHash[static_cast<size_t>(hashType)];
            }
            else
            {
                switch (hashType)
                {
                    case HashType::RootSignature:
                        currentHash = &mRootSignatureHash;
                        break;
                    case HashType::PipelineState:
                        currentHash = &mPipelineStateHash;
                        break;
                    case HashType::Topology:
                        currentHash = &mTopologyHash;
                        break;
                    case HashType::VertexBuffer:
                        currentHash = &mVertexBufferHash;
                        break;
                    case HashType::IndexBuffer:
                        currentHash = &mIndexBufferHash;
                        break;
                    default: 
                        YAGET_ASSERT(false, "HashType: %d not handled", static_cast<int>(hashType));
                }
            }

            if (currentHash)
            {
                if (*currentHash != newHash)
                {
                    *currentHash = newHash;
                    return true; // State has changed
                }

                return false; // State is the same
            }

            return true; // If we couldn't find a slot for the texture hash, treat it as a change
        }
    };


}