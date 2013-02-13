//
//  main.cpp
//  space
//
//  Created by Leonardo Murillo on 2/6/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stack>
#include <bitset>
#include "TreeNode.h"

const char *CONFIG_FILE = "/etc/below92/spaceserver.data.txt";
const int UDP_PORT = 55551;
std::vector< std::vector<std::string> > configData;
sockaddr_in address, destAddress;
socklen_t destLength = sizeof(destAddress);

std::string ROOT_HOSTNAME = "root.highfidelity.co";
std::string ROOT_NICKNAME = "root";
std::string *LAST_KNOWN_HOSTNAME = new std::string();

const short PACKET_LENGTH_BYTES = 1024;

TreeNode rootNode;

void printBinaryValue(char element) {
    std::bitset<8> x(element);
    std::cout << "Printing binary value: " << x << std::endl;
}

int create_socket()
{
    //  Create socket
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf("Failed to create socket: %d\n", handle);
        return false;
    }
    
    return handle;
}

int network_init()
{
    int handle = create_socket();
    
    //  Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) UDP_PORT );
    
    if (bind(handle, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
        printf( "failed to bind socket\n" );
        return false;
    }
    
    return handle;
}

TreeNode *findOrCreateNode(unsigned long lengthInBits,
                           unsigned char *addressBytes,
                           std::string *hostname,
                           std::string *nickname,
                           int domain_id) {
    
    TreeNode *currentNode = &rootNode;
    
    for (int i = 0; i < lengthInBits; i += 3) {
        unsigned char octetA;
        unsigned char octetB;
        unsigned char octet;
        
        /*
         * @TODO Put those shifts into a nice single statement, leaving as is for now
         */
        if (i%8 < 6) {
            octetA = addressBytes[i/8] << i%8;
            octet = octetA >> (5);
        } else {
            octetA = addressBytes[i/8] << i;
            octetA = octetA >>  (11 - i);
            octetB = addressBytes[i/8 + 1] >> (11 - i + 2);
            octet = octetA | octetB;
        }
        
        if (currentNode->child[octet] == NULL) {
            currentNode->child[octet] = new TreeNode;
        } else if (!currentNode->child[octet]->hostname->empty()) {
            LAST_KNOWN_HOSTNAME = currentNode->child[octet]->hostname;
        }
        
        currentNode = currentNode->child[octet];
    }
    
    if (currentNode->hostname->empty()) {
        currentNode->hostname = hostname;
        currentNode->nickname = nickname;
    }
    
    return currentNode;
};

bool loadSpaceData(void) {
    std::ifstream configFile(CONFIG_FILE);
    std::string line;
    
    if (configFile.is_open()) {
        while (getline(configFile, line)) {
            std::istringstream iss(line);
            std::vector<std::string> thisEntry;
            copy(std::istream_iterator<std::string>(iss),
                 std::istream_iterator<std::string>(),
                 std::back_inserter(thisEntry));
            configData.push_back(thisEntry);
            thisEntry.clear();
        }
    } else {
        std::cout << "Unable to load config file\n";
        return false;
    }
    
    for (std::vector< std::vector<std::string> >::iterator it = configData.begin(); it != configData.end(); ++it) {
        std::string *thisAddress = &(*it)[1];
        std::string *thisHostname = &(*it)[2];
        std::string *thisNickname = &(*it)[3];
        
        char lengthByteString[8];
        unsigned long lengthByte;
        unsigned long bitsInAddress;
        std::size_t bytesForAddress;
        
        std::size_t lengthByteSlice = (*thisAddress).copy(lengthByteString, 8, 0);
        lengthByteString[lengthByteSlice] = '\0';
        lengthByte = strtoul(lengthByteString, NULL, 2);
        
        bitsInAddress = lengthByte * 3;
        bytesForAddress = (((bitsInAddress + 7) & ~7));
        char *addressBitStream = new char();
        std::size_t addressBitSlice = (*thisAddress).copy(addressBitStream, (*thisAddress).length(), 8);

        if (bitsInAddress != addressBitSlice) {
            std::cout << "[FATAL] Mismatching byte length: " << bitsInAddress
                      << " and address bits: " << sizeof(addressBitStream) << std::endl;
            return false;
        }
        
        char paddedBitString[bytesForAddress];
        strcpy(paddedBitString, addressBitStream);
        for (unsigned long i = addressBitSlice; i < bytesForAddress; ++i ) {
            paddedBitString[i] = '0';
        }
        paddedBitString[bytesForAddress] = '\0';
        
        std::string byteBufferHolder = *new std::string(paddedBitString);
        unsigned char addressBytes[bytesForAddress / 8];
        addressBytes[bytesForAddress / 8] = '\0';
        int j = 0;
        
        for (unsigned long i = 0; i < bytesForAddress; i += 8) {
            char *byteHolder = new char;
            unsigned long thisByte;
            byteBufferHolder.copy(byteHolder, 8, i);
            thisByte = strtoul(byteHolder, NULL, 2);
            addressBytes[j] = thisByte;
            ++j;
        }
        
        findOrCreateNode(bitsInAddress, addressBytes, thisHostname, thisNickname, 0);
    }
    return true;
}

int main (int argc, const char *argv[]) {
    int handle = network_init();
    unsigned char packetData[PACKET_LENGTH_BYTES];
    long received_bytes = 0;
    
    if (!handle) {
        std::cout << "[FATAL] Failed to create UDP listening socket" << std::endl;
        return 0;
    } else {
        std::cout << "[DEBUG] Socket started" << std::endl;
    }
    
    rootNode.hostname = &ROOT_HOSTNAME;
    rootNode.nickname = &ROOT_NICKNAME;
    
    loadSpaceData();
    
    std::cout << "[DEBUG] Listening for Datagrams" << std::endl;
    
    while (true) {
        received_bytes = recvfrom(handle, (unsigned char*)packetData, PACKET_LENGTH_BYTES, 0, (sockaddr*)&destAddress, &destLength);
        if (received_bytes > 0) {
            unsigned long lengthInBits;
            lengthInBits = packetData[0] * 3;
            
            unsigned char addressData[sizeof(packetData)-1];
            for (int i = 0; i < sizeof(packetData)-1; ++i) {
                addressData[i] = packetData[i+1];
            }
            std::string thisHostname;
            std::string thisNickname;
            std::string hostnameHolder;
            int domain_id = 0;
            long sentBytes;
            
            TreeNode thisNode = *findOrCreateNode(lengthInBits, addressData, &thisHostname, &thisNickname, domain_id);
            
            if (thisNode.hostname->empty()) {
                hostnameHolder = *LAST_KNOWN_HOSTNAME;
            } else {
                hostnameHolder = *thisNode.hostname;
            }
            
            char hostname[hostnameHolder.size() + 1];
            std::copy(hostnameHolder.begin(), hostnameHolder.end(), hostname);
            hostname[hostnameHolder.size()] = '\0';
            
            sentBytes = sendto(handle, &hostname, PACKET_LENGTH_BYTES,
                               0, (sockaddr*)&destAddress, sizeof(destAddress));
        }
    }
}

