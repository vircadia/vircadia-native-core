//
//  AgentList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "AgentList.h"
#include "AgentTypes.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "shared_Log.h"

#ifdef _WIN32
#include "Syssocket.h"
#else
#include <arpa/inet.h>
#endif

using shared_lib::printLog;

const char SOLO_AGENT_TYPES[3] = {
    AGENT_TYPE_AVATAR_MIXER,
    AGENT_TYPE_AUDIO_MIXER,
    AGENT_TYPE_VOXEL
};

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

bool silentAgentThreadStopFlag = false;
bool domainServerCheckinStopFlag = false;
bool pingUnknownAgentThreadStopFlag = false;

AgentList* AgentList::_sharedInstance = NULL;

AgentList* AgentList::createInstance(char ownerType, unsigned int socketListenPort) {
    if (_sharedInstance == NULL) {
        _sharedInstance = new AgentList(ownerType, socketListenPort);
    } else {
        printLog("AgentList createInstance called with existing instance.\n");
    }
    
    return _sharedInstance;
}

AgentList* AgentList::getInstance() {
    if (_sharedInstance == NULL) {
        printLog("AgentList getInstance called before call to createInstance. Returning NULL pointer.\n");
    }
    
    return _sharedInstance;
}

AgentList::AgentList(char newOwnerType, unsigned int newSocketListenPort) :
    _agentBuckets(),
    _numAgents(0),
    agentSocket(newSocketListenPort),
    ownerType(newOwnerType),
    socketListenPort(newSocketListenPort),
    lastAgentId(0)
{
    pthread_mutex_init(&mutex, 0);
}

AgentList::~AgentList() {
    // stop the spawned threads, if they were started
    stopSilentAgentRemovalThread();
    stopDomainServerCheckInThread();
    stopPingUnknownAgentsThread();
    
    pthread_mutex_destroy(&mutex);
}

UDPSocket& AgentList::getAgentSocket() {
    return agentSocket;
}

char AgentList::getOwnerType() {
    return ownerType;
}

unsigned int AgentList::getSocketListenPort() {
    return socketListenPort;
}

void AgentList::processAgentData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes) {
    switch (((char *)packetData)[0]) {
        case PACKET_HEADER_DOMAIN: {
            updateList(packetData, dataBytes);
            break;
        }
        case PACKET_HEADER_PING: {
            agentSocket.send(senderAddress, &PACKET_HEADER_PING_REPLY, 1);
            break;
        }
        case PACKET_HEADER_PING_REPLY: {
            handlePingReply(senderAddress);
            break;
        }
    }
}

void AgentList::processBulkAgentData(sockaddr *senderAddress, unsigned char *packetData, int numTotalBytes) {
    lock();
    
    // find the avatar mixer in our agent list and update the lastRecvTime from it
    Agent* bulkSendAgent = agentWithAddress(senderAddress);

    if (bulkSendAgent) {
        bulkSendAgent->setLastRecvTimeUsecs(usecTimestampNow());
        bulkSendAgent->recordBytesReceived(numTotalBytes);
    }

    unsigned char *startPosition = packetData;
    unsigned char *currentPosition = startPosition + 1;
    unsigned char packetHolder[numTotalBytes];
    
    packetHolder[0] = PACKET_HEADER_HEAD_DATA;
    
    uint16_t agentID = -1;
    
    while ((currentPosition - startPosition) < numTotalBytes) {
        currentPosition += unpackAgentId(currentPosition, &agentID);
        memcpy(packetHolder + 1, currentPosition, numTotalBytes - (currentPosition - startPosition));
        
        Agent* matchingAgent = agentWithID(agentID);
        
        if (!matchingAgent) {
            // we're missing this agent, we need to add it to the list
            addOrUpdateAgent(NULL, NULL, AGENT_TYPE_AVATAR, agentID);
            
            // TODO: this is a really stupid way to do this
            // Add a reverse iterator and go from the end of the list
            matchingAgent = agentWithID(agentID);
        }
        
        currentPosition += updateAgentWithData(matchingAgent,
                                               packetHolder,
                                               numTotalBytes - (currentPosition - startPosition));
    }
    
    unlock();
}

int AgentList::updateAgentWithData(sockaddr *senderAddress, unsigned char *packetData, size_t dataBytes) {
    // find the agent by the sockaddr
    Agent* matchingAgent = agentWithAddress(senderAddress);
    
    if (matchingAgent) {
        return updateAgentWithData(matchingAgent, packetData, dataBytes);
    } else {
        return 0;
    }
}

