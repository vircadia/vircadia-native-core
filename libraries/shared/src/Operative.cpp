//
//  Operative.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <glm/gtc/quaternion.hpp>

#include "NodeList.h"
#include "NodeTypes.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

#include "Operative.h"

const float BUG_VOXEL_SIZE = 0.0625f / 128;
glm::vec3 bugPosition  = glm::vec3(BUG_VOXEL_SIZE * 20.0, BUG_VOXEL_SIZE * 30.0, BUG_VOXEL_SIZE * 20.0);
glm::vec3 bugDirection = glm::vec3(0, 0, 1);
const int VOXELS_PER_BUG = 18;
glm::vec3 bugPathCenter = glm::vec3(BUG_VOXEL_SIZE * 150.0, BUG_VOXEL_SIZE * 30.0, BUG_VOXEL_SIZE * 150.0);
float bugPathRadius = BUG_VOXEL_SIZE * 140.0;
float bugPathTheta = 0.0;
float bugRotation = 0.0;
float bugAngleDelta = 0.2 * (M_PI / 180.0f);
bool moveBugInLine = false;

unsigned long packetsSent = 0;
unsigned long bytesSent = 0;

glm::vec3 rotatePoint(glm::vec3 point, float angle) {
    //  First, create the quaternion based on this angle of rotation
    glm::quat q(glm::vec3(0, -angle, 0));
    
    //  Next, create a rotation matrix from that quaternion
    glm::mat4 rotation = glm::mat4_cast(q);
    
    //  Transform the original vectors by the rotation matrix to get the new vectors
    glm::vec4 quatPoint(point.x, point.y, point.z, 0);
    glm::vec4 newPoint = quatPoint * rotation;
    
    return glm::vec3(newPoint.x, newPoint.y, newPoint.z);
}

class BugPart {
public:
    glm::vec3       partLocation;
    unsigned char   partColor[3];
    
    BugPart(const glm::vec3& location, unsigned char red, unsigned char green, unsigned char blue ) {
        partLocation = location;
        partColor[0] = red;
        partColor[1] = green;
        partColor[2] = blue;
    }
};

