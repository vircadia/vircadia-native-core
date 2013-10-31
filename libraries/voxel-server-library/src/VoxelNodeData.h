//
//  VoxelNodeData.h
//  hifi
//
//  Created by Stephen Birarda on 3/21/13.
//
//

#ifndef __hifi__VoxelNodeData__
#define __hifi__VoxelNodeData__

#include <iostream>
#include <NodeData.h>
#include <VoxelQuery.h>

#include <CoverageMap.h>
#include <VoxelConstants.h>
#include <VoxelNodeBag.h>
#include <VoxelSceneStats.h>

class VoxelSendThread;
class VoxelServer;

class VoxelNodeData : public VoxelQuery {
public:
    VoxelNodeData(Node* owningNode);
    ~VoxelNodeData();

    void resetVoxelPacket();  // resets voxel packet to after "V" header

    void writeToPacket(unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _voxelPacket; }
    int getPacketLength() const { return (MAX_VOXEL_PACKET_SIZE - _voxelPacketAvailableBytes); }
    bool isPacketWaiting() const { return _voxelPacketWaiting; }

    bool packetIsDuplicate() const;
    bool shouldSuppressDuplicatePacket();

    int getAvailable() const { return _voxelPacketAvailableBytes; }
    int getMaxSearchLevel() const { return _maxSearchLevel; };
    void resetMaxSearchLevel() { _maxSearchLevel = 1; };
    void incrementMaxSearchLevel() { _maxSearchLevel++; };

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; };
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    VoxelNodeBag nodeBag;
    CoverageMap map;

    ViewFrustum& getCurrentViewFrustum() { return _currentViewFrustum; };
    ViewFrustum& getLastKnownViewFrustum() { return _lastKnownViewFrustum; };
    
    // These are not classic setters because they are calculating and maintaining state
    // which is set asynchronously through the network receive
    bool updateCurrentViewFrustum();
    void updateLastKnownViewFrustum();

    bool getViewSent() const { return _viewSent; };
    void setViewSent(bool viewSent);

    bool getViewFrustumChanging() const { return _viewFrustumChanging;            };
    bool getViewFrustumJustStoppedChanging() const { return _viewFrustumJustStoppedChanging; };

    bool moveShouldDump() const;

    uint64_t getLastTimeBagEmpty() const { return _lastTimeBagEmpty; };
    void setLastTimeBagEmpty(uint64_t lastTimeBagEmpty) { _lastTimeBagEmpty = lastTimeBagEmpty; };

    bool getCurrentPacketIsColor() const { return _currentPacketIsColor; };

    bool hasLodChanged() const { return _lodChanged; };
    
    VoxelSceneStats stats;
    
    void initializeVoxelSendThread(VoxelServer* voxelServer);
    bool isVoxelSendThreadInitalized() { return _voxelSendThread; }
    
    void dumpOutOfView();
    
private:
    VoxelNodeData(const VoxelNodeData &);
    VoxelNodeData& operator= (const VoxelNodeData&);
    
    bool _viewSent;
    unsigned char* _voxelPacket;
    unsigned char* _voxelPacketAt;
    int _voxelPacketAvailableBytes;
    bool _voxelPacketWaiting;

    unsigned char* _lastVoxelPacket;
    int _lastVoxelPacketLength;
    int _duplicatePacketCount;
    uint64_t _firstSuppressedPacket;

    int _maxSearchLevel;
    int _maxLevelReachedInLastSearch;
    ViewFrustum _currentViewFrustum;
    ViewFrustum _lastKnownViewFrustum;
    uint64_t _lastTimeBagEmpty;
    bool _viewFrustumChanging;
    bool _viewFrustumJustStoppedChanging;
    bool _currentPacketIsColor;

    VoxelSendThread* _voxelSendThread;

    // watch for LOD changes
    int _lastClientBoundaryLevelAdjust;
    float _lastClientVoxelSizeScale;
    bool _lodChanged;
    bool _lodInitialized;
};

#endif /* defined(__hifi__VoxelNodeData__) */
