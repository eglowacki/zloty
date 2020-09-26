#include "Resources/ResourceWatchRefresher.h"
#include "IdGameCache.h"
#include "Device.h"
#include "App/Application.h"


uint64_t yaget::render::UpdatePolicyWatched::GetWatchId(Device& device)
{
    return idspace::get_burnable(device.App().IdCache); 
}
