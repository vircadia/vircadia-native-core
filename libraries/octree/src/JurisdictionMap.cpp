//
//  JurisdictionMap.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QDebug>

#include <DependencyManager.h>
#include <NodeList.h>
#include <udt/PacketHeaders.h>

#include "OctreeLogging.h"
#include "JurisdictionMap.h"

void myDebugOutputBits(unsigned char byte, bool withNewLine) {
    if (isalnum(byte)) {
        printf("[ %d (%c): ", byte, byte);
    } else {
        printf("[ %d (0x%x): ", byte, byte);
    }

    for (int i = 0; i < 8; i++) {
        printf("%d", byte >> (7 - i) & 1);
    }
    printf(" ] ");

    if (withNewLine) {
        printf("\n");
    }
}

void myDebugPrintOctalCode(const unsigned char* octalCode, bool withNewLine) {
    if (!octalCode) {
        printf("nullptr");
    } else {
        for (size_t i = 0; i < bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octalCode)); i++) {
            myDebugOutputBits(octalCode[i], false);
        }
    }
    if (withNewLine) {
        printf("\n");
    }
}

// standard assignment
// copy assignment
JurisdictionMap& JurisdictionMap::operator=(const JurisdictionMap& other) {
    copyContents(other);
    return *this;
}

// Copy constructor
JurisdictionMap::JurisdictionMap(const JurisdictionMap& other) : _rootOctalCode(nullptr) {
    copyContents(other);
}

void JurisdictionMap::copyContents(const OctalCodePtr& rootCodeIn, const OctalCodePtrList& endNodesIn) {
    OctalCodePtr rootCode = rootCodeIn;
    if (!rootCode) {
        rootCode = createOctalCodePtr(1);
        *rootCode = 0;
    }

    OctalCodePtrList emptyEndNodes;
    init(rootCode, endNodesIn);
}

void JurisdictionMap::copyContents(const JurisdictionMap& other) {
    _nodeType = other._nodeType;

    OctalCodePtr rootOctalCode;
    OctalCodePtrList endNodes;

    std::tie(rootOctalCode, endNodes) = other.getRootAndEndNodeOctalCodes();

    init(rootOctalCode, endNodes);
}

JurisdictionMap::~JurisdictionMap() {
}

JurisdictionMap::JurisdictionMap(NodeType_t type) : _rootOctalCode(nullptr) {
    _nodeType = type;
    OctalCodePtr rootCode = createOctalCodePtr(1);
    *rootCode = 0;

    OctalCodePtrList emptyEndNodes;
    init(rootCode, emptyEndNodes);
}

JurisdictionMap::JurisdictionMap(const char* filename) : _rootOctalCode(nullptr) {
    readFromFile(filename);
}

JurisdictionMap::JurisdictionMap(const char* rootHexCode, const char* endNodesHexCodes) {

    qCDebug(octree, "JurisdictionMap::JurisdictionMap(const char* rootHexCode=[%p] %s, const char* endNodesHexCodes=[%p] %s)",
        rootHexCode, rootHexCode, endNodesHexCodes, endNodesHexCodes);

    _rootOctalCode = hexStringToOctalCode(QString(rootHexCode));

    qCDebug(octree, "JurisdictionMap::JurisdictionMap() _rootOctalCode=%p octalCode=", _rootOctalCode.get());
    myDebugPrintOctalCode(_rootOctalCode.get(), true);

    QString endNodesHexStrings(endNodesHexCodes);
    QString delimiterPattern(",");
    QStringList endNodeList = endNodesHexStrings.split(delimiterPattern);

    for (int i = 0; i < endNodeList.size(); i++) {
        QString endNodeHexString = endNodeList.at(i);

        auto endNodeOctcode = hexStringToOctalCode(endNodeHexString);

        qCDebug(octree, "JurisdictionMap::JurisdictionMap()  endNodeList(%d)=%s",
            i, endNodeHexString.toLocal8Bit().constData());

        //printOctalCode(endNodeOctcode);
        _endNodes.push_back(endNodeOctcode);

        qCDebug(octree, "JurisdictionMap::JurisdictionMap() endNodeOctcode=%p octalCode=", endNodeOctcode.get());
        myDebugPrintOctalCode(endNodeOctcode.get(), true);

    }
}

std::tuple<OctalCodePtr, OctalCodePtrList> JurisdictionMap::getRootAndEndNodeOctalCodes() const {
    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    return std::tuple<OctalCodePtr, OctalCodePtrList>(_rootOctalCode, _endNodes);
}

OctalCodePtr JurisdictionMap::getRootOctalCode() const {
    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    return _rootOctalCode;
}

OctalCodePtrList JurisdictionMap::getEndNodeOctalCodes() const {
    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    return _endNodes;
}

void JurisdictionMap::init(OctalCodePtr rootOctalCode, const OctalCodePtrList& endNodes) {
    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    _rootOctalCode = rootOctalCode;
    _endNodes = endNodes;
}

