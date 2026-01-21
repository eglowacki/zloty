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

#include "YagetCore.h"
#include "Streams/Guid.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget
{
    struct DependencyNode 
    {
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
        DependencyGraph(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& fileName);
        ~DependencyGraph();

        void Add(const Guid& parentGuid, const Guid& childGuid);

        DependencyNode* Find(const Guid& guid) const;

    private:
        std::map<Guid, DependencyNode> mNodes;
        io::VirtualTransportSystem& mVTS;
        io::VirtualTransportSystem::Section mSection;
    };

}
