//////////////////////////////////////////////////////////////////////
// DependencyGraph.h
//
//  Copyright 01/16/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include "Parsers/DependencyGraph.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "HashUtilities.h"
#include "Streams/Guid.h"
#include "Streams/Watcher.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget
{
    struct DependencyNode
    {
        using Section = io::VirtualTransportSystem::Section;

        DependencyNode() = default;
        DependencyNode(const Guid& guid);
        void Add(const Guid& guid);
        DependencyNode *FindNode(const Guid& guid, std::vector<DependencyNode*>* pathTo) const;

        bool IsSingleDepth() const;
        void ResolveNames(const io::VirtualTransportSystem& vts);
        inline bool operator<(const DependencyNode& other) const { return mGuid < other.mGuid; }

        // is this or any descendant dirty
        bool IsBranchDirty() const;

        Guid mGuid;
        std::string mName;
        bool mDirty = false;
        std::vector<DependencyNode> mDependencies;
    };


    class DependencyGraph
    {
    public:
        using Section = io::VirtualTransportSystem::Section;

        using DirtyCallback = std::function<void(const Guid& guid)>;
        DependencyGraph(io::VirtualTransportSystem& vts, const Section& fileName, DirtyCallback dirtyCallback);
        ~DependencyGraph();

        void Add(const Guid& parentGuid, const Guid& childGuid);

        DependencyNode *Find(const Guid& guid, std::vector<DependencyNode*>* pathTo) const;

    private:
        void AddWatchFiles(const Guid& guid);
        void RemoveWatchFiles(const Guid& guid);

        io::VirtualTransportSystem& mVTS;
        Section mSection;
        std::map<Guid, DependencyNode> mNodes;
        size_t mNodesHash{};

        io::Watcher mWatcher;
        std::set<Guid> mWatchedTags;
        DirtyCallback mDirtyCallback;
    };

}


//--------------------------------------------------------------------------------------------------
template <>
struct std::hash<yaget::DependencyNode>
{
    typedef yaget::DependencyNode argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& dependencyNode) const noexcept
    {
        std::hash<yaget::Guid> hasherGuid;
        std::hash<std::string> hasherName;
        auto hasher = [&]()
        {
            size_t seed = 0;
            yaget::conv::hash_combine(seed, hasherGuid(dependencyNode.mGuid), hasherName(dependencyNode.mName));
            for (const auto& childNode : dependencyNode.mDependencies)
            {
                yaget::conv::hash_combine(seed, operator()(childNode));
            }
            return seed;
        };

        return hasher();
    }
};
