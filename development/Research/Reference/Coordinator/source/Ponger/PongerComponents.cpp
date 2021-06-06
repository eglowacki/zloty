#include "Ponger/PongerComponents.h"


size_t ponger::DebugComponent::CalculateStateHash() const
{
    std::hash<DebugComponent> hash_fn;
    return hash_fn(*this);
}

void ponger::DebugComponent::SetVisualTag(const yaget::io::Tag& tag)
{
    mVisualTag = tag;
    mStateHashDirty = true;
}

void ponger::DebugComponent::SetColor(const colors::Color& color)
{
    mColor = color;
    mStateHashDirty = true;
}
