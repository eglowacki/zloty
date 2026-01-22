#include "Parsers/DependencyGraph.h"
#include "VTS/ResolvedAssets.h"


namespace
{
    std::string MakeValidGuid(const std::string& id)
    {
        return "00000000-0000-0000-0000-00000000" + id;
    }


    template <typename Key, typename Value>
    std::size_t calculate_map_hash(const std::map<Key, Value>& m) 
    {
        std::size_t seed = 0;
        std::hash<Key> hash_key;
        std::hash<Value> hash_value;

        // Iterate through the map. std::map guarantees a consistent, sorted order.
        for (const auto& pair : m) 
        {
            std::size_t key_hash = hash_key(pair.first);
            std::size_t value_hash = hash_value(pair.second);

            // Combine the key and value hashes for the current pair
            std::size_t pair_hash = key_hash;
            yaget::conv::hash_combine(pair_hash, value_hash);

            // Combine the current pair's hash into the total seed
            yaget::conv::hash_combine(seed, pair_hash);
        }

        return seed;
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
        mName = conv::Convertor<Section>::ToString(Section(tag));
    }
    else if (!mName.empty())
    {
        mGuid = vts.GetTag(Section(mName)).mGuid;
    }

    for (auto& element : mDependencies)
    {
        element.ResolveNames(vts);
    }
}


namespace
{
    template <typename T>
    T TransformNodes(auto& dependencyData, yaget::io::VirtualTransportSystem& vts)
    {
        for (auto& node : dependencyData | std::views::values)
        {
            // disk file wil only have names, we need to resolve guids here
            node.ResolveNames(vts);
        }

        T nodes = 
            dependencyData | 
            std::views::transform([](const auto& node)
            {
                int z = 0;
                z;
                if constexpr (std::is_same_v<typename T::key_type, yaget::Guid>)
                {
                    return typename T::value_type{node.second.mGuid, node.second};
                }
                else
                {
                    return typename T::value_type{node.second.mName, node.second};
                }
            }) | 
            std::ranges::to<std::map>();

        return nodes;
    }

    std::map<yaget::Guid, yaget::DependencyNode> ReadDependencyNodes(const yaget::io::VirtualTransportSystem::Section& section, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        std::map<Guid, DependencyNode> depNodes;

        yaget::io::SingleBLobLoader<io::JsonAsset> cacheLoader(vts, section);
        if (auto asset = cacheLoader.GetAsset())
        {
            auto& root = asset->root;
            DiskDependencyData depData = root;
            depNodes = TransformNodes<std::map<Guid, DependencyNode>>(depData, vts);
        }

        return depNodes;
    }
    
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::DependencyGraph(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& fileName)
    : mVTS(vts)
    , mSection(fileName)
    , mNodes(ReadDependencyNodes(mSection, mVTS))
    , mNodesHash(calculate_map_hash(mNodes))
{
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::~DependencyGraph()
{
    std::map<std::string, DependencyNode> depNode = TransformNodes<std::map<std::string, DependencyNode>>(mNodes, mVTS);

    auto nodesHash = calculate_map_hash(mNodes);
    if (nodesHash != mNodesHash)
    {
        Section saveSection(mSection.Name + "Write@" + mSection.Filter);
        const auto saveFileTag = mVTS.AssureTag(saveSection);
        
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
