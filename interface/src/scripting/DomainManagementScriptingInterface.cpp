//
//  DomainManagementScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Liv Erickson on 7/21/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DomainManagementScriptingInterface.h"
#include "EntitiesLogging.h"
#include "Application.h"

DomainManagementScriptingInterface::DomainManagementScriptingInterface()
{
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::canReplaceContentChanged, this, &DomainManagementScriptingInterface::canReplaceDomainContentChanged);
}

DomainManagementScriptingInterface::~DomainManagementScriptingInterface() {
    auto nodeList = DependencyManager::get<NodeList>();
    disconnect(nodeList.data(), &NodeList::canReplaceContentChanged, this, &DomainManagementScriptingInterface::canReplaceDomainContentChanged);
}

bool DomainManagementScriptingInterface::canReplaceDomainContent() {
    auto nodeList = DependencyManager::get<NodeList>();
    return nodeList->getThisNodeCanReplaceContent();
}

void DomainManagementScriptingInterface::replaceDomainContentSet(const QString url){
    if (DependencyManager::get<NodeList>()->getThisNodeCanReplaceContent()) {
        QByteArray _url(url.toUtf8());
        auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
        limitedNodeList->eachMatchingNode([](const SharedNodePointer& node) {
            return node->getType() == NodeType::EntityServer && node->getActiveSocket();
        }, [&_url, limitedNodeList](const SharedNodePointer& octreeNode) {
            auto octreeFilePacketList = NLPacketList::create(PacketType::OctreeFileReplacementFromUrl, QByteArray(), true, true);
            octreeFilePacketList->write(_url);
            qCDebug(entities) << "Attempting to send an octree file url to replace domain content";

            limitedNodeList->sendPacketList(std::move(octreeFilePacketList), *octreeNode);
        });
    };
}
