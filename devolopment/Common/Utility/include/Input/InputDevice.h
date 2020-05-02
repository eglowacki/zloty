///////////////////////////////////////////////////////////////////////
// InputDevice.h
//
//  Copyright 11/13/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Record system which abstracts platform specific code.
//      It does relay on input being fed to it by user.
//
//
//  #include "Record/InputDevice.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef YAGET_INPUT_INPUT_SYSTEM_H
#define YAGET_INPUT_INPUT_SYSTEM_H
#pragma once

#include "YagetCore.h"
#include "Metrics/Performance.h"
#include "Platform/Support.h"
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <stack>
#include <queue>
#include <mutex>
#include <bitset>
//#include <BitArray.h>

namespace yaget
{
    namespace metrics { class Channel; }
    namespace io { class VirtualTransportSystem; }

    namespace input
    {
        constexpr uint32_t kButtonUp        = 0x0001;
        constexpr uint32_t kButtonDown      = 0x0002;
        constexpr uint32_t kButtonShift     = 0x0004;
        constexpr uint32_t kButtonCtrl      = 0x0008;
        constexpr uint32_t kButtonAlt       = 0x0010;
        constexpr uint32_t kButtonCaps      = 0x0020;
        constexpr uint32_t kMouseMove       = 0x0040;
        constexpr uint32_t kMouseWheel      = 0x0080;
        constexpr uint32_t kButtonNumLock   = 0x0100;
        constexpr uint32_t kInputSeqOred    = 0x0200; ///< user can specify several input types

        /*!
        Some defines for keys.
        */
        constexpr uint32_t kBack             = 8;
        constexpr uint32_t kTab              = 9;
        constexpr uint32_t kReturn           = 13;
        constexpr uint32_t kEsc              = 27;
        constexpr uint32_t kArrowUp          = 255;
        constexpr uint32_t kArrowDown        = 254;
        constexpr uint32_t kArrowRight       = 253;
        constexpr uint32_t kArrowLeft        = 252;
        constexpr uint32_t kPageUp           = 33;
        constexpr uint32_t kPageDown         = 34;
        constexpr uint32_t kEnd              = 249;
        constexpr uint32_t kHome             = 248;
        constexpr uint32_t kInsert           = 247;
        constexpr uint32_t kDelete           = 246;

        constexpr uint32_t kMouseLeft        = 1;
        constexpr uint32_t kMouseRight       = 2;
        constexpr uint32_t kMouseMiddle      = 3;
        constexpr uint32_t kMouse4           = 4;
        constexpr uint32_t kMouse5           = 5;
        constexpr uint32_t kMouse6           = 6;

        constexpr uint32_t kMaxMouseButtons  = 32;

        using ActionCallback_t = std::function<void(const std::string& /*actionName*/, uint64_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/, uint32_t /*flags*/)>;
        using ActionNonParamCallback_t = std::function<void(void)>;

        class InputDevice : public Noncopyable<InputDevice>
        {
        public:
            InputDevice(const InputDevice&) = delete;
            InputDevice& operator=(const InputDevice&) = delete;

            InputDevice(io::VirtualTransportSystem& vts);
            ~InputDevice();

            class Record
            {
            public:
                Record(const time::Microsecond_t timeStamp, const uint32_t flags, int type);
                virtual ~Record() = default;

                /*!
                \brief Returns TRUE if Target type matches and flags are equals.

                Will return TRUE if mType and mFlags are the same.
                Parameter And is used to see how we check this.mFlags against Source.mFlags.
                If TRUE then (Source.mFlags & this.mFlags) is used, otherwise
                (Source.mFlags == this.mFlags) is used.
                */
                virtual bool Is(const Record* target, bool andCompare = true) const;

                void ResetValidHit();

                //! This will return TRUE if all the bits from target are in source.
                // 0b0001 & 0b0011 = false
                // 0b0011 & 0b0001 = true
                static bool CompareAllBits(const uint32_t source, const uint32_t target);

                virtual std::string ToString() const;

                virtual void Process(const std::string& actionName, ActionCallback_t actionCallback) const = 0;

                const uint32_t mFlags = 0;                      ///< Different flags, different meaning based on what kind of input.
                const time::Microsecond_t mTimeStamp = 0;     ///< Time of that input.
                std::shared_ptr<Record> mNext;                  ///< We can chain Inputs for replays and 'Action' matching.
                bool mValidHit = false;
                const int Type = 1;
            };

            class Key : public Record
            {
            public:
                Key(const uint64_t microDeltaTime, const uint32_t flags, const unsigned char keyValue);

                // From Record
                bool Is(const Record *target, bool andCompare = true) const override;
                std::string ToString() const override;
                void Process(const std::string& actionName, ActionCallback_t actionCallback) const override;

