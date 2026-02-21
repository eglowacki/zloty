#include "Parsers/DependencyGraph.h"
#include "VTS/ResolvedAssets.h"
#include "Json/JsonHelpers.h"


namespace
{
    using DiskDependencyData = std::map<std::string, yaget::DependencyNode>;


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
    void to_json(nlohmann::json& j, const DependencyNode& p)
    {
        if (p.mDependencies.empty())
        {
            j = nlohmann::json{ {"name", p.mName} };
        }
        else
        {
            if (p.IsSingleDepth())
            {
                Strings dependencies = p.mDependencies | std::views::transform([](const DependencyNode& node)
                    {
                        return node.mName;
                    }) | std::ranges::to<Strings>();

                j = nlohmann::json{ {"name", p.mName}, {"dependencies", dependencies} };
            }
            else
            {
                j = nlohmann::json{ {"name", p.mName}, {"dependencies", p.mDependencies} };
            }
        }
    }


    void from_json(const nlohmann::json& j, DependencyNode& p)
    {
        p.mName = json::GetValue(j, "name", std::string{});

        if (j.is_object())
        {
            p.mDependencies = json::GetValue(j, "dependencies", std::vector<DependencyNode>{});
        }
        else
        {
            if (auto dependencies = json::GetValue(j, "dependencies", Strings{}); !dependencies.empty())
            {
                for (const auto& dependency : dependencies)
                {
                    p.mDependencies.push_back(DependencyNode{});
                    p.mDependencies.back().mName = dependency;
                }
            }
        }
    }


