//
//  octal.cpp
//  interface
//
//  Created by Philip on 2/4/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Various subroutines for converting between X,Y,Z coords and octree coordinates.
//  

#include "Util.h"
#include "octal.h"
#include <cstring>

const int X = 0;
const int Y = 1;
const int Z = 2;

domainNode rootNode; 

//  Given a position vector between zero and one (but less than one), and a voxel scale 1/2^scale,
//  returns the smallest voxel at that scale which encloses the given point.
void getVoxel(float * pos, int scale, float * vpos) {
    float vscale = powf(2, scale);
    vpos[X] = floor(pos[X]*vscale)/vscale;
    vpos[Y] = floor(pos[Y]*vscale)/vscale;
    vpos[Z] = floor(pos[Z]*vscale)/vscale;
}

//
//  Given a pointer to an octal code and some domain owner data, iterate the tree and add the node(s) as needed.
//
domainNode* createNode(int lengthInBits, char * octalData,
                             char * hostname, char * nickname, int domain_id) {
    domainNode * currentNode = &rootNode;
    for (int i = 0;  i < lengthInBits; i+=3)  {
        char octet = 0;
        if (i%8 < 6) octet = octalData[i/8] << (i%8);
        else {
            octet = octalData[i/8] << (i%8)  && ((octalData[i/8 + 1] >> (i%8)) << (8 - i%8));
        }
        if (currentNode->child[octet] == NULL) currentNode->child[octet] = new domainNode;
        currentNode = currentNode->child[octet];
    }
    // Copy in the new node info...
    strcpy(currentNode->hostname,  hostname);
    strcpy(currentNode->nickname, nickname);
    currentNode->domain_id = domain_id;
    return currentNode;
}
