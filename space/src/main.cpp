//
//  main.cpp
//  space
//
//  Created by Leonardo Murillo on 2/6/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <string.h>

#include "TreeNode.h"
#include "UDPSocket.h"

const char *CONFIG_FILE = "/Users/birarda/code/worklist/checkouts/hifi/space/example.data.txt";
const unsigned short SPACE_LISTENING_PORT = 55551;
const short MAX_NAME_LENGTH = 255;

const char ROOT_HOSTNAME[] = "root.highfidelity.co";
const char ROOT_NICKNAME[] = "root";

const size_t PACKET_LENGTH_BYTES = 1024;

sockaddr_in destAddress;
socklen_t destLength = sizeof(destAddress);

char *lastKnownHostname;

TreeNode rootNode;
UDPSocket spaceSocket(SPACE_LISTENING_PORT);

TreeNode *findOrCreateNode(int lengthInBits,
                           unsigned char *addressBytes,
                           char *hostname,
                           char *nickname,
                           int domainID) {
    
    TreeNode *currentNode = &rootNode;
    
    for (int i = 0; i < lengthInBits; i += 3) {
        unsigned char octet;
        
        if (i%8 < 6) {
            octet = addressBytes[i/8] << i%8 >> (5);
        } else {
            octet = (addressBytes[i/8] << i >>  (11 - i)) | (addressBytes[i/8 + 1] >> (11 - i + 2));
        }
        
        if (currentNode->child[octet] == NULL) {
            currentNode->child[octet] = new TreeNode;
        } else if (currentNode->child[octet]->hostname != NULL) {
            lastKnownHostname = currentNode->child[octet]->hostname;
        }
        
        currentNode = currentNode->child[octet];
    }
    
    if (currentNode->hostname == NULL) {
        currentNode->hostname = hostname;
        currentNode->nickname = nickname;
    }
    
    return currentNode;
};

bool loadSpaceData(void) {
    FILE *configFile = std::fopen(CONFIG_FILE, "r");
    char formatString[10];
    
    if (configFile == NULL) {
        std::cout << "Unable to load config file!\n";
        return false;
    } else {
        char *lengthBitString = new char[8];
        int itemsRead = 0;
        
        while ((itemsRead = fscanf(configFile, "0 %8c", lengthBitString)) > 0) {
            
            // calculate the number of bits in the address and bits required for padding
            unsigned long threeBitCodes = strtoul(lengthBitString, NULL, 2);
            int bitsInAddress = threeBitCodes * 3;
            int paddingBits = 8 - (bitsInAddress % 8);
            int addressByteLength = (bitsInAddress + paddingBits) / 8;
            
            // create an unsigned char * to hold the padded address
            unsigned char *paddedAddress = new unsigned char[addressByteLength];
            
            char *fullByteBitString = new char[8];
   
            for (int c = 0; c < addressByteLength; c++) {
                if (c + 1 == addressByteLength && paddingBits > 0) {
                    // this is the last byte, and we need some padding bits
                    // pull as many bits as are left
                    int goodBits = 8 - paddingBits;
                    sprintf(formatString, "%%%dc", goodBits);
                    itemsRead = fscanf(configFile, formatString, fullByteBitString);
                    
                    // fill out the rest with zeros
                    memset(fullByteBitString + goodBits, '0', paddingBits);
                } else {
                    // pull 8 bits (which will be one byte) from the file
                    itemsRead = fscanf(configFile, "%8c", fullByteBitString);
                }
                
                // set the corresponding value in the unsigned char array
                *(paddedAddress + c) = strtoul(fullByteBitString, NULL, 2);
            }
    
            char *nodeHostname = new char[MAX_NAME_LENGTH];
            char *nodeNickname = new char[MAX_NAME_LENGTH];
            itemsRead = fscanf(configFile, "%s %s\n", nodeHostname, nodeNickname);
         
            findOrCreateNode(bitsInAddress, paddedAddress, nodeHostname, nodeNickname, 0);
        }
        
        std::fclose(configFile);
        
        return true;
    }
}

int main (int argc, const char *argv[]) {
    
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    unsigned char packetData[PACKET_LENGTH_BYTES];
    ssize_t receivedBytes = 0;
    
    rootNode.hostname = new char[MAX_NAME_LENGTH];
    rootNode.nickname = new char[MAX_NAME_LENGTH];
    
    strcpy(rootNode.hostname, ROOT_HOSTNAME);
    strcpy(rootNode.nickname, ROOT_NICKNAME);
    
    loadSpaceData();
    
    std::cout << "[DEBUG] Listening for Datagrams" << std::endl;
    
    while (true) {
        if (spaceSocket.receive((sockaddr *)&destAddress, &packetData, &receivedBytes)) {
            unsigned long lengthInBits;
            lengthInBits = packetData[0] * 3;
            
            unsigned char addressData[sizeof(packetData)-1];
            for (int i = 0; i < sizeof(packetData)-1; ++i) {
                addressData[i] = packetData[i+1];
            }
            
            TreeNode *thisNode = findOrCreateNode(lengthInBits, addressData, NULL, NULL, 0);
            char *hostnameToSend = (thisNode->hostname == NULL)
                ? lastKnownHostname
                : thisNode->hostname;
            
            spaceSocket.send((sockaddr *)&destAddress, &hostnameToSend, sizeof(hostnameToSend));
        }
    }
}

