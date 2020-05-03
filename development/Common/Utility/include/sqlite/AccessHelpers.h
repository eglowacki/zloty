#pragma once

#include <functional>


// Helper free functions
// Provide get extension helpers that can operate on Actor and ActoExtensions, and have checked variations of those calls
// Assumptions:
//  get extension with actor will handle nulptr correctly, but any calls to get extension with ActorExtension, assumes *(asserts)
//  that pointer is valid and also has valid Owner Actor.
// The *_checked version will assert on nullptr parameter and verify that extension exists on an Actor
//
// Usage:
//  Actor* myActor // this maybe null or not
//  HealthExtension* health = get_extension<HealthExtension>(myActor);
//  if (health)
//  {
//      // do something with it
//  }
//
//  This will fire assert if myActor is nullptr or there is no HealthExtension
//  HealthExtension* health = get_extension_checked<HealthExtension>(myActor);
//  health->DoSomething();
//
//  Second variations
//  SpeedExtension* speed // this maybe null or not
//  HealthExtension* health = get_extension<HealthExtension>(speed);
//  if (health)
//  {
//      // do something with it
//  }
//
//  This will fire assert if myActor is nullptr or there is no HealthExtension
//  HealthExtension* health = get_extension_checked<HealthExtension>(speed);
//  health->DoSomething();
//
namespace LYGame
{
    ///// Assert if the GameExtension is missing
    //template<typename T>
    //T* get_game_extension_checked()
    //{
    //    T* extension = g_Game->GetExtension<T>();
    //    CRY_ASSERT_TRACE(extension, ("Can not get game extension of type: '%s'", T::GetStaticClass()->GetClassName()));
    //    return extension;
    //}

    ///// Return T type extension if it exists on this actor
    ///// Returns valid extension, or nullptr if actor is NULL or extension doe snot exist
    //template<typename T>
    //T* get_extension(const Actor *actor)
    //{
    //    if (actor)
    //    {
    //        return get_extension<T>(*actor);
    //    }

    //    return nullptr;
    //}

    ///// Same function as above, but takes actor as reference. This assumes that we have a valid actor.
    ///// If extension doe snot exist, it will return nullptr
    //template<typename T>
    //T* get_extension(const Actor& actor)
    //{
    //    return actor.GetExtension<T>();
    //}

    ///// This will fire assert if actor parameters is nullptr or there is no extension on an actor. 
    ///// It's usage is for times when we expect extension to be on actor.
    //template<typename T>
    //T* get_extension_checked(const Actor *actor)
    //{
    //    CRY_ASSERT_TRACE(actor, ("Can not get extension of type: '%s' for nullptr Actor.", T::GetStaticClass()->GetClassName()));
    //    return get_extension_checked<T>(*actor);
    //}

    ///// This will fire assert if actor parameters is nullptr or there is no extension on an actor. 
    ///// It's usage is for times when we expect extension to be on actor.
    //template<typename T>
    //T* get_extension_checked(const _weak_ptr<Actor>& actor)
    //{
    //    CRY_ASSERT_TRACE(actor.is_valid(), ("Can not get extension of type: '%s' for nullptr Actor.", T::GetStaticClass()->GetClassName()));
    //    return get_extension_checked<T>(*actor.get());
    //}

    ///// Same as above, but it takes actor ref.
    //template<typename T>
    //T* get_extension_checked(const Actor& actor)
    //{
    //    T* extension = get_extension<T>(actor);
    //    CRY_ASSERT_TRACE(extension, ("There is no extension of type: '%s' for Actor: '%s'.", T::GetStaticClass()->GetClassName(), get_display_name(actor).c_str()));
    //    return extension;
    //}

    ///// Helper function to get particular extension from actor owner of this one. It is just synthetic sugar to simplify usage cases
    //template<typename T>
    //T* get_extension(const ActorExtension *extension)
    //{
    //    CRY_ASSERT_TRACE(extension, ("get_extension called requesting: '%s' extension, but ActorExtension param is nullptr.", T::GetStaticClass()->GetClassName()));
    //    CRY_ASSERT_TRACE(extension->GetActor(), ("get_extension called for type: '%s' without an owner.", T::GetStaticClass()->GetClassName()));
    //    return get_extension<T>(*extension->GetActor());
    //}

    ///// Same function behavior as the one above, but it will assert on missing extension. IOW, usage of this function expect the extension to be there
    //template<typename T>
    //T* get_extension_checked(const ActorExtension *extension)
    //{
    //    CRY_ASSERT_TRACE(extension, ("get_extension_checked called requesting: '%s' extension, but ActorExtension param is nullptr.", T::GetStaticClass()->GetClassName()));
    //    CRY_ASSERT_TRACE(extension->GetActor(), ("get_extension_checked called for type: '%s' without an owner.", T::GetStaticClass()->GetClassName()));
    //    return get_extension_checked<T>(*extension->GetActor());
    //}

