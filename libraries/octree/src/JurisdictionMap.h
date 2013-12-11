//
//  JurisdictionMap.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 8/1/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__JurisdictionMap__
#define __hifi__JurisdictionMap__

#include <map>
#include <stdint.h>
#include <vector>

#include <QtCore/QString>
#include <QtCore/QUuid>

#include <NodeTypes.h>

class JurisdictionMap {
public:
    enum Area {
        ABOVE,
        WITHIN,
        BELOW
    };
    
    // standard constructors
    JurisdictionMap(NODE_TYPE type = NODE_TYPE_VOXEL_SERVER); // default constructor
    JurisdictionMap(const JurisdictionMap& other); // copy constructor

    // standard assignment
    JurisdictionMap& operator=(const JurisdictionMap& other);    // copy assignment

#ifdef HAS_MOVE_SEMANTICS
    // move constructor and assignment
    JurisdictionMap(JurisdictionMap&& other); // move constructor
    JurisdictionMap& operator= (JurisdictionMap&& other);         // move assignment
#endif
    
    // application constructors    
    JurisdictionMap(const char* filename);
    JurisdictionMap(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes);
    JurisdictionMap(const char* rootHextString, const char* endNodesHextString);
    ~JurisdictionMap();

    Area isMyJurisdiction(const unsigned char* nodeOctalCode, int childIndex) const;

    bool writeToFile(const char* filename);
    bool readFromFile(const char* filename);

    unsigned char* getRootOctalCode() const { return _rootOctalCode; }
    unsigned char* getEndNodeOctalCode(int index) const { return _endNodes[index]; }
    int getEndNodeCount() const { return _endNodes.size(); }

    void copyContents(unsigned char* rootCodeIn, const std::vector<unsigned char*>& endNodesIn);

    int unpackFromMessage(unsigned char* sourceBuffer, int availableBytes);
    int packIntoMessage(unsigned char* destinationBuffer, int availableBytes);
    
    /// Available to pack an empty or unknown jurisdiction into a network packet, used when no JurisdictionMap is available
    static int packEmptyJurisdictionIntoMessage(NODE_TYPE type, unsigned char* destinationBuffer, int availableBytes);

    void displayDebugDetails() const;
    
    NODE_TYPE getNodeType() const { return _nodeType; }
    void setNodeType(NODE_TYPE type) { _nodeType = type; }
    
private:
    void copyContents(const JurisdictionMap& other); // use assignment instead
    void clear();
    void init(unsigned char* rootOctalCode, const std::vector<unsigned char*>& endNodes);

    unsigned char* _rootOctalCode;
    std::vector<unsigned char*> _endNodes;
    NODE_TYPE _nodeType;
};

/// Map between node IDs and their reported JurisdictionMap. Typically used by classes that need to know which nodes are 
/// managing which jurisdictions.
typedef std::map<QUuid, JurisdictionMap> NodeToJurisdictionMap;


#endif /* defined(__hifi__JurisdictionMap__) */


