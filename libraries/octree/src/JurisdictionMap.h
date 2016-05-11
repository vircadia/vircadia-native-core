//
//  JurisdictionMap.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JurisdictionMap_h
#define hifi_JurisdictionMap_h

#include <map>
#include <stdint.h>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QUuid>

#include <shared/ReadWriteLockable.h>

#include <NLPacket.h>
#include <Node.h>
#include <OctalCode.h>

class JurisdictionMap {
public:
    enum Area {
        ABOVE,
        WITHIN,
        BELOW
    };

    // standard constructors
    JurisdictionMap(NodeType_t type = NodeType::EntityServer); // default constructor
    JurisdictionMap(const JurisdictionMap& other); // copy constructor

    // standard assignment
    JurisdictionMap& operator=(const JurisdictionMap& other);    // copy assignment

    // application constructors
    JurisdictionMap(const char* filename);
    JurisdictionMap(const char* rootHextString, const char* endNodesHextString);

    ~JurisdictionMap();

    Area isMyJurisdiction(const unsigned char* nodeOctalCode, int childIndex) const;

    bool writeToFile(const char* filename);
    bool readFromFile(const char* filename);

    // Provide an atomic way to get both the rootOctalCode and endNodeOctalCodes.
    std::tuple<OctalCodePtr, OctalCodePtrList> getRootAndEndNodeOctalCodes() const;
    OctalCodePtr getRootOctalCode() const;
    OctalCodePtrList getEndNodeOctalCodes() const;

    void copyContents(const OctalCodePtr& rootCodeIn, const OctalCodePtrList& endNodesIn);

    int unpackFromPacket(ReceivedMessage& message);
    std::unique_ptr<NLPacket> packIntoPacket();

    /// Available to pack an empty or unknown jurisdiction into a network packet, used when no JurisdictionMap is available
    static std::unique_ptr<NLPacket> packEmptyJurisdictionIntoMessage(NodeType_t type);

    void displayDebugDetails() const;

    NodeType_t getNodeType() const { return _nodeType; }
    void setNodeType(NodeType_t type) { _nodeType = type; }

private:
    void copyContents(const JurisdictionMap& other); // use assignment instead
    void init(OctalCodePtr rootOctalCode, const OctalCodePtrList& endNodes);

    mutable std::mutex _octalCodeMutex;
    OctalCodePtr _rootOctalCode { nullptr };
    OctalCodePtrList _endNodes;
    NodeType_t _nodeType;
};

/// Map between node IDs and their reported JurisdictionMap. Typically used by classes that need to know which nodes are
/// managing which jurisdictions.
class NodeToJurisdictionMap : public QMap<QUuid, JurisdictionMap>, public ReadWriteLockable {};
typedef QMap<QUuid, JurisdictionMap>::iterator NodeToJurisdictionMapIterator;


#endif // hifi_JurisdictionMap_h