int AgentList::updateAgentWithData(Agent *agent, unsigned char *packetData, int dataBytes) {
    agent->setLastRecvTimeUsecs(usecTimestampNow());
    
    if (agent->getActiveSocket() != NULL) {
        agent->recordBytesReceived(dataBytes);
    }
    
    if (agent->getLinkedData() == NULL) {
        if (linkedDataCreateCallback != NULL) {
            linkedDataCreateCallback(agent);
        }
    }
    
    return agent->getLinkedData()->parseData(packetData, dataBytes);
}

Agent* AgentList::agentWithAddress(sockaddr *senderAddress) {
    for(AgentList::iterator agent = begin(); agent != end(); agent++) {
        if (agent->getActiveSocket() != NULL && socketMatch(agent->getActiveSocket(), senderAddress)) {
            return &(*agent);
        }
    }
    
    return NULL;
}

Agent* AgentList::agentWithID(uint16_t agentID) {
    for(AgentList::iterator agent = begin(); agent != end(); agent++) {
        if (agent->getAgentId() == agentID) {
            return &(*agent);
        }
    }

    return NULL;
}

uint16_t AgentList::getLastAgentId() {
    return lastAgentId;
}

void AgentList::increaseAgentId() {
    ++lastAgentId;
}

int AgentList::updateList(unsigned char *packetData, size_t dataBytes) {
    int readAgents = 0;

    char agentType;
    uint16_t agentId;
    
    // assumes only IPv4 addresses
    sockaddr_in agentPublicSocket;
    agentPublicSocket.sin_family = AF_INET;
    sockaddr_in agentLocalSocket;
    agentLocalSocket.sin_family = AF_INET;
    
    unsigned char *readPtr = packetData + 1;
    unsigned char *startPtr = packetData;
    
    while((readPtr - startPtr) < dataBytes) {
        agentType = *readPtr++;
        readPtr += unpackAgentId(readPtr, (uint16_t *)&agentId);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentPublicSocket);
        readPtr += unpackSocket(readPtr, (sockaddr *)&agentLocalSocket);
        
        addOrUpdateAgent((sockaddr *)&agentPublicSocket, (sockaddr *)&agentLocalSocket, agentType, agentId);
    }  

    return readAgents;
}

bool AgentList::addOrUpdateAgent(sockaddr *publicSocket, sockaddr *localSocket, char agentType, uint16_t agentId) {
    AgentList::iterator agent = end();
    
    if (publicSocket != NULL) {
        for (agent = begin(); agent != end(); agent++) {
            if (agent->matches(publicSocket, localSocket, agentType)) {
                // we already have this agent, stop checking
                break;
            }
        }
    } 
    
    if (agent == end()) {
        // we didn't have this agent, so add them
        Agent* newAgent = new Agent(publicSocket, localSocket, agentType, agentId);
        
        if (socketMatch(publicSocket, localSocket)) {
            // likely debugging scenario with two agents on local network
            // set the agent active right away
            newAgent->activatePublicSocket();
        }
        
        if (newAgent->getType() == AGENT_TYPE_AUDIO_MIXER && audioMixerSocketUpdate != NULL) {
            // this is an audio mixer
            // for now that means we need to tell the audio class
            // to use the local socket information the domain server gave us
            sockaddr_in *publicSocketIn = (sockaddr_in *)publicSocket;
            audioMixerSocketUpdate(publicSocketIn->sin_addr.s_addr, publicSocketIn->sin_port);
        } else if (newAgent->getType() == AGENT_TYPE_VOXEL || newAgent->getType() == AGENT_TYPE_AVATAR_MIXER) {
            // this is currently the cheat we use to talk directly to our test servers on EC2
            // to be removed when we have a proper identification strategy
            newAgent->activatePublicSocket();
        }
        
        addAgentToList(newAgent);
        
        return true;
    } else {
        
        if (agent->getType() == AGENT_TYPE_AUDIO_MIXER || agent->getType() == AGENT_TYPE_VOXEL) {
            // until the Audio class also uses our agentList, we need to update
            // the lastRecvTimeUsecs for the audio mixer so it doesn't get killed and re-added continously
            agent->setLastRecvTimeUsecs(usecTimestampNow());
        }
        
        // we had this agent already, do nothing for now
        return false;
    }    
}