    ///// Useful in logging in particular
    ////  Actor* myActor // this maybe null or not
    ///// string actorName = get_display_name(myActor);
    //inline string get_display_name(const Actor& actor)
    //{
    //    return actor.GetName();
    //}

    ///// Same as above one, but it takes pointer and allows nullptr for an actor. If actor does not exist, it will return empty string
    //inline string get_display_name(const Actor* actor)
    //{
    //    return actor ? get_display_name(*actor) : "";
    //}

    ///// Same as above, but it works with extension and gets it's owner/actor name. Just a helper function to simplify usage.
    //inline string get_display_name(const ActorExtension* extension)
    //{
    //    CRY_ASSERT_TRACE(extension, ("get_display_name called with ActorExtension as nullptr."));
    //    CRY_ASSERT_TRACE(extension->GetActor(), ("get_display_name called for type: '%s' without an owner.", extension->GetClassName()));
    //    return get_display_name(*extension->GetActor());
    //}

    ///// Same set of functions as above, but for getting the prototype path instead of the name
    //inline string get_prototype_path(const Actor& actor)
    //{
    //    return actor.m_Path;
    //}

    //inline string get_prototype_path(const Actor* actor)
    //{
    //    return actor ? get_prototype_path(*actor) : "";
    //}

    //inline string get_prototype_path(const ActorExtension* extension)
    //{
    //    CRY_ASSERT_TRACE(extension, ("get_prototype_path called with ActorExtension as nullptr."));
    //    CRY_ASSERT_TRACE(extension->GetActor(), ("get_prototype_path called for type: '%s' without an owner.", extension->GetClassName()));
    //    return get_prototype_path(*extension->GetActor());
    //}

    ///// Function to convert Vec3 type into string representation
    //inline string ToString(const Vec3& Value)
    //{
    //    string ressult;
    //    ressult.Format("x = %.2f, y = %.2f, z = %.2f", Value.x, Value.y, Value.z);
    //    return ressult;
    //}

    ///// Function to convert Quat type into string representation
    //inline string ToString(const Quat& Value)
    //{
    //    string ressult;
    //    ressult.Format("x = %.2f, y = %.2f, z = %.2f, w = %.2f", Value.v.x, Value.v.y, Value.v.z, Value.w);
    //    return ressult;
    //}

    //// Convert from string to Vec3
    //bool FromString(const string& str, Vec3& outVec);

    //// Convert from string to Quat
    //bool FromString(const string& str, Quat& outQuat);

    ///// Is the editor running?
    ///// This makes no distinction between whether the user is in the
    ///// game playing by way of Ctrl-G vs just editing.
    //inline bool is_editor()
    //{
    //    return gEnv->IsEditor();
    //}

    ///// Is the editor running and the player has pressed Ctrl-G to play the game?
    //inline bool is_editor_playing()
    //{
    //    // An assert for our current understanding of these flags
    //    CRY_ASSERT(!gEnv->IsEditor() || gEnv->IsEditorGameMode() != gEnv->IsEditing());

    //    return gEnv->IsEditor() && gEnv->IsEditorGameMode();
    //}

    ///// Is the editor running but the player is not playing the game with ctrl-G?
    //inline bool is_editor_editing()
    //{
    //    // An assert for our current understanding of these flags
    //    CRY_ASSERT(!gEnv->IsEditor() || gEnv->IsEditorGameMode() != gEnv->IsEditing());

    //    return gEnv->IsEditor() && !gEnv->IsEditorGameMode();
    //}

    ///// Is this a dedicated server? No client present, editor not running
    //inline bool is_dedicated_server()
    //{
    //    if (is_editor())
    //    {
    //        return false;
    //    }

    //    return !gEnv->IsClient() && gEnv->bServer;
    //}

    ///// Is this a dedicated client? No server present, editor not running
    //inline bool is_dedicated_client()
    //{
    //    if (is_editor())
    //    {
    //        return false;
    //    }

    //    return gEnv->IsClient() && !gEnv->bServer;
    //}

    ///// Is this any type of dedicated, be it server or client?
    //inline bool is_dedicated()
    //{
    //    return is_dedicated_client() || is_dedicated_server();
    //}

    //inline bool is_server()
    //{
    //    return is_editor() || gEnv->bServer;
    //}

    //inline bool is_client()
    //{
    //    return is_editor() || gEnv->IsClient();
    //}

    //enum class NetworkData
    //{
    //    Server,
    //    Client,
    //    Shared,
    //};

} // namespace LYGame