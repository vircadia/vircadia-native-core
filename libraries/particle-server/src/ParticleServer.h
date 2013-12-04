//
//  ParticleServer.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __particle_server__ParticleServer__
#define __particle_server__ParticleServer__

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <Assignment.h>

#include "civetweb.h"

#include "ParticlePersistThread.h"
#include "ParticleSendThread.h"
#include "ParticleServerConsts.h"
#include "ParticleServerPacketProcessor.h"

/// Handles assignments of type ParticleServer - sending particles to various clients.
class ParticleServer : public ThreadedAssignment {
public:                
    ParticleServer(const unsigned char* dataBuffer, int numBytes);
    
    ~ParticleServer();
    
    /// runs the particle server assignment
    void run();
    
    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    bool wantsDebugParticleSending() const { return _debugParticleSending; }
    bool wantsDebugParticleReceiving() const { return _debugParticleReceiving; }
    bool wantsVerboseDebug() const { return _verboseDebug; }
    bool wantShowAnimationDebug() const { return _shouldShowAnimationDebug; }
    bool wantDumpParticlesOnMove() const { return _dumpParticlesOnMove; }
    bool wantDisplayParticleStats() const { return _displayParticleStats; }

    ParticleTree& getServerTree() { return _serverTree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }
    
    int getPacketsPerClientPerInterval() const { return _packetsPerClientPerInterval; }
    
    static ParticleServer* GetInstance() { return _theInstance; }
    
    bool isInitialLoadComplete() const { return (_particlePersistThread) ? _particlePersistThread->isInitialLoadComplete() : true; }
    time_t* getLoadCompleted() { return (_particlePersistThread) ? _particlePersistThread->getLoadCompleted() : NULL; }
    uint64_t getLoadElapsedTime() const { return (_particlePersistThread) ? _particlePersistThread->getLoadElapsedTime() : 0; }
    
private:
    int _argc;
    const char** _argv;
    char** _parsedArgV;

    char _particlePersistFilename[MAX_FILENAME_LENGTH];
    int _packetsPerClientPerInterval;
    ParticleTree _serverTree; // this IS a reaveraging tree 
    bool _wantParticlePersist;
    bool _wantLocalDomain;
    bool _debugParticleSending;
    bool _shouldShowAnimationDebug;
    bool _displayParticleStats;
    bool _debugParticleReceiving;
    bool _dumpParticlesOnMove;
    bool _verboseDebug;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    ParticleServerPacketProcessor* _particleServerPacketProcessor;
    ParticlePersistThread* _particlePersistThread;
    
    NodeWatcher _nodeWatcher; // used to cleanup AGENT data when agents are killed
    
    void parsePayload();

    void initMongoose(int port);

    static int civetwebRequestHandler(struct mg_connection *connection);
    static ParticleServer* _theInstance;
    
    time_t _started;
    uint64_t _startedUSecs;
};

#endif // __particle_server__ParticleServer__
