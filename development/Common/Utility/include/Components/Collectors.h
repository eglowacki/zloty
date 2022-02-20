//////////////////////////////////////////////////////////////////////
// Collectors.h
//
//  Copyright 7/5/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers for gathering position and scene data to populate Stager and let rendering thread use that
//      Sample of Templates:
//      template <typename T>
//          stuct S{};
//
//          S<int> s1;
//          S<bool> s2;
//
//          template<typename T>
//          using type = S<T>
//
// Partial template specialization
// template <bool B, typename T, typename U>
// struct conditional
// {
//	// using type = T;
// };
//
// template <typename T, typename U>
// struct conditional<false, T, U>
// {
//  using type = U;
// };
// 
// template <typename T0, typename... Ts>
// void print(T0&& t0, Ts&&...ts)
// {
//  std::cout << std::forward<T0>(t0) << "\n";
//  
//  if constexpr (sizeof...(ts))
//      print(std:::forward<T0>(ts)...);
//  }
//  
// template <size_t I>
// auto& get(person& p)
// {
//  if constexpr (I = 0)
//      return p.get_id();
//  if constexpr (I = 1)
//      return p.get_name();
//  if constexpr (I = 2)
//      return p.get_age();
// }
//
//
// template <auto v>
// struct constant
// {
//    static constexpr auto value = v;
// };
//
// using i = constant<2048>;
// 
// template <typename T>
// const bool is_constant_v = (some_expresion);
// is_constant_V<int>;
// 
//
// std::visit([](auto& t)
// {
//    using T = std::decay_t<decltype(t)>;
//    if constexpr (std::is_same_v<T, std::string>)
//        // ... do stuff
// });
//
// 
// template <typename... Ts>
// struct all_integral : std::conjunction<std::is_integral<Ts>...>
// {};
//
// 
// auto t
// foo<ECS_TYPE(t)>;
// ECS_TYPE expends to 'typename decltype(t)::type'
//
// ----------- Higher Order Functions ----------------
//using event = std::variant<connect, disconnect, heartbeat>;
//
//void process(event&& e)
//{
//    std::visit(
//        overload([](connect) { /* ..do stuff */ },
//                 [](disconnect) { /* ..do stuff */ },
//                 [](heartbeat) { /* ..do stuff */ }),
//          e);
//}
//
//process(event{ connect {} });
//process(event{ disconnect {} });
//process(event{ heartbeat {} });
//
//
// https://stackoverflow.com/questions/47496358/c-lambdas-how-to-capture-variadic-parameter-pack-from-the-upper-scope?rq=1
//
//  #include "Components/Collectors.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once


#include "YagetCore.h"
#include "MathFacade.h"
#include "ComponentTypes.h"
#include "PayloadStager.h"
#include "Streams/Buffers.h"
#include "Time/GameClock.h"
#include "Debugging/Assert.h"
#include <memory>
#include <typeindex>


// provide comp id and type_index pair for hash so it can be used with containers (unordered_map)
template <>
struct std::hash<std::pair<yaget::comp::Id_t, std::type_index>>
{
	size_t operator()(const std::pair<yaget::comp::Id_t, std::type_index>& collectorKey) const noexcept
	{
		constexpr std::hash<yaget::comp::Id_t> hashId_fn;
		constexpr std::hash<std::type_index> hashType_fn;

		size_t hashOne = hashId_fn(collectorKey.first);
		size_t hashTwo = hashType_fn(collectorKey.second);
		if (hashOne > hashTwo)
		{
			std::swap(hashOne, hashTwo);
		}

		constexpr std::hash<size_t> hashKey_fn;
		const size_t result = hashKey_fn(hashOne + hashTwo);

		return result;
	}
};

namespace yaget::comp
{
	// Used as base class for more specific handling of collection of scene data.
	// Main purpose is to handle last id marker, keep track of which components changed
	// and maintain list of component hash states
	// Potential implementation of Process
	//void Process(yaget::comp::Id_t id, const yaget::time::GameClock& gameClock, yaget::metric::Channel& channel, yaget::comp::LocationComponent* location)
	//{
	//    if (IsEndMarker(id, gameClock))
	//    {
	//        return;
	//    }

	//    size_t currentHash = location->GetState();
	//    if (UpdateHash(id, std::type_index(typeid(yaget::comp::LocationComponent)), currentHash))
	//    {
	//        mPayload->mLocations.push_back(yaget::comp::LocationChunk{ id, location->GetPosition(), location->GetOrientation() });
	//    }
	//    else
	//    {
	//        mPayload->mActiveIds.insert(id);
	//    }
	//}
	// 
	template <typename T>
	class CollectorHelper : public Noncopyable<CollectorHelper<T>>
	{
	public:
		using Stager = PayloadStager<T>;

		CollectorHelper() // : mPayloadStager(payloadStager)
		{
		}

		const Stager& PayloadStager() const { return mPayloadStager; }

	protected:
		// Return true (set payload and clear current one)
		// if id is end marker, otherwise false
		bool IsEndMarker(Id_t id, const time::GameClock& gameClock)
		{
			if (id == END_ID_MARKER)
			{
				mPayloadStager.SetPayload(mPayload);
				mPayload = nullptr;
				return true;
			}

			if (mLastTickCounter != gameClock.GetTickCounter())
			{
				mLastTickCounter = gameClock.GetTickCounter();
				// this is a start of the frame, get new buffer to work with
				mPayload = mPayloadStager.CreatePayload();
			}

			YAGET_ASSERT(mPayload, "Payload for stager is not initialized, posibly calling into Process in the same Frame/Tick that end id marker was already called.");
			return false;
		}

		// Return true if hash was updated (entity changed), false otherwise and nothing changed about it
		bool UpdateHash(Id_t id, std::type_index compType, size_t currentHash)
		{
			const StateKey stateKey = std::make_pair(id, compType);
			const auto it = mHashes.find(stateKey);
			const size_t lastHash = it != mHashes.end() ? it->second : std::numeric_limits<size_t>::max();

			if (lastHash != currentHash)
			{
				// we need to update render location, since it got changed last time we rendered
				mHashes.insert_or_assign(stateKey, currentHash);
				return true;
			}

			return false;
		}

		// IsEndMarker(...) call manages content of this data. Derived classes will use this
		// to fill application specific data
		typename Stager::Payload mPayload;

	private:
		Stager mPayloadStager;

		// maps hash value for component to see if we need to get new values
		using StateKey = std::pair<Id_t, std::type_index>;
		using Hashes = std::unordered_map<StateKey, size_t>;

		Hashes mHashes;
		uint64_t mLastTickCounter = time::INVALID_TICK_COUNTER;
	};

	// Used for specifying which data collector needs to get from game thread
	struct LocationChunk
	{
		Id_t mId = INVALID_ID;
		math3d::Vector3 mPosition;
		math3d::Quaternion mOrientation;
		math3d::Vector3 mScale;
		io::Tag mTag;
		math3d::Color mColor;
	};

	struct SceneChunk
	{
		LocationChunk mCamera;
		std::vector<LocationChunk> mLocations{};
		ItemIds mActiveIds{};
		ItemIds mDebugIds{};
	};
} // namespace yaget::comp
