//
//  VoxelTreeElement.h
//  libraries/voxels/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelTreeElement_h
#define hifi_VoxelTreeElement_h

//#define HAS_AUDIT_CHILDREN
//#define SIMPLE_CHILD_ARRAY
#define SIMPLE_EXTERNAL_CHILDREN

#include <QReadWriteLock>

#include <AACube.h>
#include <OctreeElement.h>
#include <SharedUtil.h>

#include "ViewFrustum.h"
#include "VoxelConstants.h"

class VoxelTree;
class VoxelTreeElement;
class VoxelSystem;

class VoxelTreeElement : public OctreeElement {
    friend class VoxelTree; // to allow createElement to new us...
    
    VoxelTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL);
    
public:
    virtual ~VoxelTreeElement();
    virtual void init(unsigned char * octalCode);

    virtual bool hasContent() const { return isColored(); }
    virtual void splitChildren();
    virtual bool requiresSplit() const;
    virtual OctreeElement::AppendState appendElementData(OctreePacketData* packetData, EncodeBitstreamParams& params) const;
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    virtual void calculateAverageFromChildren();
    virtual bool collapseChildren();
    virtual bool findSpherePenetration(const glm::vec3& center, float radius, 
                        glm::vec3& penetration, void** penetratedObject) const;



    glBufferIndex getBufferIndex() const { return _glBufferIndex; }
    bool isKnownBufferIndex() const { return !_unknownBufferIndex; }
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; _unknownBufferIndex =(index == GLBUFFER_INDEX_UNKNOWN);}
    VoxelSystem* getVoxelSystem() const;
    void setVoxelSystem(VoxelSystem* voxelSystem);

    virtual bool isRendered() const { return isKnownBufferIndex(); }

    bool isColored() const { return _color[3] == 1; }

    void setColor(const nodeColor& color);
    const nodeColor& getColor() const { return _color; }

    void setDensity(float density) { _density = density; }
    float getDensity() const { return _density; }

    void setInteriorOcclusions(unsigned char interiorExclusions);
    void setExteriorOcclusions(unsigned char exteriorOcclusions);
    unsigned char getExteriorOcclusions() const;
    unsigned char getInteriorOcclusions() const;

    // type safe versions of OctreeElement methods
    VoxelTreeElement* getChildAtIndex(int childIndex) { return (VoxelTreeElement*)OctreeElement::getChildAtIndex(childIndex); }
    VoxelTreeElement* addChildAtIndex(int childIndex) { return (VoxelTreeElement*)OctreeElement::addChildAtIndex(childIndex); }
    
protected:

    uint32_t _glBufferIndex : 24; /// Client only, vbo index for this voxel if being rendered, 3 bytes
    uint32_t _voxelSystemIndex : 8; /// Client only, index to the VoxelSystem rendering this voxel, 1 bytes

    // Support for _voxelSystemIndex, we use these static member variables to track the VoxelSystems that are
    // in use by various voxel nodes. We map the VoxelSystem pointers into an 1 byte key, this limits us to at
    // most 255 voxel systems in use at a time within the client. Which is far more than we need.
    static uint8_t _nextIndex;
    static std::map<VoxelSystem*, uint8_t> _mapVoxelSystemPointersToIndex;
    static std::map<uint8_t, VoxelSystem*> _mapIndexToVoxelSystemPointers;

    float _density; /// Client and server, If leaf: density = 1, if internal node: 0-1 density of voxels inside, 4 bytes

    nodeColor _color; /// Client and server, true color of this voxel, 4 bytes

private:
    unsigned char _exteriorOcclusions;          ///< Exterior shared partition boundaries that are completely occupied
    unsigned char _interiorOcclusions;          ///< Interior shared partition boundaries with siblings
};

inline void VoxelTreeElement::setExteriorOcclusions(unsigned char exteriorOcclusions) { 
    if (_exteriorOcclusions != exteriorOcclusions) {
        _exteriorOcclusions = exteriorOcclusions; 
        setDirtyBit();
    }
}

inline void VoxelTreeElement::setInteriorOcclusions(unsigned char interiorOcclusions) { 
    if (_interiorOcclusions != interiorOcclusions) {
        _interiorOcclusions = interiorOcclusions; 
        setDirtyBit();
    }
}

inline unsigned char VoxelTreeElement::getInteriorOcclusions() const { 
    return _interiorOcclusions; 
}

inline unsigned char VoxelTreeElement::getExteriorOcclusions() const { 
    return _exteriorOcclusions; 
}

#endif // hifi_VoxelTreeElement_h
