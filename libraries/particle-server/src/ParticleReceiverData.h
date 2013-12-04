//
//  ParticleReceiverData.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleReceiverData__
#define __hifi__ParticleReceiverData__

#include <iostream>
#include <NodeData.h>
#include <ParticlePacketData.h>
#include <ParticleQuery.h>

#include <CoverageMap.h>
#include <ParticleConstants.h>
#include <ParticleNodeBag.h>
#include <ParticleSceneStats.h>

class ParticleSendThread;
class ParticleServer;

class ParticleReceiverData : public ParticleQuery {
public:
    ParticleReceiverData(Node* owningNode);
    virtual ~ParticleReceiverData();

    void resetParticlePacket(bool lastWasSurpressed = false);  // resets particle packet to after "V" header

    void writeToPacket(const unsigned char* buffer, int bytes); // writes to end of packet

    const unsigned char* getPacket() const { return _particlePacket; }
    int getPacketLength() const { return (MAX_PACKET_SIZE - _particlePacketAvailableBytes); }
    bool isPacketWaiting() const { return _particlePacketWaiting; }

    bool packetIsDuplicate() const;
    bool shouldSuppressDuplicatePacket();

    int getAvailable() const { return _particlePacketAvailableBytes; }
    int getMaxSearchLevel() const { return _maxSearchLevel; }
    void resetMaxSearchLevel() { _maxSearchLevel = 1; }
    void incrementMaxSearchLevel() { _maxSearchLevel++; }

    int getMaxLevelReached() const { return _maxLevelReachedInLastSearch; }
    void setMaxLevelReached(int maxLevelReached) { _maxLevelReachedInLastSearch = maxLevelReached; }

    ParticleNodeBag nodeBag;
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
    
    ParticleSceneStats stats;
    
    void initializeParticleSendThread(ParticleServer* particleServer);
    bool isParticleSendThreadInitalized() { return _particleSendThread; }
    
    void dumpOutOfView();
    
private:
    ParticleReceiverData(const ParticleReceiverData &);
    ParticleReceiverData& operator= (const ParticleReceiverData&);
    
    bool _viewSent;
    unsigned char* _particlePacket;
    unsigned char* _particlePacketAt;
    int _particlePacketAvailableBytes;
    bool _particlePacketWaiting;

    unsigned char* _lastParticlePacket;
    int _lastParticlePacketLength;
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

    ParticleSendThread* _particleSendThread;

    // watch for LOD changes
    int _lastClientBoundaryLevelAdjust;
    float _lastClientParticleSizeScale;
    bool _lodChanged;
    bool _lodInitialized;
    
    PARTICLE_PACKET_SEQUENCE _sequenceNumber;
};

#endif /* defined(__hifi__ParticleReceiverData__) */
