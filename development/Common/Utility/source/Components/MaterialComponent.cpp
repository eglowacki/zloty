#include "Components/MaterialComponent.h"


yaget::comp::MaterialComponent::MaterialComponent(Id_t id, const db_material::Section::Types& section)
    : PersistentBaseComponent(id, std::tie(section))
{
}