void AgentList::addAgentToList(Agent* newAgent) {
    // find the correct array to add this agent to
    int bucketIndex = _numAgents / AGENTS_PER_BUCKET;
    
    if (!_agentBuckets[bucketIndex]) {
        _agentBuckets[bucketIndex] = new Agent*[AGENTS_PER_BUCKET]();
    }
    
    _agentBuckets[bucketIndex][_numAgents % AGENTS_PER_BUCKET] = newAgent;
    
    ++_numAgents;
    
    printLog("Added agent - ");
    Agent::printLog(*newAgent);
}

void AgentList::broadcastToAgents(unsigned char *broadcastData, size_t dataBytes, const char* agentTypes, int numAgentTypes) {
    for(AgentList::iterator agent = begin(); agent != end(); agent++) {
        // only send to the AgentTypes we are asked to send to.
        if (agent->getActiveSocket() != NULL && memchr(agentTypes, agent->getType(), numAgentTypes)) {
            // we know which socket is good for this agent, send there
            agentSocket.send(agent->getActiveSocket(), broadcastData, dataBytes);
        }
    }
}

void AgentList::handlePingReply(sockaddr *agentAddress) {
    for(AgentList::iterator agent = begin(); agent != end(); agent++) {
        // check both the public and local addresses for each agent to see if we find a match
        // prioritize the private address so that we prune erroneous local matches
        if (socketMatch(agent->getPublicSocket(), agentAddress)) {
            agent->activatePublicSocket();
            break;
        } else if (socketMatch(agent->getLocalSocket(), agentAddress)) {
            agent->activateLocalSocket();
            break;
        }
    }
}

Agent* AgentList::soloAgentOfType(char agentType) {
    if (memchr(SOLO_AGENT_TYPES, agentType, sizeof(SOLO_AGENT_TYPES)) != NULL) {
        for(AgentList::iterator agent = begin(); agent != end(); agent++) {
            if (agent->getType() == agentType) {
                return &(*agent);
            }
        }
    }
    
    return NULL;
}

void *pingUnknownAgents(void *args) {
    
    AgentList* agentList = (AgentList*) args;
    const int PING_INTERVAL_USECS = 1 * 1000000;
    
    timeval lastSend;
    
    while (!pingUnknownAgentThreadStopFlag) {
        gettimeofday(&lastSend, NULL);
        
        for(AgentList::iterator agent = agentList->begin();
            agent != agentList->end();
            agent++) {
            if (agent->getActiveSocket() == NULL
                && (agent->getPublicSocket() != NULL && agent->getLocalSocket() != NULL)) {
                // ping both of the sockets for the agent so we can figure out
                // which socket we can use
                agentList->getAgentSocket().send(agent->getPublicSocket(), &PACKET_HEADER_PING, 1);
                agentList->getAgentSocket().send(agent->getLocalSocket(), &PACKET_HEADER_PING, 1);
            }
        }
        
        double usecToSleep = PING_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    return NULL;
}

void AgentList::startPingUnknownAgentsThread() {
    pthread_create(&pingUnknownAgentsThread, NULL, pingUnknownAgents, (void *)this);
}

void AgentList::stopPingUnknownAgentsThread() {
    pingUnknownAgentThreadStopFlag = true;
    pthread_join(pingUnknownAgentsThread, NULL);
}

void *removeSilentAgents(void *args) {
    AgentList* agentList = (AgentList*) args;
    double checkTimeUSecs, sleepTime;
    
    while (!silentAgentThreadStopFlag) {
        checkTimeUSecs = usecTimestampNow();
        
        for(AgentList::iterator agent = agentList->begin(); agent != agentList->end(); ++agent) {
            
            if ((checkTimeUSecs - agent->getLastRecvTimeUsecs()) > AGENT_SILENCE_THRESHOLD_USECS
            	&& agent->getType() != AGENT_TYPE_VOXEL) {
                
                printLog("Killing agent - ");
                Agent::printLog(*agent);
                
                agent->setAlive(false);
            }
        }
        
        sleepTime = AGENT_SILENCE_THRESHOLD_USECS - (usecTimestampNow() - checkTimeUSecs);
        #ifdef _WIN32
        Sleep( static_cast<int>(1000.0f*sleepTime) );
        #else
        usleep(sleepTime);
        #endif
    }
    
    pthread_exit(0);
    return NULL;
}

void AgentList::startSilentAgentRemovalThread() {
    pthread_create(&removeSilentAgentsThread, NULL, removeSilentAgents, (void*) this);
}

void AgentList::stopSilentAgentRemovalThread() {
    silentAgentThreadStopFlag = true;
    pthread_join(removeSilentAgentsThread, NULL);
}

void *checkInWithDomainServer(void *args) {
    
    const int DOMAIN_SERVER_CHECK_IN_USECS = 1 * 1000000;
    
    AgentList* parentAgentList = (AgentList*) args;
    
    timeval lastSend;
    unsigned char output[7];
    
    in_addr_t localAddress = getLocalAddress();
    
    //  Lookup the IP address of the domain server if we need to
    if (atoi(DOMAIN_IP) == 0) {
        struct hostent* pHostInfo;
        if ((pHostInfo = gethostbyname(DOMAIN_HOSTNAME)) != NULL) {
            sockaddr_in tempAddress;
            memcpy(&tempAddress.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);
            strcpy(DOMAIN_IP, inet_ntoa(tempAddress.sin_addr));
            printLog("Domain server: %s - %s\n", DOMAIN_HOSTNAME, DOMAIN_IP);
            
        } else {
            printLog("Failed lookup domainserver\n");
        }
    } else printLog("Using static domainserver IP: %s\n", DOMAIN_IP);
    
    
    while (!domainServerCheckinStopFlag) {
        gettimeofday(&lastSend, NULL);
        
        output[0] = parentAgentList->getOwnerType();
        packSocket(output + 1, localAddress, htons(parentAgentList->getSocketListenPort()));
        
        parentAgentList->getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);
        
        double usecToSleep = DOMAIN_SERVER_CHECK_IN_USECS - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
    }
    
    pthread_exit(0);
    return NULL;
}