JurisdictionMap::Area JurisdictionMap::isMyJurisdiction(const unsigned char* nodeOctalCode, int childIndex) const {
    // to be in our jurisdiction, we must be under the root...

    std::lock_guard<std::mutex> lock(_octalCodeMutex);

    // if the node is an ancestor of my root, then we return ABOVE
    if (isAncestorOf(nodeOctalCode, _rootOctalCode.get())) {
        return ABOVE;
    }

    // otherwise...
    bool isInJurisdiction = isAncestorOf(_rootOctalCode.get(), nodeOctalCode, childIndex);
    // if we're under the root, then we can't be under any of the endpoints
    if (isInJurisdiction) {
        for (size_t i = 0; i < _endNodes.size(); i++) {
            bool isUnderEndNode = isAncestorOf(_endNodes[i].get(), nodeOctalCode);
            if (isUnderEndNode) {
                isInJurisdiction = false;
                break;
            }
        }
    }
    return isInJurisdiction ? WITHIN : BELOW;
}


bool JurisdictionMap::readFromFile(const char* filename) {
    QString settingsFile(filename);
    QSettings settings(settingsFile, QSettings::IniFormat);
    QString rootCode = settings.value("root","00").toString();
    qCDebug(octree) << "rootCode=" << rootCode;

    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    _rootOctalCode = hexStringToOctalCode(rootCode);
    printOctalCode(_rootOctalCode.get());

    settings.beginGroup("endNodes");
    const QStringList childKeys = settings.childKeys();
    QHash<QString, QString> values;
    foreach (const QString &childKey, childKeys) {
        QString childValue = settings.value(childKey).toString();
        values.insert(childKey, childValue);
        qCDebug(octree) << childKey << "=" << childValue;

        auto octcode = hexStringToOctalCode(childValue);
        printOctalCode(octcode.get());

        _endNodes.push_back(octcode);
    }
    settings.endGroup();
    return true;
}

void JurisdictionMap::displayDebugDetails() const {
    std::lock_guard<std::mutex> lock(_octalCodeMutex);

    QString rootNodeValue = octalCodeToHexString(_rootOctalCode.get());

    qCDebug(octree) << "root:" << rootNodeValue;

    for (size_t i = 0; i < _endNodes.size(); i++) {
        QString value = octalCodeToHexString(_endNodes[i].get());
        qCDebug(octree) << "End node[" << i << "]: " << rootNodeValue;
    }
}


bool JurisdictionMap::writeToFile(const char* filename) {
    QString     settingsFile(filename);
    QSettings   settings(settingsFile, QSettings::IniFormat);

    std::lock_guard<std::mutex> lock(_octalCodeMutex);

    QString rootNodeValue = octalCodeToHexString(_rootOctalCode.get());

    settings.setValue("root", rootNodeValue);

    settings.beginGroup("endNodes");
    for (size_t i = 0; i < _endNodes.size(); i++) {
        QString key = QString("endnode%1").arg(i);
        QString value = octalCodeToHexString(_endNodes[i].get());
        settings.setValue(key, value);
    }
    settings.endGroup();
    return true;
}

std::unique_ptr<NLPacket> JurisdictionMap::packEmptyJurisdictionIntoMessage(NodeType_t type) {
    int bytes = 0;
    auto packet = NLPacket::create(PacketType::Jurisdiction, sizeof(type) + sizeof(bytes));

    // Pack the Node Type in first byte
    packet->writePrimitive(type);
    // No root or end node details to pack!
    packet->writePrimitive(bytes);

    return packet; // includes header!
}

std::unique_ptr<NLPacket> JurisdictionMap::packIntoPacket() {
    auto packet = NLPacket::create(PacketType::Jurisdiction);

    // Pack the Node Type in first byte
    NodeType_t type = getNodeType();
    packet->writePrimitive(type);

    // add the root jurisdiction
    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    if (_rootOctalCode) {
        size_t bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(_rootOctalCode.get()));
        // No root or end node details to pack!
        packet->writePrimitive(bytes);
        packet->write(reinterpret_cast<char*>(_rootOctalCode.get()), bytes);

        // if and only if there's a root jurisdiction, also include the end nodes
        int endNodeCount = (int)_endNodes.size();
        packet->writePrimitive(endNodeCount);

        for (int i=0; i < endNodeCount; i++) {
            auto endNodeCode = _endNodes[i].get();
            size_t bytes = 0;
            if (endNodeCode) {
                bytes = bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(endNodeCode));
            }
            packet->writePrimitive(bytes);
            packet->write(reinterpret_cast<char*>(endNodeCode), bytes);
        }
    } else {
        int bytes = 0;
        packet->writePrimitive(bytes);
    }

    return packet;
}

int JurisdictionMap::unpackFromPacket(ReceivedMessage& message) {
    // read the root jurisdiction
    int bytes = 0;
    message.readPrimitive(&bytes);

    std::lock_guard<std::mutex> lock(_octalCodeMutex);
    _rootOctalCode = nullptr;
    _endNodes.clear();

    if (bytes > 0 && bytes <= message.getBytesLeftToRead()) {
        _rootOctalCode = createOctalCodePtr(bytes);
        message.read(reinterpret_cast<char*>(_rootOctalCode.get()), bytes);

        // if and only if there's a root jurisdiction, also include the end nodes
        int endNodeCount = 0;
        message.readPrimitive(&endNodeCount);
        
        for (int i = 0; i < endNodeCount; i++) {
            int bytes = 0;
            message.readPrimitive(&bytes);

            if (bytes <= message.getBytesLeftToRead()) {
                auto endNodeCode = createOctalCodePtr(bytes);
                message.read(reinterpret_cast<char*>(endNodeCode.get()), bytes);

                // if the endNodeCode was 0 length then don't add it
                if (bytes > 0) {
                    _endNodes.push_back(endNodeCode);
                }
            }
        }
    }

    return message.getPosition(); // excludes header
}