    void from_json(const nlohmann::json& j, DiskDependencyData& environment)
    {
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            auto key = it.key();
            conv::Trim(key);
            auto value = it.value();
            from_json(value, environment[key]);
        }
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyNode::DependencyNode(const Guid& guid)
    : mGuid(guid)
{
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyNode::Add(const Guid& guid)
{
    if (!FindNode(guid, nullptr))
    {
        mDependencies.push_back(DependencyNode(guid));
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyNode* yaget::DependencyNode::FindNode(const Guid& guid, std::vector<DependencyNode*>* pathTo) const
{
    if (mGuid == guid)
    {
        if (pathTo)
        {
            pathTo->push_back(const_cast<DependencyNode*>(this));
        }
        return const_cast<DependencyNode*>(this);
    }

    if (pathTo)
    {
        pathTo->push_back(const_cast<DependencyNode*>(this));
    }

    for (auto& dependency : mDependencies)
    {
        if (auto foundNode = dependency.FindNode(guid, pathTo); foundNode != nullptr)
        {
            return foundNode;
        }
    }

    if (pathTo)
    {
        pathTo->pop_back();
    }
    return nullptr;
}


//-------------------------------------------------------------------------------------------------
bool yaget::DependencyNode::IsSingleDepth() const
{
    size_t depth = std::accumulate(mDependencies.begin(), mDependencies.end(), static_cast<size_t>(0),
        [](size_t sum, const DependencyNode& p) { return sum + p.mDependencies.size(); });

    return depth == 0;
}


//-------------------------------------------------------------------------------------------------
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


//-------------------------------------------------------------------------------------------------
bool yaget::DependencyNode::IsBranchDirty() const
{
    if (mDirty)
    {
        return true;
    }

    for (const auto& node : mDependencies)
    {
        if (node.IsBranchDirty())
        {
            return true;
        }
    }

    return false;
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyNode::ClearDirty()
{
    mDirty = false;
    for (auto& node : mDependencies)
    {
        node.ClearDirty();
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

        T nodes = dependencyData | std::views::transform([](const auto& node)
            {
                if constexpr (std::is_same_v<typename T::key_type, yaget::Guid>)
                {
                    return typename T::value_type{ node.second.mGuid, node.second };
                }
                else
                {
                    return typename T::value_type{ node.second.mName, node.second };
                }
            }) | std::ranges::to<std::map>();

        return nodes;
    }


    std::map<yaget::Guid, yaget::DependencyNode> ReadDependencyNodes(const yaget::io::VirtualTransportSystem::Section& section,
        yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        std::map<Guid, DependencyNode> depNodes;

        const auto& configBlock = dev::CurrentConfiguration().mDataLoaders;
        if (!configBlock.mSkipDependencyGraph)
        {
            if (auto asset = io::LoadJson(vts, section))
            {
                const auto& root = asset->root;

                try
                {
                    DiskDependencyData depData = root;
                    depNodes = TransformNodes<std::map<Guid, DependencyNode>>(depData, vts);
                    YLOG_CINFO("ASET", depNodes.empty(), "Loaded Dependency Nodes from: '%s:%s'.",
                        conv::Convertor<io::VirtualTransportSystem::Section>::ToString(section).c_str(), asset->mTag.ResolveVTS().c_str());
                }
                catch (nlohmann::json::exception& ex)
                {
                    YLOG_ERROR("ASET", "Exception during loading of Dependency Nodes from: '%s:%s'.\n\t%s",
                        conv::Convertor<io::VirtualTransportSystem::Section>::ToString(section).c_str(), asset->mTag.ResolveVTS().c_str(), ex.what());
                }
            }
        }

        return depNodes;
    }
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::DependencyGraph(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& fileName, DirtyCallback dirtyCallback)
    : mVTS(vts)
    , mSection(fileName)
    , mNodes(ReadDependencyNodes(mSection, mVTS))
    , mNodesHash(calculate_map_hash(mNodes))
    , mDirtyCallback(dirtyCallback)
{
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyGraph::~DependencyGraph()
{
    for (const Guid& guid : mWatchedTags)
    {
        std::hash<Guid> hasher;
        mWatcher.Remove(hasher(guid));
    }

    auto depNode = TransformNodes<DiskDependencyData>(mNodes, mVTS);

    auto nodesHash = calculate_map_hash(mNodes);
    if (nodesHash != mNodesHash)
    {
        Section saveSection(mSection.mName + "Write@" + mSection.mFilter);
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

        YLOG_INFO("ASET", "Loaded Dependency Nodes saved to : '%s:%s'.", conv::Convertor<io::VirtualTransportSystem::Section>::ToString(saveSection).c_str(),
            saveFileTag.ResolveVTS().c_str());
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyGraph::Add(const Guid& parentGuid, const Guid& childGuid)
{
    if (auto node = Find(parentGuid, nullptr))
    {
        mt::WriteLock writeLocker(mSharedMutex);
        node->Add(childGuid);
    }
    else
    {
        mt::WriteLock writeLocker(mSharedMutex);
        auto n = mNodes.insert({ parentGuid, {parentGuid} });
        n.first->second.Add(childGuid);
    }

    mt::WriteLock writeLocker(mSharedMutex);
    AddWatchFiles(parentGuid);
    AddWatchFiles(childGuid);
}


//-------------------------------------------------------------------------------------------------
yaget::DependencyNode* yaget::DependencyGraph::Find(const Guid& guid, std::vector<DependencyNode*>* pathTo) const
{
    mt::ReadLock readLocker(mSharedMutex);
    for (auto& val : mNodes | std::views::values)
    {
        if (auto foundNode = val.FindNode(guid, pathTo); foundNode != nullptr)
        {
            return foundNode;
        }
    }

    return nullptr;
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyGraph::ClearDirty(const Guid& guid)
{
    if (auto node = Find(guid, nullptr))
    {
        mt::WriteLock writeLocker(mSharedMutex);
        node->ClearDirty();
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyGraph::AddWatchFiles(const Guid& guid)
{
    if (!mWatchedTags.contains(guid))
    {
        auto tag = mVTS.FindTag(guid);
        if (auto shaderFilePath = tag.ResolveVTS(); !shaderFilePath.empty())
        {
            mWatchedTags.insert(guid);

            std::hash<Guid> hasher;
            mWatcher.Add(hasher(guid), shaderFilePath, [this, This = this, tag]()
            {
                std::vector<DependencyNode*> pathTo;
                if (DependencyNode* shaderNode = Find(tag.mGuid, &pathTo))
                {
                    mt::WriteLock writeLocker(mSharedMutex);
                    std::ranges::for_each(pathTo, [](auto& node)
                    {
                        node->mDirty = true;
                    });
                }

                mDirtyCallback(tag.mGuid);
            });
        }
    }
}


//-------------------------------------------------------------------------------------------------
void yaget::DependencyGraph::RemoveWatchFiles(const Guid& guid)
{
    if (mWatchedTags.contains(guid))
    {
        mWatchedTags.erase(guid);
        std::hash<Guid> hasher;
        mWatcher.Remove(hasher(guid));
    }
}
