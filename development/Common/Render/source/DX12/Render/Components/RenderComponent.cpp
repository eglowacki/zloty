#include "Render/Components/RenderComponent.h"
#include "Render/Platform/Adapter.h"


//-------------------------------------------------------------------------------------------------
yaget::render::RenderComponent::RenderComponent(comp::Id_t id, const math3d::Matrix& matrix, const io::Tag& sceneItemTag)
    : BaseComponent(id)
    , mMatrix(matrix)
    , mMatrixT(matrix.Transpose())  
    , mSceneItemTag(sceneItemTag)
{
}


//-------------------------------------------------------------------------------------------------
yaget::render::RenderComponent::~RenderComponent() = default;


//-------------------------------------------------------------------------------------------------
void yaget::render::RenderComponent::UpdateMatrix(const math3d::Matrix& matrix)
{
    mMatrix = matrix;
    mMatrixT = matrix.Transpose();
}