const BugPart bugParts[VOXELS_PER_BUG] = {
    
    // tail
    BugPart(glm::vec3( 0, 0, -3), 51, 51, 153) ,
    BugPart(glm::vec3( 0, 0, -2), 51, 51, 153) ,
    BugPart(glm::vec3( 0, 0, -1), 51, 51, 153) ,
    
    // body
    BugPart(glm::vec3( 0, 0,  0), 255, 200, 0) ,
    BugPart(glm::vec3( 0, 0,  1), 255, 200, 0) ,
    
    // head
    BugPart(glm::vec3( 0, 0,  2), 200, 0, 0) ,
    
    // eyes
    BugPart(glm::vec3( 1, 0,  3), 64, 64, 64) ,
    BugPart(glm::vec3(-1, 0,  3), 64, 64, 64) ,
    
    // wings
    BugPart(glm::vec3( 3, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3( 2, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3( 1, 0,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-1, 0,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-2, 1,  1), 0, 153, 0) ,
    BugPart(glm::vec3(-3, 1,  1), 0, 153, 0) ,
    
    
    BugPart(glm::vec3( 2, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3( 1, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3(-1, -1,  0), 153, 200, 0) ,
    BugPart(glm::vec3(-2, -1,  0), 153, 200, 0) ,
};

void removeOldBug() {
    VoxelDetail details[VOXELS_PER_BUG];
    unsigned char* bufferOut;
    int sizeOut;
    
    // Generate voxels for where bug used to be
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        
        glm::vec3 partAt = bugParts[i].partLocation * BUG_VOXEL_SIZE * (bugDirection.x < 0 ? -1.0f : 1.0f);
        glm::vec3 rotatedPartAt = rotatePoint(partAt, bugRotation);
        glm::vec3 offsetPartAt = rotatedPartAt + bugPosition;
        
        details[i].x = offsetPartAt.x;
        details[i].y = offsetPartAt.y;
        details[i].z = offsetPartAt.z;
        
        details[i].red   = bugParts[i].partColor[0];
        details[i].green = bugParts[i].partColor[1];
        details[i].blue  = bugParts[i].partColor[2];
    }
    
    // send the "erase message" first...
    PACKET_HEADER message = PACKET_HEADER_ERASE_VOXEL;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){
        
        ::packetsSent++;
        ::bytesSent += sizeOut;
        
        NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }
}

static void renderMovingBug() {
    VoxelDetail details[VOXELS_PER_BUG];
    unsigned char* bufferOut;
    int sizeOut;
    
    removeOldBug();
    
    // Move the bug...
    if (moveBugInLine) {
        bugPosition.x += (bugDirection.x * BUG_VOXEL_SIZE);
        bugPosition.y += (bugDirection.y * BUG_VOXEL_SIZE);
        bugPosition.z += (bugDirection.z * BUG_VOXEL_SIZE);
        
        // Check boundaries
        if (bugPosition.z > 1.0) {
            bugDirection.z = -1;
        }
        if (bugPosition.z < BUG_VOXEL_SIZE) {
            bugDirection.z = 1;
        }
    } else {
        
        //printf("bugPathCenter=(%f,%f,%f)\n", bugPathCenter.x, bugPathCenter.y, bugPathCenter.z);
        
        bugPathTheta += bugAngleDelta; // move slightly
        bugRotation  -= bugAngleDelta; // rotate slightly
        
        // If we loop past end of circle, just reset back into normal range
        if (bugPathTheta > (360.0f * PI_OVER_180)) {
            bugPathTheta = 0;
            bugRotation  = 0;
        }
        
        float x = bugPathCenter.x + bugPathRadius * cos(bugPathTheta);
        float z = bugPathCenter.z + bugPathRadius * sin(bugPathTheta);
        float y = bugPathCenter.y;
        
        bugPosition = glm::vec3(x, y, z);
        //printf("bugPathTheta=%f\n", bugPathTheta);
        //printf("bugRotation=%f\n", bugRotation);
    }
    
    //printf("bugPosition=(%f,%f,%f)\n", bugPosition.x, bugPosition.y, bugPosition.z);
    //printf("bugDirection=(%f,%f,%f)\n", bugDirection.x, bugDirection.y, bugDirection.z);
    // would be nice to add some randomness here...
    
    // Generate voxels for where bug is going to
    for (int i = 0; i < VOXELS_PER_BUG; i++) {
        details[i].s = BUG_VOXEL_SIZE;
        
        glm::vec3 partAt = bugParts[i].partLocation * BUG_VOXEL_SIZE * (bugDirection.x < 0 ? -1.0f : 1.0f);
        glm::vec3 rotatedPartAt = rotatePoint(partAt, bugRotation);
        glm::vec3 offsetPartAt = rotatedPartAt + bugPosition;
        
        details[i].x = offsetPartAt.x;
        details[i].y = offsetPartAt.y;
        details[i].z = offsetPartAt.z;
        
        details[i].red   = bugParts[i].partColor[0];
        details[i].green = bugParts[i].partColor[1];
        details[i].blue  = bugParts[i].partColor[2];
    }
    
    // send the "create message" ...
    PACKET_HEADER message = PACKET_HEADER_SET_VOXEL_DESTRUCTIVE;
    if (createVoxelEditMessage(message, 0, VOXELS_PER_BUG, (VoxelDetail*)&details, bufferOut, sizeOut)){
        
        ::packetsSent++;
        ::bytesSent += sizeOut;
        
        NodeList::getInstance()->broadcastToNodes(bufferOut, sizeOut, &NODE_TYPE_VOXEL_SERVER, 1);
        delete[] bufferOut;
    }
}

const int ACTUAL_FPS = 60;
const double OUR_FPS_IN_MILLISECONDS = 1000.0/ACTUAL_FPS; // determines FPS from our desired FPS
const int ANIMATE_VOXELS_INTERVAL_USECS = OUR_FPS_IN_MILLISECONDS * 1000.0; // converts from milliseconds to usecs

void Operative::run() {
    timeval lastSendTime = {};
    timeval lastDomainServerCheckIn = {};
    
    NodeList* nodeList = NodeList::getInstance();
    
    sockaddr nodePublicAddress;
    
    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    // change the owner type on our NodeList
    NodeList::getInstance()->setOwnerType(NODE_TYPE_AGENT);
    NodeList::getInstance()->setNodeTypesOfInterest(&NODE_TYPE_VOXEL_SERVER, 1);
    NodeList::getInstance()->getNodeSocket()->setBlocking(false);
    
    while (!shouldStop) {
        gettimeofday(&lastSendTime, NULL);
        
        renderMovingBug();
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        // Nodes sending messages to us...
        if (nodeList->getNodeSocket()->receive(&nodePublicAddress, packetData, &receivedBytes)) {
            NodeList::getInstance()->processNodeData(&nodePublicAddress, packetData, receivedBytes);
        }
        
        // dynamically sleep until we need to fire off the next set of voxels
        long long usecToSleep =  ANIMATE_VOXELS_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&lastSendTime));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Last send took too much time, not sleeping!\n";
        }
    }
    
    printf("Removing the old bird and dying.\n");
    
    // we've been told to stop, remove the last bug
    removeOldBug();
}