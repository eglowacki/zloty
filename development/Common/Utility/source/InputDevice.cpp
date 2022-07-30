#include "Input/InputDevice.h"
#include "Logger/YLog.h"
#include "Debugging/Assert.h"
#include "fmt/format.h"
#include "StringHelpers.h"
#include "App/AppUtilities.h"
#include "VTS/VirtualTransportSystem.h"
#include "VTS/ResolvedAssets.h"
#include "Metrics/Concurrency.h"

#include <filesystem>
#include <fstream>
#include <algorithm>

namespace
{
    using namespace yaget;

    struct ci_less
    {
        bool operator()(const std::string& s1, const std::string& s2) const
        {
            return conv::ToLower(s1) < conv::ToLower(s2);
        }
    };

    std::map<std::string, uint32_t, ci_less> FlagsMap = {
        { "ButtonUp", input::kButtonUp },
        { "ButtonDown", input::kButtonDown },
        { "ButtonShift", input::kButtonShift },
        { "ButtonCtrl", input::kButtonCtrl },
        { "ButtonAlt", input::kButtonAlt },
        { "ButtonCaps", input::kButtonCaps },
        { "MouseMove", input::kMouseMove },
        { "MouseWheel", input::kMouseWheel },
        { "ButtonNumLock", input::kButtonNumLock },
        { "InputSeqOred", input::kInputSeqOred },
    };

    //--------------------------------------------------------------------------------------------------
    template <typename T>
    void SetSettingsValue(T& valuToSet, const nlohmann::json& node, const char* name)
    {
        auto it = node.find(name);
        if (it != node.end())
        {
            valuToSet = it->get<T>();
        }
    }


    void ParseActionFlags(uint32_t& value, const nlohmann::json& node, const char* name)
    {
        std::string flags;
        SetSettingsValue(flags, node, name);

        std::vector<std::string> tokens = conv::Split(flags, "|");

        for (auto&& token : tokens)
        {
            std::map<std::string, uint32_t>::const_iterator it = FlagsMap.find(token);
            if (it != FlagsMap.end())
            {
                value |= (*it).second;
            }
            else
            {
                YLOG_ERROR("INPT", "There is no mapping for flags: '%s'.", flags.c_str());
            }
        }
    }

    std::map<std::string, int, ci_less> KeysMap = {
        { "Back", 8 },
        { "Tab", 9 },
        { "Return", 13 },
        { "ESC", 27 },
        { "ArrowUp", 38 },
        { "ArrowDown", 40 },
        { "ArrowRight", 39 },
        { "ArrowLeft", 37 },
        { "PageUp", 33 },
        { "PageDown", 34 },
        { "End", 35 },
        { "Home", 36 },
        //{ "Insert", 45 },
        { "Delete", 46 },
        { "MouseLeft", 1 },
        { "MouseRight", 2 },
        { "MouseMiddle", 3 },
        { "Mouse4", 4 },
        { "Mouse5", 5 },
        { "MouseWheel", 6 },
        { "F1", 112 },
        { "F2", 113 },
        { "F3", 114 },
        { "F4", 115 },
        { "F5", 116 },
        { "F6", 117 },
        { "F7", 118 },
        { "F8", 119 },
        { "F9", 120 },
        { "F10", 121 },
        { "F11", 122 },
        { "F12", 123 },
        { "CapsLock", 20 },
        { "Shift", 160 },
    };

    void ParseActionValue(int& value, const nlohmann::json& node, const char* name)
    {
        std::string keys;

        auto itn = node.find(name);
        if (itn->is_string())
        {
            keys = itn->get<std::string>();
        }
        else if (itn->is_number_integer())
        {
            value = itn->get<int>();
            return;
        }

        std::map<std::string, int>::const_iterator it = KeysMap.find(keys);
        if (it == KeysMap.end() && keys.size() == 1 && keys[0] >= 32 && keys[0] <= 127)
        {
            // no record for this key
            value = keys[0];
        }
        else if (it != KeysMap.end())
        {
            value = (*it).second;
        }
        else
        {
            YLOG_ERROR("INPT", "There is no mapping for key: '%s'.", keys.c_str());
        }
    }

    bool IsActionMouse(int keys)
    {
        for (auto&& token : KeysMap)
        {
            if (keys == token.second)
            {
                return token.first.compare(0, strlen("Mouse"), "Mouse") == 0;
            }
        }

        return false;
    }

