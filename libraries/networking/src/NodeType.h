//
//  NodeType.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 05/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NodeType_h
#define hifi_NodeType_h

#pragma once

typedef quint8 NodeType_t;

namespace NodeType {
    const NodeType_t DomainServer = 'D';
    const NodeType_t EntityServer = 'o'; // was ModelServer
    const NodeType_t Agent = 'I';
    const NodeType_t AudioMixer = 'M';
    const NodeType_t AvatarMixer = 'W';
    const NodeType_t AssetServer = 'A';
    const NodeType_t MessagesMixer = 'm';
    const NodeType_t EntityScriptServer = 'S';
    const NodeType_t UpstreamAudioMixer = 'B';
    const NodeType_t UpstreamAvatarMixer = 'C';
    const NodeType_t DownstreamAudioMixer = 'a';
    const NodeType_t DownstreamAvatarMixer = 'w';
    const NodeType_t Unassigned = 1;

    void init();

    const QString& getNodeTypeName(NodeType_t nodeType);
    bool isUpstream(NodeType_t nodeType);
    bool isDownstream(NodeType_t nodeType);
    NodeType_t upstreamType(NodeType_t primaryType);
    NodeType_t downstreamType(NodeType_t primaryType);


    NodeType_t fromString(QString type);
}

typedef QSet<NodeType_t> NodeSet;

#endif // hifi_NodeType_h
