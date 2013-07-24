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
#include <AvatarData.h>

#include <CoverageMap.h>
#include <VoxelConstants.h>
#include <VoxelNodeBag.h>
#include <VoxelSceneStats.h>

class VoxelNodeData : public AvatarData {
public:
    VoxelNodeData(Node* owningNode);
    ~VoxelNodeData();

    void resetVoxelPacket();  // resets voxel packet to after "V" header

    void writeToPacket(unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _voxelPacket; }
    int getPacketLength() const { return (MAX_VOXEL_PACKET_SIZE - _voxelPacketAvailableBytes); }
    bool isPacketWaiting() const { return _voxelPacketWaiting; }
    int getAvailable() const { return _voxelPacketAvailableBytes; }
    int getMaxSearchLevel() const { return _maxSearchLevel; };
    void resetMaxSearchLevel() { _maxSearchLevel = 1; };
    void incrementMaxSearchLevel() { _maxSearchLevel++; };

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; };
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    VoxelNodeBag nodeBag;
    CoverageMap map;

    ViewFrustum& getCurrentViewFrustum()     { return _currentViewFrustum; };
    ViewFrustum& getLastKnownViewFrustum()   { return _lastKnownViewFrustum; };
    
    // These are not classic setters because they are calculating and maintaining state
    // which is set asynchronously through the network receive
    bool updateCurrentViewFrustum();
    void updateLastKnownViewFrustum();

    bool getViewSent() const        { return _viewSent; };
    void setViewSent(bool viewSent);

    bool getViewFrustumChanging()            const { return _viewFrustumChanging;            };
    bool getViewFrustumJustStoppedChanging() const { return _viewFrustumJustStoppedChanging; };
    

    uint64_t  getLastTimeBagEmpty() const                      { return _lastTimeBagEmpty; };
    void      setLastTimeBagEmpty(uint64_t lastTimeBagEmpty)  { _lastTimeBagEmpty = lastTimeBagEmpty; };

    bool getCurrentPacketIsColor() const { return _currentPacketIsColor; };
    
    VoxelSceneStats stats;
    
private:
    VoxelNodeData(const VoxelNodeData &);
    VoxelNodeData& operator= (const VoxelNodeData&);
    
    bool _viewSent;
    unsigned char* _voxelPacket;
    unsigned char* _voxelPacketAt;
    int _voxelPacketAvailableBytes;
    bool _voxelPacketWaiting;
    int _maxSearchLevel;
    int _maxLevelReachedInLastSearch;
    ViewFrustum _currentViewFrustum;
    ViewFrustum _lastKnownViewFrustum;
    uint64_t _lastTimeBagEmpty;
    bool _viewFrustumChanging;
    bool _viewFrustumJustStoppedChanging;
    bool _currentPacketIsColor;
};

#endif /* defined(__hifi__VoxelNodeData__) */
