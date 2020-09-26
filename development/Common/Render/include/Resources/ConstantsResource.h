//////////////////////////////////////////////////////////////////////
// ConstantsResource.h
//
//  Copyright 8/3/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Management of constant buffers for shaders
//
//
//  #include "Resources/ConstantsResource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "ResourceView.h"
#include "VTS/RenderResolvedAssets.h"
#include <wrl/client.h>


namespace yaget
{
    namespace render
    {
        class Device;
        
        //--------------------------------------------------------------------------------------------------
        // Wrapper around constants buffer
        class ConstantsResource : public ResourceView
        {
        public:
            enum class Type { Vertex, Pixel };

            ConstantsResource(Device& device, std::shared_ptr<io::render::ShaderAsset> asset);

            bool Activate() override;
            const char* GetNameType() const override { return "Constants"; }

            template <typename T>
            void Update(const std::string& constantName, const std::string& slotName, const T& source)
            {
                auto it = mBuffers.find(constantName);
                if (it != mBuffers.end())
                {
                    it->second.Update<T>(slotName, source);
                }
            }

        private:
            // each structure represents one cBuffer with member variables
            struct Buffer
            {
                template <typename T>
                void Update(const std::string& variableName, const T& source)
                {
                    if (variableName.empty() || *variableName.begin() == '=')
                    {
                        YAGET_ASSERT(sizeof(T) == mData.size(), "Trying to set Constant Slot of size: '%d' with user data of size: '%d'.", mData.size(), sizeof(T));

                        T* target = reinterpret_cast<T*>(mData.data());
                        *target = source;
                        mDirty = true;
                    }
                    else
                    {
                        auto it = mSlots.find(variableName);
                        if (it != mSlots.end())
                        {
                            T* target = reinterpret_cast<T*>(mData.data() + it->second);
                            *target = source;
                            mDirty = true;
                        }
                    }
                }

                Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants;
                std::vector<uint8_t> mData;
                std::map<std::string, size_t> mSlots;
                bool mDirty = false;
                uint32_t mRegisterNumber = 0;
            };

            std::unordered_map<std::string, Buffer> mBuffers;
            Type mType = Type::Vertex;
        };

    } // namespace render
} // namespace yaget
