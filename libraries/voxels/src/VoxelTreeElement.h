//
//  VoxelTreeElement.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//
//

#ifndef __hifi__VoxelTreeElement__
#define __hifi__VoxelTreeElement__

//#define HAS_AUDIT_CHILDREN
//#define SIMPLE_CHILD_ARRAY
#define SIMPLE_EXTERNAL_CHILDREN

#include <QReadWriteLock>

#include <OctreeElement.h>
#include <SharedUtil.h>

#include "AABox.h"
#include "ViewFrustum.h"
#include "VoxelConstants.h"

class VoxelTree;
class VoxelTreeElement;
class VoxelSystem;

class VoxelTreeElement : public OctreeElement {
    friend class VoxelTree; // to allow createElement to new us...
    
    VoxelTreeElement(unsigned char* octalCode = NULL);

    virtual OctreeElement* createNewElement(unsigned char* octalCode = NULL) const;
    
public:
    virtual ~VoxelTreeElement();
    virtual void init(unsigned char * octalCode);

    virtual bool hasContent() const { return isColored(); }
    virtual void splitChildren();
    virtual bool requiresSplit() const;
    virtual bool appendElementData(OctreePacketData* packetData) const;
    virtual int readElementDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    virtual void calculateAverageFromChildren();
    virtual bool collapseChildren();


    glBufferIndex getBufferIndex() const { return _glBufferIndex; }
    bool isKnownBufferIndex() const { return !_unknownBufferIndex; }
    void setBufferIndex(glBufferIndex index) { _glBufferIndex = index; _unknownBufferIndex =(index == GLBUFFER_INDEX_UNKNOWN);}
    VoxelSystem* getVoxelSystem() const;
    void setVoxelSystem(VoxelSystem* voxelSystem);

    virtual bool isRendered() const { return isKnownBufferIndex(); }

    bool isColored() const { return _trueColor[3] == 1; }

    void setFalseColor(colorPart red, colorPart green, colorPart blue);
    void setFalseColored(bool isFalseColored);
    bool getFalseColored() { return _falseColored; }
    void setColor(const nodeColor& color);
    const nodeColor& getTrueColor() const { return _trueColor; }
    const nodeColor& getColor() const { return _currentColor; }

    void setDensity(float density) { _density = density; }
    float getDensity() const { return _density; }
    
    // type safe versions of OctreeElement methods
    VoxelTreeElement* getChildAtIndex(int childIndex) { return (VoxelTreeElement*)OctreeElement::getChildAtIndex(childIndex); }
    VoxelTreeElement* addChildAtIndex(int childIndex) { return (VoxelTreeElement*)OctreeElement::addChildAtIndex(childIndex); }
    
protected:

    uint32_t _glBufferIndex : 24, /// Client only, vbo index for this voxel if being rendered, 3 bytes
             _voxelSystemIndex : 8; /// Client only, index to the VoxelSystem rendering this voxel, 1 bytes

    // Support for _voxelSystemIndex, we use these static member variables to track the VoxelSystems that are
    // in use by various voxel nodes. We map the VoxelSystem pointers into an 1 byte key, this limits us to at
    // most 255 voxel systems in use at a time within the client. Which is far more than we need.
    static uint8_t _nextIndex;
    static std::map<VoxelSystem*, uint8_t> _mapVoxelSystemPointersToIndex;
    static std::map<uint8_t, VoxelSystem*> _mapIndexToVoxelSystemPointers;

    float _density; /// Client and server, If leaf: density = 1, if internal node: 0-1 density of voxels inside, 4 bytes

    nodeColor _trueColor; /// Client and server, true color of this voxel, 4 bytes
    nodeColor _currentColor; /// Client only, false color of this voxel, 4 bytes
};

#endif /* defined(__hifi__VoxelTreeElement__) */