#include "Plugin/ModuleHelper.h"
#include "Config/ConfigHelper.h"
#include "StringHelper.h"
#include "Logger/Log.h"
#include <vector>


namespace
{
    //! List of components which can be registered with ObjectManager
    typedef std::map<std::string, eg::module::RegisterComponentType_t> RegisterComponentTypes_t;

    RegisterComponentTypes_t& getMap()
    {
        static RegisterComponentTypes_t RegisterComponentTypes;
        return RegisterComponentTypes;
    }

} // namespace


namespace eg {
namespace module {


 bool RegisterComponent(const std::string& name, RegisterComponentType_t registerFunction)
{
    getMap().insert(std::make_pair(name, registerFunction));
    return true;
}


void ActivateComponents(const std::string& /*name*/)
{
    logs_not_impl("ActivateComponents");
    /*
    std::string key = "Plugins/" + name + "/Components";
    std::vector<std::string> defualtComponents;
    defualtComponents.push_back("*");
    std::vector<std::string> components = config::ReadStringArray(key.c_str(), defualtComponents);
    for (std::vector<std::string>::const_iterator it = components.begin(); it != components.end(); ++it)
    {
        // \todo since any one of the entries might be wild card, we need to always compare it
        for (RegisterComponentTypes_t::const_iterator itreg = getMap().begin(); itreg != getMap().end(); ++itreg)
        {
            if (WildCompareI(*it, (*itreg).first))
            {
                (*itreg).second();
                wxLogTrace(tr_util, "Registered %s Component with ObjectManager.", (*itreg).first.c_str());
            }
        }
    }
    */
}


} // namespace module
} // namespace eg
