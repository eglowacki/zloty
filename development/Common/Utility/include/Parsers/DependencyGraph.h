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
#include "YagetCore.h"
#include "Streams/Guid.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget
{
    struct DependencyNode 
    {
        using Section = io::VirtualTransportSystem::Section;

        DependencyNode() = default;
        DependencyNode(const Guid& guid);
        void Add(const Guid& guid);
        DependencyNode* FindNode(const Guid& guid) const;

        void ResolveNames(const io::VirtualTransportSystem& vts);

        inline bool operator<(const DependencyNode& other) const { return mGuid < other.mGuid; }

        Guid mGuid;
        std::string mName;
        std::vector<DependencyNode> mDependencies;
    };

    class DependencyGraph
    {
    public:
        using Section = io::VirtualTransportSystem::Section;

        DependencyGraph(io::VirtualTransportSystem& vts, const Section& fileName);
        ~DependencyGraph();

        void Add(const Guid& parentGuid, const Guid& childGuid);

        DependencyNode* Find(const Guid& guid) const;

    private:
        io::VirtualTransportSystem& mVTS;
        Section mSection;
        std::map<Guid, DependencyNode> mNodes;
        size_t mNodesHash{};
    };

}


//--------------------------------------------------------------------------------------------------
template <>
struct std::hash<yaget::DependencyNode>
{
    typedef yaget::DependencyNode argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const &dependencyNode) const
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