void AgentList::startDomainServerCheckInThread() {
    pthread_create(&checkInWithDomainServerThread, NULL, checkInWithDomainServer, (void*) this);
}

void AgentList::stopDomainServerCheckInThread() {
    domainServerCheckinStopFlag = true;
    pthread_join(checkInWithDomainServerThread, NULL);
}

AgentList::iterator AgentList::begin() const {
    Agent** agentBucket = NULL;
    
    for (int i = 0; i < _numAgents; i++) {
        if (i % AGENTS_PER_BUCKET == 0) {
            agentBucket =  _agentBuckets[i / AGENTS_PER_BUCKET];
        }
        
        if (agentBucket[i % AGENTS_PER_BUCKET]->isAlive()) {
            return AgentListIterator(this, i);
        }
    }
    
    return AgentListIterator(this, 0);
}

AgentList::iterator AgentList::end() const {
    return AgentListIterator(this, _numAgents);
}

AgentListIterator::AgentListIterator(const AgentList* agentList, int agentIndex) :
    _agentIndex(agentIndex) {
    _agentList = agentList;
}

AgentListIterator& AgentListIterator::operator=(const AgentListIterator& otherValue) {
    _agentList = otherValue._agentList;
    _agentIndex = otherValue._agentIndex;
    return *this;
}

bool AgentListIterator::operator==(const AgentListIterator &otherValue) {
    return _agentIndex == otherValue._agentIndex;
}

bool AgentListIterator::operator!=(const AgentListIterator &otherValue) {
    return !(*this == otherValue);
}

Agent& AgentListIterator::operator*() {
    Agent** agentBucket = _agentList->_agentBuckets[_agentIndex / AGENTS_PER_BUCKET];
    return *agentBucket[_agentIndex % AGENTS_PER_BUCKET];
}

Agent* AgentListIterator::operator->() {
    Agent** agentBucket = _agentList->_agentBuckets[_agentIndex / AGENTS_PER_BUCKET];
    return agentBucket[_agentIndex % AGENTS_PER_BUCKET];
}

AgentListIterator& AgentListIterator::operator++() {
    skipDeadAndStopIncrement();
    return *this;
}

AgentList::iterator AgentListIterator::operator++(int) {
    AgentListIterator newIterator = AgentListIterator(*this);
    skipDeadAndStopIncrement();
    return newIterator;
}

void AgentListIterator::skipDeadAndStopIncrement() {
    while (_agentIndex != _agentList->_numAgents) {
        ++_agentIndex;
        
        if (_agentIndex == _agentList->_numAgents) {
            break;
        } else if ((*(*this)).isAlive()) {
            // skip over the dead agents
            break;
        }
    }
}