                const unsigned char mValue = 0;     ///< ASCII number if this key.
                mutable bool mDown = false;         ///< handles the case, when key down and up actions are the same one
                                                    ///< and anytime key is up (regardless of other key pressed), we want to generate release
            };

            //! mouse input class
            class Mouse : public Record
            {
            public:
                using Buttons = std::bitset<kMaxMouseButtons>;

                struct Location
                {
                    Location() = default;
                    Location(int x, int y) : x(x), y(y) {}
                    int x = 0; int y = 0;

                    bool operator==(const Location& rhs) const { return x == rhs.x && y == rhs.y; }
                    bool operator!=(const Location& rhs) const { return !(*this == rhs); }
                };
                const Location mPos;        ///<Absolute position of the mouse.
                const Buttons mButtons;    ///<Bit pattern of all the buttons which the flags are true.
                const int mZDelta = 0;

                Mouse(const time::Microsecond_t microDeltaTime, const uint32_t flags, const Buttons& buttons, int zDelta, const Location& pos = Location(-1, -1));
                Mouse(const uint32_t flags, const Buttons& buttons, int zDelta, const Location& pos = Location(-1, -1));

                // from Record
                bool Is(const Record *target, bool andCompare = true) const override;
                std::string ToString() const override;
                void Process(const std::string& actionName, ActionCallback_t actionCallback) const override;
            };


            // Used by platform specific code to feed translated user input
            void KeyRecord(uint32_t flags, int keyValue, time::Microsecond_t timeStamp = platform::GetRealTime(time::kMicrosecondUnit));
            void MouseRecord(uint32_t flags, const Mouse::Buttons& buttons, int zDelta, const Mouse::Location& pos, time::Microsecond_t timeStamp = platform::GetRealTime(time::kMicrosecondUnit));

            int MapKey(int value) const;
            void RegisterActionCallback(const std::string& actionName, ActionCallback_t actionCallback);
            // SImple input callback that does not take any parameters. Ex: like quit, programmer might not care about
            // actionName, timeStamp, mouseX, mouseY or flags
            void RegisterSimpleActionCallback(const std::string& actionName, ActionNonParamCallback_t actionCallback);
            // Return true if actionName is registered with input device
            bool IsAction(const std::string& actionName) const;

            // Return display string for key/input associated with this action
            std::string ActionToString(const std::string& actionName) const;
            void TriggerAction(const std::string& actionName, int32_t mouseX, int32_t mouseY, time::Microsecond_t timeStamp = platform::GetRealTime(time::kMicrosecondUnit));

            // Return number of messages processed in this tick
            uint32_t Tick(const time::GameClock& gameClock, const metrics::PerformancePolicy& performancePolicy, metrics::Channel& channel);

            //! Pop top context off and return it
            std::string PopContext();
            //! Push new context onto top of a stack and make it current
            void PushContext(const std::string& newContextName);

            struct ContextScoper
            {
                ContextScoper(InputDevice& inputDevice, const std::string& newContextName) : inputDevice(inputDevice)
                {
                    inputDevice.PushContext(newContextName);
                }

                ~ContextScoper() 
                {
                    (void)inputDevice.PopContext();
                }

                InputDevice& inputDevice;
            };

        private:
            void LoadConfigFiles(io::VirtualTransportSystem& vts);
            void ProcessRecord(const Record& record);

            mutable std::mutex mActionMapMutex;     // if we never want to re-load config file during runtime, then this mutex could go away
            mutable std::mutex mPendingInputsMutex;

            struct CompareInput
            {
                bool operator()(const std::shared_ptr<Record>& p1, const std::shared_ptr<Record>& p2) const
                {
                    // return "true" if "p1" is ordered before "p2", for example:
                    return p1->mTimeStamp > p2->mTimeStamp;
                }
            };
            using InputRecords_t = std::priority_queue<std::shared_ptr<Record>, std::vector<std::shared_ptr<Record>>, CompareInput>;
            InputRecords_t mPendingInputs;
            std::map<int, int> mKeyMap;

            struct ActionMap
            {
                bool Is(const Record* record, const std::string& currentContextName);
                std::string ToString() const;

                bool mAnyRecord = false;
                std::string mName;
                std::string mContextName;
                std::string mDisplayText;
                time::Microsecond_t mLastAccessTime = 0;
                std::shared_ptr<Record> mRecord = nullptr;
                std::vector<ActionCallback_t> mCallbacks;

            };
            std::map<std::string, ActionMap> mActionMap;
            std::stack<std::string> mContextStack;
        };


    } // namespace input

} // namespace yaget


#define YAGET_INPUT_MANAGER_INCLUDE_IMPLEMENTATION
#include "InputManagerImpl.h"
#undef YAGET_INPUT_MANAGER_INCLUDE_IMPLEMENTATION

#endif // YAGET_INPUT_INPUT_SYSTEM_H
