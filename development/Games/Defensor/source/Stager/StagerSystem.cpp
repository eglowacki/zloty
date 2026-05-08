#include "StagerSystem.h"


//-------------------------------------------------------------------------------------------------
void defensor::game::DefensorStagerSystem::UpdateStageComponent(const items::db_stage::Name::Types& stageName, items::db_stage::Blend::Types stageBlend)
{
    CoordinatorSet& cs = GetCS();
    constexpr auto stageId = comp::GLOBAL_ID_MARKER;
    if (auto stageComponent = cs.LoadComponent<items::StageComponent>(stageId))
    {
        stageComponent->SetValue<items::db_stage::Name>(stageName);
        stageComponent->SetValue<items::db_stage::Blend>(stageBlend);
    }
}