    void DumpActionConstansts()
    {
        std::string settingsPath = util::ExpendEnv("$(LogFolder)/ActionConstants.txt", nullptr);

        std::ofstream file(settingsPath.c_str());

        file << "; These values are used in KeyBindings file to author actions.\n";
        file << "; User friendly names for keys. Use ASCII values as int or char for rest of values, (\"A\" or 65).\n";
        file << "[KeysMap]\n";
        for (auto&& it: KeysMap)
        {
            file << it.first << '\n';
        }

        file << std::endl << "; User friendly names for key flags. Those can be or'ed together with |, (ButtonDown|ButtonShift).\n";
        file << "[FlagsMap]\n";
        for (auto&& it : FlagsMap)
        {
            file << it.first << '\n';
        }
        file << "[Example]\n";
        file << "\"Quit App\":\n";
        file << "{\n";
        file << "    \"Action\": \"Quit App\",\n";
        file << "    \"ContextName\" : \"\",\n";
        file << "    \"DisplayName\" : \"ESC\",\n";
        file << "    \"Flags\" : \"ButtonDown\",\n";
        file << "    \"Value\" : \"ESC\"\n";
        file << "}\n";
    }

} // namespace


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::Record::ToString() const
{
    std::string flagsText;
    const int numberOfBits = sizeof(uint32_t) * 8;

    for (int i = 0; i < numberOfBits; ++i)
    {
        if (uint32_t value = (mFlags & (0x80000000 >> i)))
        {
            for (auto&& it : FlagsMap)
            {
                if (it.second == value)
                {
                    flagsText += flagsText.empty() ? it.first : ("|" + it.first);
                    break;
                }
            }
        }
    }

    // we want to convert all numbers into readable strings
    return fmt::format("Flags: {}, Time: {}", flagsText, mTimeStamp);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::Mouse::ToString() const
{
    std::string buttonsText;
    if (mButtons[input::kMouseLeft])
    {
        buttonsText = "MouseLeft";
    }
    if (mButtons[input::kMouseRight])
    {
        buttonsText += (!buttonsText.empty() ? "|" : "") + std::string("MouseRight");
    }
    if (mButtons[input::kMouseMiddle])
    {
        buttonsText += (!buttonsText.empty() ? "|" : "") + std::string("MouseMiddle");
    }
    if (mButtons[input::kMouse4])
    {
        buttonsText += (!buttonsText.empty() ? "|" : "") + std::string("Mouse4");
    }
    if (mButtons[input::kMouse5])
    {
        buttonsText += (!buttonsText.empty() ? "|" : "") + std::string("Mouse5");
    }
    return fmt::format("Buttons: {}, Mouse: x:{}, y:{}, wheel: {}, {}", buttonsText, mPos.x, mPos.y, mZDelta, Record::ToString());
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::Key::ToString() const
{
    std::string keyText(1, mValue);
    for (auto&& it : KeysMap)
    {
        if (it.second == mValue)
        {
            keyText = it.first;
            break;
        }
    }

    return fmt::format("Key: {}, {}", keyText, Record::ToString());
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::Mouse::Process(const std::string& actionName, ActionCallback_t actionCallback) const
{
    actionCallback(actionName, mTimeStamp, mPos.x, mPos.y, mFlags);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::Key::Process(const std::string& actionName, ActionCallback_t actionCallback) const
{
    actionCallback(actionName, mTimeStamp, 0, 0, mFlags);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
input::InputDevice::InputDevice(io::VirtualTransportSystem& vts)
{
    LoadConfigFiles(vts);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
input::InputDevice::~InputDevice()
{ }


//------------------------------------------------------------------------------------------------------------------------------------------------------
int input::InputDevice::MapKey(int value) const
{
    // this will translate OS generated key code
    // into our own Record types
    std::map<int, int>::const_iterator it = mKeyMap.find(value);
    // we do allow specifying -1 as to use the original OS value
    if (it != mKeyMap.end() && (*it).second != -1)
    {
        return (*it).second;
    }

    return value;
}


//// or even nicer with a raw string literal
//auto j2 = R"(
//  {
//    "happy": true,
//    "pi": 3.141
//  }
//)"_json;


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::LoadConfigFiles(io::VirtualTransportSystem& vts)
{
    DumpActionConstansts();

    mKeyMap.insert(std::make_pair(192, 96));
    //mKeyMap.insert(std::make_pair(189, 45));
    mKeyMap.insert(std::make_pair(187, 61));
    mKeyMap.insert(std::make_pair(219, 91));
    mKeyMap.insert(std::make_pair(220, 92));
    mKeyMap.insert(std::make_pair(221, 93));
    mKeyMap.insert(std::make_pair(186, 59));
    mKeyMap.insert(std::make_pair(222, 39));
    mKeyMap.insert(std::make_pair(188, 44));
    mKeyMap.insert(std::make_pair(190, 46));
    mKeyMap.insert(std::make_pair(191, 47));

    using Section = io::VirtualTransportSystem::Section;

    const Section keySection("Settings@KeyBindings");

    io::BLobLoader<io::JsonAsset> bindingsLoader(vts, keySection);
    if (bindingsLoader.Assets().empty())
    {
        YLOG_ERROR("INPT", "Did not load key bindings from: vts: '%s'.", keySection.ToString().c_str());
        return;
    }

    const auto& checkedAsset = bindingsLoader.Assets();
    nlohmann::json combinedBindings;

    for (const auto& config : checkedAsset)
    {
        YLOG_INFO("INPT", "Current file: '%s' being processed. combineBindings value: '%s'.", config->mTag.ResolveVTS().c_str(), combinedBindings.dump().c_str());
        combinedBindings.merge_patch(config->root);
    }

    YLOG_INFO("INPT", "CombineBindings value: '%s'.", combinedBindings.dump().c_str());

    const nlohmann::json& root = combinedBindings;

    try
    {
        std::unique_lock<std::mutex> locker(mActionMapMutex);

        mActionMap.clear();
        for (const auto& rootElement : root)
        {
            uint32_t flags = 0;
            int keys = 0;
            ActionMap actionMap;

            if (rootElement.is_object())
            {
                SetSettingsValue(actionMap.mName, rootElement, "Action");
                SetSettingsValue(actionMap.mContextName, rootElement, "ContextName");
                SetSettingsValue(actionMap.mDisplayText, rootElement, "DisplayName");
                ParseActionFlags(flags, rootElement, "Flags");
                ParseActionValue(keys, rootElement, "Value");

                if (IsActionMouse(keys))
                {
                    Mouse::Buttons buttons;
                    buttons[keys] = true;
                    actionMap.mRecord = std::make_shared<Mouse>(0, flags, buttons, 0);
                }
                else
                {
                    actionMap.mRecord = std::make_shared<Key>(0, flags, static_cast<const unsigned char>(keys));
                }

                mActionMap.insert(std::make_pair(actionMap.mName, actionMap));
                YLOG_INFO("INPT", "Register '%s' action: %s", actionMap.mName.c_str(), actionMap.mRecord->ToString().c_str());
            }
        }
    }
    catch(const std::exception& e)
    {
        YLOG_ERROR("INPT", "Did not load key bindings from: '%s'. Error: %s.", keySection.ToString().c_str(), e.what());
    }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::KeyRecord(uint32_t flags, int keyValue, yaget::time::Microsecond_t timeStamp /*= platform::GetRealTime(time::kMicroSecondUnit)*/)
{
    metrics::UniqueLock locker(mPendingInputsMutex, "KeyRecord", YAGET_METRICS_CHANNEL_FILE_LINE);

    auto record = std::make_shared<Key>(timeStamp, flags, static_cast<unsigned char>(keyValue));
    YLOG_DEBUG("INPT", "Generated Key (%d) Record: '%s'.' at time (ms): '%d'.", keyValue, record->ToString().c_str(), time::FromTo<uint32_t>(timeStamp, time::kMicrosecondUnit, time::kMilisecondUnit));
    mPendingInputs.push(std::move(record));
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::MouseRecord(uint32_t flags, const InputDevice::Mouse::Buttons& buttons, int zDelta, const InputDevice::Mouse::Location& pos, yaget::time::Microsecond_t timeStamp /*= platform::GetRealTime(time::kMicroSecondUnit)*/)
{
    metrics::UniqueLock locker(mPendingInputsMutex, "MouseRecord", YAGET_METRICS_CHANNEL_FILE_LINE);

    auto record = std::make_shared<Mouse>(timeStamp, flags, buttons, zDelta, pos);
    YLOG_DEBUG("INPT", "Generated Mouse Record: '%s' at time (ms): '%d'.", record->ToString().c_str(), time::FromTo<uint32_t>(timeStamp, time::kMicrosecondUnit, time::kMilisecondUnit));
    mPendingInputs.push(std::move(record));
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::ProcessRecord(const Record& record)
{
    metrics::UniqueLock locker(mActionMapMutex, "ProcessRecord", YAGET_METRICS_CHANNEL_FILE_LINE);

    const std::string currentContextName = mContextStack.empty() ? "" : mContextStack.top();

    for (auto&& action : mActionMap)
    {
        ActionMap& actionMap = action.second;
        if (actionMap.Is(&record, currentContextName))
        {
            for (auto&& callback : actionMap.mCallbacks)
            {
                record.Process(action.first, callback);
            }
        }
    }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t input::InputDevice::Tick(const time::GameClock& gameClock, const metrics::PerformancePolicy& performancePolicy, metrics::Channel& /*channel*/)
{
    metrics::Channel channel("Input.Tick", YAGET_METRICS_CHANNEL_FILE_LINE);

    time::Microsecond_t begginingTime = platform::GetRealTime(time::kMicrosecondUnit);
    time::Microsecond_t maxBudgetTime = performancePolicy.mBudget == std::numeric_limits<time::Microsecond_t>::max() ? performancePolicy.mBudget : begginingTime + performancePolicy.mBudget;
    InputRecords_t inputsToProcess;

    // only process input up to this time
    time::Microsecond_t maxInputTime = gameClock.GetLogicTime();
    {
        metrics::UniqueLock locker(mPendingInputsMutex, "Input.Pending", YAGET_METRICS_CHANNEL_FILE_LINE);

        if (!mPendingInputs.empty())
        {
            while (!mPendingInputs.empty() && mPendingInputs.top()->mTimeStamp <= maxInputTime)
            {
                inputsToProcess.push(mPendingInputs.top());
                mPendingInputs.pop();
            }
        }
    }

    uint32_t numMessages = 0;
    // we simply do not want to hit this marker for empty list
    if (!inputsToProcess.empty())
    {
        metrics::Channel span("Input.Processing", YAGET_METRICS_CHANNEL_FILE_LINE);

        while (!inputsToProcess.empty())
        {
            ProcessRecord(*inputsToProcess.top());
            inputsToProcess.pop();

            numMessages++;

            if (platform::GetRealTime(time::kMicrosecondUnit) > maxBudgetTime)
            {
                break;
            }
        }

        const bool deferMessages = performancePolicy.mPolicy == metrics::PerformancePolicy::Policy::Default || performancePolicy.mPolicy == metrics::PerformancePolicy::Policy::Defer;
        if (deferMessages)
        {
            // we run out of time to process our messages and based on policy we defer remaining messages to next frame
            metrics::UniqueLock locker(mPendingInputsMutex, "DeferInput", YAGET_METRICS_CHANNEL_FILE_LINE);

            while (!inputsToProcess.empty())
            {
                mPendingInputs.push(inputsToProcess.top());
                inputsToProcess.pop();
            }
        }
    }

    return numMessages;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::TriggerAction(const std::string& actionName, int32_t mouseX, int32_t mouseY, time::Microsecond_t timeStamp /*= platform::GetRealTime(time::kMicroSecondUnit)*/)
{
    //std::unique_lock<std::mutex> locker(mActionMapMutex);
    metrics::UniqueLock locker(mActionMapMutex, fmt::format("TriggerAction-{}", actionName).c_str(), YAGET_METRICS_CHANNEL_FILE_LINE);

    std::string currentContextName = mContextStack.empty() ? "" : mContextStack.top();

    std::map<std::string, ActionMap>::iterator it = mActionMap.find(actionName);
    if (it != mActionMap.end() && it->second.Is(nullptr, currentContextName))
    {
        ActionMap& actionMap = it->second;
        for (auto&& callback : actionMap.mCallbacks)
        {
            callback(actionName, timeStamp, mouseX, mouseY, actionMap.mRecord->mFlags);
        }
    }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::RegisterSimpleActionCallback(const std::string& actionName, input::ActionNonParamCallback_t actionCallback)
{
    //struct CallbackWrapper
    //{
    //    void operator()(const std::string& /*actionName*/, uint64_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/, uint32_t /*flags*/) const
    //    {
    //        mActionCallback();
    //    }

    //    input::ActionNonParamCallback_t mActionCallback;
    //};

    auto wrapper = [actionCallback](auto&&... params)
    {
        actionCallback();
    };

    RegisterActionCallback(actionName, [actionCallback](auto&&... /*params*/) { actionCallback(); });
    //RegisterActionCallback(actionName, CallbackWrapper({ actionCallback }));
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::RegisterActionCallback(const std::string& actionName, input::ActionCallback_t actionCallback)
{
    YAGET_ASSERT(actionCallback, "Registration for action: '%s' is not valid with empty actionCallback", actionName.c_str());

    std::unique_lock<std::mutex> locker(mActionMapMutex);
    std::map<std::string, ActionMap>::iterator it = mActionMap.find(actionName);
    if (it != mActionMap.end())
    {
        (*it).second.mCallbacks.push_back(actionCallback);
    }
    else
    {
        YLOG_ERROR("INPT", "Action '%s' is not registered with input system, ignoring RegisterActionCallback", actionName.c_str());
    }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
bool input::InputDevice::IsAction(const std::string& actionName) const
{
    std::unique_lock<std::mutex> locker(mActionMapMutex);
    return mActionMap.contains(actionName);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::ActionToString(const std::string& actionName) const
{
    std::unique_lock<std::mutex> locker(mActionMapMutex);
    std::map<std::string, ActionMap>::const_iterator it = mActionMap.find(actionName);
    if (it != mActionMap.end())
    {
        return (*it).second.ToString();
    }

    return  "";
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::Record::ResetValidHit()
{
    if (mValidHit)
    {
        mValidHit = false;
        if (mNext)
        {
            mNext->ResetValidHit();
        }
    }
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::ActionMap::ToString() const
{
    return mDisplayText;// mRecord->ToString();
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
bool input::InputDevice::ActionMap::Is(const Record* record, const std::string& currentContextName)
{
    // check against what context this action supports
    if (!mContextName.empty() && !currentContextName.empty() && mContextName != currentContextName)
    {
        return false;
    }
    // if record is nullptr, then we only check against the context name
    if (!record)
    {
        return true;
    }

    // when we process multi input, like for ctrl-K ctrl-K, we only allow so much time
    // before we rest the timer 
    // NOTE: using 1 second, but needs to be config driven
    constexpr time::Microsecond_t kMultiInputTimeout = time::FromTo<time::Microsecond_t>(1.0f, time::kSecondUnit, time::kMicrosecondUnit);
    Record* currentRecord = mRecord.get();

    // we are special case here for multi action and single action input
    if (currentRecord->mNext)
    {
        // we just want any match along this record chain
        if (mAnyRecord)
        {
            do
            {
                if (currentRecord->Is(record))
                {
                    return true;
                }

                currentRecord = currentRecord->mNext.get();
            } while (currentRecord);

            return false;
        }

        if (!currentRecord->mValidHit)
        {
            // this is untouched multi action input, so, let's start the timer for how long it will stay valid
            mLastAccessTime = record->mTimeStamp;
        }

        // this means that we already hit valid at least for the first action, so now, we need to first verify if timer is still valid
        // which means that incoming record time stamp must be before first access plus max time to wait for next key
        if (record->mTimeStamp < mLastAccessTime + kMultiInputTimeout)
        {
            // advance to the next not hit action in the chain
            while (currentRecord && currentRecord->mValidHit)
            {
                currentRecord = currentRecord->mNext.get();
            }
        }
        else
        {
            // timer run-out, reset hits
            mRecord->ResetValidHit();
        }
    }

    YAGET_ASSERT(currentRecord, "This should never happen, since this means that all recored where hit/validated.");
    if (currentRecord->Is(record))
    {
        currentRecord->mValidHit = true;
        if (!currentRecord->mNext)
        {
            mRecord->ResetValidHit();
            return true;
        }
    }

    return false;
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
void input::InputDevice::PushContext(const std::string& newContextName)
{
    std::unique_lock<std::mutex> locker(mActionMapMutex);

    mContextStack.push(newContextName);
}


//------------------------------------------------------------------------------------------------------------------------------------------------------
std::string input::InputDevice::PopContext()
{
    std::unique_lock<std::mutex> locker(mActionMapMutex);

    std::string topOfContext;
    if (!mContextStack.empty())
    {
        topOfContext = mContextStack.top();
        mContextStack.pop();
    }

    return topOfContext;
}

