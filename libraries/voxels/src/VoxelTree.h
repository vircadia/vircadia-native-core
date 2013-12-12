//
//  VoxelTree.h
//  hifi
//
//  Created by Stephen Birarda on 3/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTree__
#define __hifi__VoxelTree__

#include <set>
#include <SimpleMovingAverage.h>
#include <OctreeElementBag.h>
#include <Octree.h>
#include <CoverageMap.h>
#include <JurisdictionMap.h>
#include <ViewFrustum.h>

#include "VoxelTreeElement.h"
#include "VoxelPacketData.h"
#include "VoxelSceneStats.h"
#include "VoxelEditPacketSender.h"

class ReadCodeColorBufferToTreeArgs;

class VoxelTree : public Octree {
    Q_OBJECT
public:

    VoxelTree(bool shouldReaverage = false);
    
    virtual VoxelTreeElement* createNewElement(unsigned char * octalCode = NULL) const;
    VoxelTreeElement* getRoot() { return (VoxelTreeElement*)_rootNode; }

    void deleteVoxelAt(float x, float y, float z, float s);
    VoxelTreeElement* getVoxelAt(float x, float y, float z, float s) const;
    void createVoxel(float x, float y, float z, float s, 
                     unsigned char red, unsigned char green, unsigned char blue, bool destructive = false);

    void createLine(glm::vec3 point1, glm::vec3 point2, float unitSize, rgbColor color, bool destructive = false);
    void createSphere(float radius, float xc, float yc, float zc, float voxelSize,
                             bool solid, creationMode mode, bool destructive = false, bool debug = false);

    void nudgeSubTree(VoxelTreeElement* elementToNudge, const glm::vec3& nudgeAmount, VoxelEditPacketSender& voxelEditSender);


    /// reads voxels from square image with alpha as a Y-axis
    bool readFromSquareARGB32Pixels(const char *filename);
    
    /// reads from minecraft file
    bool readFromSchematicFile(const char* filename);

    void readCodeColorBufferToTree(const unsigned char* codeColorBuffer, bool destructive = false);

    virtual bool handlesEditPacketType(PACKET_TYPE packetType) const;
    virtual int processEditPacketData(PACKET_TYPE packetType, unsigned char* packetData, int packetLength,
                    unsigned char* editData, int maxLength, Node* senderNode);
    void processSetVoxelsBitstream(const unsigned char* bitstream, int bufferSizeBytes);

/**
signals:
    void importSize(float x, float y, float z);
    void importProgress(int progress);

public slots:
    void cancelImport();
**/

private:
    // helper functions for nudgeSubTree
    void recurseNodeForNudge(VoxelTreeElement* element, RecurseOctreeOperation operation, void* extraData);
    static bool nudgeCheck(OctreeElement* element, void* extraData);
    void nudgeLeaf(VoxelTreeElement* element, void* extraData);
    void chunkifyLeaf(VoxelTreeElement* element);
    void readCodeColorBufferToTreeRecursion(VoxelTreeElement* node, ReadCodeColorBufferToTreeArgs& args);
};

#endif /* defined(__hifi__VoxelTree__) */
