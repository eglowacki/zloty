///////////////////////////////////////////////////////////////////////
// InputManagerImpl.h
//
//  Copyright 11/20/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "InputManagerImpl.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_INPUT_MANAGER_IMPL_H
#define YAGET_INPUT_MANAGER_IMPL_H

#ifndef YAGET_INPUT_MANAGER_INCLUDE_IMPLEMENTATION
#error "Do not include this file explicitly."
#endif // YAGET_INPUT_MANAGER_INCLUDE_IMPLEMENTATION
#include "fmt/format.h"

namespace yaget
{
    namespace input
    {
        //---------------------------------------------------------------------------------------------------------
        //! Inline implementation
        // input
        inline InputDevice::Record::Record(const time::Microsecond_t timeStamp, const uint32_t flags, int type)
            : mFlags(flags)
            , mTimeStamp(timeStamp)
            , Type(type)
        {
        }

        //---------------------------------------------------------------------------------------------------------
        inline bool InputDevice::Record::Is(const InputDevice::Record* target, bool andCompare) const
        {
            if ((!Type) == target->Type)
            {
                return false;
            }

            //And = false;
            // if type is the same and this flags can be and'ed
            // with Source flags then Record is the same.
            //if (mType == target->mType)
            // the only time we can compare the flags if Target
            // has any flags set.
            if (target->mFlags)
            {
                if (andCompare)
                {
                    return CompareAllBits(mFlags, target->mFlags);
                }
                else
                {
                    return mFlags == target->mFlags;
                }
            }
            else
            {
                // since our Target does not have any flags,
                // we do not check it and just return TRUE.
                return true;
            }
        }

        //---------------------------------------------------------------------------------------------------------
        inline bool InputDevice::Record::CompareAllBits(const uint32_t source, const uint32_t target)
        {
            bool result = true;

            // step trough all the bits by extracting the least significant one after shifting,
            // then compare that bit with Source.
            for (int i = 0; i < 32; i++)
            {
                if (const uint32_t currentBit = target & (1 << i))
                {
                    if (!(source & currentBit))
                    {
                        result = false;
                        break;
                    }
                }
            }

            return result;
        }

        // key
        inline InputDevice::Key::Key(const uint64_t microDeltaTime, const uint32_t flags, const unsigned char keyValue) : InputDevice::Record(microDeltaTime, flags, 2)
            , mValue(keyValue)
        {
        }

        //---------------------------------------------------------------------------------------------------------
        inline bool InputDevice::Key::Is(const Record* target, bool andCompare) const
        {
            if (InputDevice::Record::Is(target, andCompare))
            {
                const Key *key = static_cast<const Key *>(target);
                if (mValue == key->mValue)
                {
                    // special case for key up
                    if (mDown && (target->mFlags & input::kButtonUp) && CompareAllBits(mFlags, input::kButtonDown | input::kButtonUp))
                    {
                        mDown = false;
                        return true;
                    }

                    constexpr uint32_t controlFlag = input::kButtonShift | input::kButtonCtrl;
                    const uint32_t thisFlags = mFlags & controlFlag;
                    const uint32_t targetFlags = target->mFlags & controlFlag;
                    if (thisFlags == targetFlags)
                    {
                        if (!mDown && (target->mFlags & input::kButtonDown) && CompareAllBits(mFlags, input::kButtonDown | input::kButtonUp))
                        {
                            mDown = true;
                        }

                        return true;
                    }
                }
            }

            return false;
        }

        inline InputDevice::Mouse::Mouse(const time::Microsecond_t microDeltaTime, const uint32_t flags, const Buttons& buttons, int zDelta, const Location& pos)
            : InputDevice::Record(microDeltaTime, flags, 3)
            , mPos(pos)
            , mButtons(buttons)
            , mZDelta(zDelta)
        {
        }

        //---------------------------------------------------------------------------------------------------------
        inline InputDevice::Mouse::Mouse(const uint32_t flags, const Buttons& buttons, int zDelta, const Location& pos)
            : InputDevice::Record(0, flags, 3)
            , mPos(pos)
            , mButtons(buttons)
            , mZDelta(zDelta)
        {
        }

        //---------------------------------------------------------------------------------------------------------
        inline bool InputDevice::Mouse::Is(const Record* target, bool andCompare) const
        {
            if (InputDevice::Record::Is(target, andCompare))
            {
                const Mouse *mouse = static_cast<const Mouse *>(target);

                if (mPos == mouse->mPos || mPos == Location(-1, -1))
                {
                    if (mButtons == mouse->mButtons)
                    {
                        return true;
                    }
                }
            }

            return false;
        }

    } // namespace input
} // namespace yaget

#endif // YAGET_INPUT_MANAGER_IMPL_H
