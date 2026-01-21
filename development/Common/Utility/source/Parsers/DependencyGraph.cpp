#include "Parsers/DependencyGraph.h"

#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"


namespace
{
    std::string MakeValidGuid(const std::string& id)
    {
        return "00000000-0000-0000-0000-00000000" + id;
    }
}


namespace yaget
{
    using DiskDependencyData = std::map<std::string, DependencyNode>;

    void to_json(nlohmann::json& j, const DependencyNode& p) 
    {
        j = nlohmann::json{ {"name", p.mName}, {"dependencies", p.mDependencies} };
    }
    void from_json(const nlohmann::json& j, DependencyNode& p) 
    {
        j.at("name").get_to(p.mName);

        j.at("dependencies").get_to(p.mDependencies);
    }

    inline void from_json(const nlohmann::json& j, DiskDependencyData& environment)
    {
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            auto key = it.key();
            yaget::conv::Trim(key);
            auto value = it.value();

            from_json(value, environment[key]);
        }
    }


}

//-------------------------------------------------------------------------------------------------
yaget::DependencyNode::DependencyNode(const yaget::Guid& guid)
    : mGuid(guid) 
{}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyNode::Add(const yaget::Guid& guid)
{
    if (!FindNode(guid))
    {
        mDependencies.push_back(DependencyNode(guid));
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyNode* yaget::DependencyNode::FindNode(const Guid& guid) const
{
    if (mGuid == guid)
    {
        return const_cast<DependencyNode*>(this);
    }

    for (auto& dependency : mDependencies)
    {
        if (auto foundNode = dependency.FindNode(guid); foundNode != nullptr)
        {
            return foundNode;
        }
    }

    return nullptr;
}

void yaget::DependencyNode::ResolveNames(const io::VirtualTransportSystem& vts)
{
    auto tag = vts.FindTag(mGuid);
    if (tag.IsValid())
    {
        //io::VirtualTransportSystem::Section section(tag);
        //mName = section.ToString();
        mName = tag.mSectionName + "@" + tag.mName;
    }
    else if (!mName.empty())
    {
        mGuid = vts.GetTag(io::VirtualTransportSystem::Section(mName)).mGuid;
    }

    for (auto& element : mDependencies)
    {
        element.ResolveNames(vts);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::DependencyGraph(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& fileName)
    : mVTS(vts)
    , mSection(fileName)
{
    io::SingleBLobLoader<io::JsonAsset> cacheLoader(mVTS, mSection);
    if (auto asset = cacheLoader.GetAsset())
    {
        auto& root = asset->root;
        DiskDependencyData depData = root;
        for (auto& node : depData | std::views::values)
        {
            node.ResolveNames(mVTS);
        }

        mNodes = 
            depData | 
            std::views::transform([this](const auto& node)
            {
                return std::map<Guid, DependencyNode>::value_type{node.second.mGuid, node.second};
            }) | 
            std::ranges::to<std::map>();
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::~DependencyGraph()
{
    for (auto& node : mNodes | std::views::values)
    {
        node.ResolveNames(mVTS);
    }

    DiskDependencyData depNode = 
        mNodes | 
        std::views::transform([this](const auto& node)
        {
                return DiskDependencyData::value_type{node.second.mName, node.second};
        }) | 
        std::ranges::to<std::map>();

    const auto saveFileTag = mVTS.AssureTag(mSection);
    
    nlohmann::json jsonBlock = depNode;
    auto textBlock = json::PrettyPrint(jsonBlock);
    io::Buffer buffer = io::CreateBuffer(textBlock);

    io::SingleBLobLoader<io::JsonAsset> graphLoader(mVTS, saveFileTag);
    if (auto asset = graphLoader.GetAsset())
    {
        asset->mBuffer = buffer;
        mVTS.UpdateAssetData(asset, io::VirtualTransportSystem::Request::UpdateOnly);
    }
    else
    {
        const auto newAsset = io::ResolveAsset<io::JsonAsset>(buffer, saveFileTag, mVTS);
        mVTS.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyGraph::Add(const Guid& parentGuid, const Guid& childGuid)
{
    if (auto node = Find(parentGuid))
    {
        node->Add(childGuid);
    }
    else
    {
        auto n = mNodes.insert({ parentGuid, {parentGuid} });
        n.first->second.Add(childGuid);
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyNode* yaget::DependencyGraph::Find(const Guid& guid) const
{
    for (auto& val : mNodes | std::views::values)
    {
        if (auto foundNode = val.FindNode(guid); foundNode != nullptr)
        {
            return foundNode;
        }
    }

    return nullptr;
}
