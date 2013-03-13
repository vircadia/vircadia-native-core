//
//  main.cpp
//  Voxel Server
//
//  Created by Stephen Birara on 03/06/13.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <sys/time.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <SharedUtil.h>

const int VOXEL_LISTEN_PORT = 40106;

const int NUMBER_OF_VOXELS = 250000;

const float MAX_UNIT_ANY_AXIS = 20.0f;

const int VERTICES_PER_VOXEL = 8;
const int VERTEX_POINTS_PER_VOXEL = 3 * VERTICES_PER_VOXEL;
const int COLOR_VALUES_PER_VOXEL = 3 * VERTICES_PER_VOXEL;

const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;

const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05;

char DOMAIN_HOSTNAME[] = "highfidelity.below92.com";
char DOMAIN_IP[100] = "192.168.1.47";    //  IP Address will be re-set by lookup on startup
const int DOMAINSERVER_PORT = 40102;

AgentList agentList(VOXEL_LISTEN_PORT);
in_addr_t localAddress;

unsigned char randomColorValue() {
    return MIN_BRIGHTNESS + (rand() % (255 - MIN_BRIGHTNESS));
}

void *reportAliveToDS(void *args) {
    
    timeval lastSend;
    unsigned char output[7];
    
    while (true) {
        gettimeofday(&lastSend, NULL);
        
        *output = 'V';
        packSocket(output + 1, localAddress, htons(VOXEL_LISTEN_PORT));
        agentList.getAgentSocket().send(DOMAIN_IP, DOMAINSERVER_PORT, output, 7);
        
        double usecToSleep = 1000000 - (usecTimestampNow() - usecTimestamp(&lastSend));
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "No sleep required!";
        }
    }
}

int main(int argc, const char * argv[])
{
    // get the local address of the voxel server
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    
    getifaddrs(&ifAddrStruct);
    
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            localAddress = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
        }
    }
    
    //  Lookup the IP address of things we have hostnames
    if (atoi(DOMAIN_IP) == 0) {
        struct hostent* pHostInfo;
        if ((pHostInfo = gethostbyname(DOMAIN_HOSTNAME)) != NULL) {
            sockaddr_in tempAddress;
            memcpy(&tempAddress.sin_addr, pHostInfo->h_addr_list[0], pHostInfo->h_length);
            strcpy(DOMAIN_IP, inet_ntoa(tempAddress.sin_addr));
            printf("Domain server %s: %s\n", DOMAIN_HOSTNAME, DOMAIN_IP);
            
        } else {
            printf("Failed lookup domainserver\n");
        }
    } else {
        printf("Using static domainserver IP: %s\n", DOMAIN_IP);
    }
    
    // setup the agentSocket to report to domain server
    pthread_t reportAliveThread;
    pthread_create(&reportAliveThread, NULL, reportAliveToDS, NULL);
    
    // new seed based on time now so voxels are different each time
    srand((unsigned)time(0));
    
    // create the vertex and color arrays to send back to requesting clients
    
    // setup data structures for our array of points and array of colors
    float *pointsArray = new float[NUMBER_OF_VOXELS * 3];
    char *colorsArray = new char[NUMBER_OF_VOXELS * 3];
    
    for (int n = 0; n < NUMBER_OF_VOXELS; n++) {
        
        // pick a random point for the center of the cube
        float azimuth = randFloat() * 2 * M_PI;
        float altitude = randFloat() * M_PI - M_PI/2;
        float radius = DEATH_STAR_RADIUS;
        float radius_twiddle = (DEATH_STAR_RADIUS/100) * powf(2, (float)(rand()%8));
        float thisScale = MAX_CUBE * 1 / (float)(rand() % 8 + 1);
        radius += radius_twiddle + (randFloat() * DEATH_STAR_RADIUS/12 - DEATH_STAR_RADIUS / 24);
        
        // fill the vertices array
        float *currentPointsPos = pointsArray + (n * 3);
        currentPointsPos[0] = radius * cosf(azimuth) * cosf(altitude);
        currentPointsPos[1] = radius * sinf(azimuth) * cosf(altitude);
        currentPointsPos[2] = radius * sinf(altitude);
        
        // fill the colors array
        char *currentColorPos = colorsArray + (n * 3);
        currentColorPos[0] = randomColorValue();
        currentColorPos[1] = randomColorValue();
        currentColorPos[2] = randomColorValue();
    }
    
    // we need space for each voxel and for the 'V' characters at the beginning of each voxel packet
    int voxelDataBytes = VOXEL_SIZE_BYTES * NUMBER_OF_VOXELS + ceil(NUMBER_OF_VOXELS / VOXELS_PER_PACKET);
    char *voxelData = new char[voxelDataBytes];
    
    // setup the interleaved voxelData packet
    for (int v = 0; v < NUMBER_OF_VOXELS; v++) {
        char *startPointer = voxelData + (v * VOXEL_SIZE_BYTES) + ((v / VOXELS_PER_PACKET) + 1);
        
        // if this is the start of a voxel packet we need to prepend with a 'V'
        if (v % VOXELS_PER_PACKET == 0) {
            *(startPointer - 1) = 'V';
        }
        
        memcpy(startPointer, pointsArray + (v * 3), sizeof(float) * 3);
        memcpy(startPointer + (3 * sizeof(float)), colorsArray + (v * 3), 3);
    }
    
    // delete the pointsArray and colorsArray that we no longer need
    delete[] pointsArray;
    delete[] colorsArray;
    
    sockaddr_in agentPublicAddress;
    
    char *packetData = new char[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
                             
    int sentVoxels = 0;
    
    // loop to send to agents requesting data
    while (true) {
        if (agentList.getAgentSocket().receive((sockaddr *)&agentPublicAddress, packetData, &receivedBytes)) {
            if (packetData[0] == 'I') {
                printf("Sending voxels to agent at address %s\n", inet_ntoa(agentPublicAddress.sin_addr));
                
                // send the agent all voxels for now
                // each packet has VOXELS_PER_PACKET, unless it's the last
                while (sentVoxels != NUMBER_OF_VOXELS) {
                    int voxelsToSend = std::min(VOXELS_PER_PACKET, NUMBER_OF_VOXELS - sentVoxels);
                    
                    agentList.getAgentSocket().send((sockaddr *)&agentPublicAddress,
                                                    voxelData + (sentVoxels * VOXEL_SIZE_BYTES) + ((sentVoxels / VOXELS_PER_PACKET)),
                                                    (voxelsToSend * VOXEL_SIZE_BYTES + 1));
                    
                    // sleep for 500 microseconds to not overload send and have lost packets
                    usleep(500);
                    
                    sentVoxels += voxelsToSend;
                }
                
                printf("Sent %d voxels to agent.\n", sentVoxels);
                sentVoxels = 0;
            }
        }
    }
    
    pthread_join(reportAliveThread, NULL);
    
    return 0;
}