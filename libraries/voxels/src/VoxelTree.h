//
//  VoxelTree.h
//  libraries/voxels/src
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelTree_h
#define hifi_VoxelTree_h

#include <Octree.h>

#include "VoxelTreeElement.h"
#include "VoxelEditPacketSender.h"

class ReadCodeColorBufferToTreeArgs;

class VoxelTree : public Octree {
    Q_OBJECT
public:

    VoxelTree(bool shouldReaverage = false);

    virtual VoxelTreeElement* createNewElement(unsigned char * octalCode = NULL);
    VoxelTreeElement* getRoot() { return static_cast<VoxelTreeElement*>(_rootElement); }

    void deleteVoxelAt(float x, float y, float z, float s);
    
    /// Find the voxel at position x,y,z,s
    /// \return pointer to the VoxelTreeElement or NULL if none at x,y,z,s.
    VoxelTreeElement* getVoxelAt(float x, float y, float z, float s) const;
    
    /// Find the voxel at position x,y,z,s
    /// \return pointer to the VoxelTreeElement or to the smallest enclosing parent if none at x,y,z,s.
    VoxelTreeElement* getEnclosingVoxelAt(float x, float y, float z, float s) const;
    
    void createVoxel(float x, float y, float z, float s,
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);

    void nudgeSubTree(VoxelTreeElement* elementToNudge, const glm::vec3& nudgeAmount, VoxelEditPacketSender& voxelEditSender);

    /// reads voxels from square image with alpha as a Y-axis
    bool readFromSquareARGB32Pixels(const char *filename);

    /// reads from minecraft file
    bool readFromSchematicFile(const char* filename);

    void readCodeColorBufferToTree(const unsigned char* codeColorBuffer, bool destructive = false);

    virtual PacketType expectedDataPacketType() const { return PacketTypeVoxelData; }
    virtual bool handlesEditPacketType(PacketType packetType) const;
    virtual int processEditPacketData(PacketType packetType, const unsigned char* packetData, int packetLength,
                    const unsigned char* editData, int maxLength, const SharedNodePointer& node);
    virtual bool recurseChildrenWithData() const { return false; }

    virtual void dumpTree();

private:
    // helper functions for nudgeSubTree
    void recurseNodeForNudge(VoxelTreeElement* element, RecurseOctreeOperation operation, void* extraData);
    static bool nudgeCheck(OctreeElement* element, void* extraData);
    void nudgeLeaf(VoxelTreeElement* element, void* extraData);
    void chunkifyLeaf(VoxelTreeElement* element);
    void readCodeColorBufferToTreeRecursion(VoxelTreeElement* node, ReadCodeColorBufferToTreeArgs& args);
};

#endif // hifi_VoxelTree_h
