//
//  OctreeQueryNode.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//
//

#ifndef __hifi__OctreeQueryNode__
#define __hifi__OctreeQueryNode__

#include <iostream>
#include <NodeData.h>
#include <OctreePacketData.h>
#include <OctreeQuery.h>

#include <CoverageMap.h>
#include <OctreeConstants.h>
#include <OctreeElementBag.h>
#include <OctreeSceneStats.h>

class OctreeSendThread;
class OctreeServer;

class OctreeQueryNode : public OctreeQuery {
public:
    OctreeQueryNode(Node* owningNode);
    virtual ~OctreeQueryNode();
    
    virtual PACKET_TYPE getMyPacketType() const = 0;

    void resetOctreePacket(bool lastWasSurpressed = false);  // resets octree packet to after "V" header

    void writeToPacket(const unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _octreePacket; }
    int getPacketLength() const { return (MAX_PACKET_SIZE - _octreePacketAvailableBytes); }
    bool isPacketWaiting() const { return _octreePacketWaiting; }

    bool packetIsDuplicate() const;
    bool shouldSuppressDuplicatePacket();

    int getAvailable() const { return _octreePacketAvailableBytes; }
    int getMaxSearchLevel() const { return _maxSearchLevel; }
    void resetMaxSearchLevel() { _maxSearchLevel = 1; }
    void incrementMaxSearchLevel() { _maxSearchLevel++; }

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; }
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    OctreeElementBag nodeBag;
    CoverageMap map;

    ViewFrustum& getCurrentViewFrustum() { return _currentViewFrustum; }
    ViewFrustum& getLastKnownViewFrustum() { return _lastKnownViewFrustum; }
    
    // These are not classic setters because they are calculating and maintaining state
    // which is set asynchronously through the network receive
    bool updateCurrentViewFrustum();
    void updateLastKnownViewFrustum();

    bool getViewSent() const { return _viewSent; }
    void setViewSent(bool viewSent);

    bool getViewFrustumChanging() const { return _viewFrustumChanging; }
    bool getViewFrustumJustStoppedChanging() const { return _viewFrustumJustStoppedChanging; }

    bool moveShouldDump() const;

    uint64_t getLastTimeBagEmpty() const { return _lastTimeBagEmpty; }
    void setLastTimeBagEmpty(uint64_t lastTimeBagEmpty) { _lastTimeBagEmpty = lastTimeBagEmpty; }

    bool getCurrentPacketIsColor() const { return _currentPacketIsColor; }
    bool getCurrentPacketIsCompressed() const { return _currentPacketIsCompressed; }
    bool getCurrentPacketFormatMatches() {
        return (getCurrentPacketIsColor() == getWantColor() && getCurrentPacketIsCompressed() == getWantCompression());
    }

    bool hasLodChanged() const { return _lodChanged; };
    
    OctreeSceneStats stats;
    
    void initializeOctreeSendThread(OctreeServer* octreeServer);
    bool isOctreeSendThreadInitalized() { return _octreeSendThread; }
    
    void dumpOutOfView();
    
private:
    OctreeQueryNode(const OctreeQueryNode &);
    OctreeQueryNode& operator= (const OctreeQueryNode&);
    
    bool _viewSent;
    unsigned char* _octreePacket;
    unsigned char* _octreePacketAt;
    int _octreePacketAvailableBytes;
    bool _octreePacketWaiting;

    unsigned char* _lastOctreePacket;
    int _lastOctreePacketLength;
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
    bool _currentPacketIsCompressed;

    OctreeSendThread* _octreeSendThread;

    // watch for LOD changes
    int _lastClientBoundaryLevelAdjust;
    float _lastClientOctreeSizeScale;
    bool _lodChanged;
    bool _lodInitialized;
    
    OCTREE_PACKET_SEQUENCE _sequenceNumber;
};

#endif /* defined(__hifi__OctreeQueryNode__) */
