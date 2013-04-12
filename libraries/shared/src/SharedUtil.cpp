//
//  SharedUtil.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#include <cstdlib>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include "Syssocket.h"
#endif
#include "SharedUtil.h"
#include "OctalCode.h"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

double usecTimestamp(timeval *time) {
    return (time->tv_sec * 1000000.0 + time->tv_usec);
}

double usecTimestampNow() {
    timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000000.0 + now.tv_usec);
}

float randFloat () {
    return (rand() % 10000)/10000.f;
}

int randIntInRange (int min, int max) {
    return min + (rand() % (max - min));
}

float randFloatInRange (float min,float max) {
    return min + ((rand() % 10000)/10000.f * (max-min));
}

unsigned char randomColorValue(int miniumum) {
    return miniumum + (rand() % (255 - miniumum));
}

bool randomBoolean() {
    return rand() % 2;
}

void outputBits(unsigned char byte) {
    printf("%d: ", byte);
    
    for (int i = 0; i < 8; i++) {
        printf("%d", byte >> (7 - i) & 1);
    }
    
    printf("\n");
}

int numberOfOnes(unsigned char byte) {
    return (byte >> 7)
        + ((byte >> 6) & 1)
        + ((byte >> 5) & 1)
        + ((byte >> 4) & 1)
        + ((byte >> 3) & 1)
        + ((byte >> 2) & 1)
        + ((byte >> 1) & 1)
        + (byte & 1);
}

bool oneAtBit(unsigned char byte, int bitIndex) {
    return (byte >> (7 - bitIndex) & 1);
}

void switchToResourcesIfRequired() {
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
        // error!
    }
    CFRelease(resourcesURL);
    
    chdir(path);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line flag and option was
//              included while launching the application, and to get the option value
//              immediately following the flag. For example if you ran:
//                      ./app -i filename.txt
//              then you're using the "-i" flag to set the input file name.
// Usage:       char * inputFilename = getCmdOption(argc, argv, "-i");
// Complaints:  Brad :)
const char* getCmdOption(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0 && i+1 < argc) {
            // then return the next option
            return argv[i+1];
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    getCmdOption()
// Description: Handy little function to tell you if a command line option flag was
//              included while launching the application. Returns bool true/false
// Usage:       bool wantDump   = cmdOptionExists(argc, argv, "-d");
// Complaints:  Brad :)

bool cmdOptionExists(int argc, const char * argv[],const char* option) {
    // check each arg
    for (int i=0; i < argc; i++) {
        // if the arg matches the desired option
        if (strcmp(option,argv[i])==0) {
            // then return the next option
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    createVoxelEditMessage()
// Description: creates an "insert" or "remove" voxel message for a voxel code 
//              corresponding to the closest voxel which encloses a cube with
//              lower corners at x,y,z, having side of length S.  
//              The input values x,y,z range 0.0 <= v < 1.0
//              message should be either 'S' for SET or 'E' for ERASE
//
// IMPORTANT:   The buffer is returned to you a buffer which you MUST delete when you are
//              done with it.
//
// HACK ATTACK: Well, what if this is larger than the MTU? That's the caller's problem, we
//              just truncate the message
// Usage:       
//                  unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
//                  tree->readCodeColorBufferToTree(voxelData);
//                  delete voxelData;
//
// Complaints:  Brad :)
#define GUESS_OF_VOXELCODE_SIZE 10
#define MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE 1500
#define SIZE_OF_COLOR_DATA 3
bool createVoxelEditMessage(unsigned char command, short int sequence, 
        int voxelCount, VoxelDetail* voxelDetails, unsigned char*& bufferOut, int& sizeOut) {
        
    bool success = true; // assume the best
    int messageSize = MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE; // just a guess for now
    int actualMessageSize = 3;
    unsigned char* messageBuffer = new unsigned char[messageSize];
    unsigned short int* sequenceAt = (unsigned short int*)&messageBuffer[1];
    
    messageBuffer[0]=command;
    *sequenceAt=sequence;
    unsigned char* copyAt = &messageBuffer[3];

    for (int i=0;i<voxelCount && success;i++) {
        // get the coded voxel
        unsigned char* voxelData = pointToVoxel(voxelDetails[i].x,voxelDetails[i].y,voxelDetails[i].z,
            voxelDetails[i].s,voxelDetails[i].red,voxelDetails[i].green,voxelDetails[i].blue);
            
        int lengthOfVoxelData = bytesRequiredForCodeLength(*voxelData)+SIZE_OF_COLOR_DATA;
        
        // make sure we have room to copy this voxel
        if (actualMessageSize+lengthOfVoxelData > MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE) {
            success=false;
        } else {
            // add it to our message
            memcpy(copyAt,voxelData,lengthOfVoxelData);
            copyAt+=lengthOfVoxelData+SIZE_OF_COLOR_DATA;
            actualMessageSize+=lengthOfVoxelData+SIZE_OF_COLOR_DATA;
        }
        // cleanup
        delete voxelData;
    }

    if (success) {    
        // finally, copy the result to the output
        bufferOut = new unsigned char[actualMessageSize];
        sizeOut=actualMessageSize;
        memcpy(bufferOut,messageBuffer,actualMessageSize);
    }
    return success;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    pointToVoxel()
// Description: Given a universal point with location x,y,z this will return the voxel
//              voxel code corresponding to the closest voxel which encloses a cube with
//              lower corners at x,y,z, having side of length S.  
//              The input values x,y,z range 0.0 <= v < 1.0
// TO DO:       This code is not very DRY. It should be cleaned up to be DRYer.
// IMPORTANT:   The voxel is returned to you a buffer which you MUST delete when you are
//              done with it.
// Usage:       
//                  unsigned char* voxelData = pointToVoxel(x,y,z,s,red,green,blue);
//                  tree->readCodeColorBufferToTree(voxelData);
//                  delete voxelData;
//
// Complaints:  Brad :)
unsigned char* pointToVoxel(float x, float y, float z, float s, unsigned char r, unsigned char g, unsigned char b ) {

    float xTest, yTest, zTest, sTest; 
    xTest = yTest = zTest = sTest = 0.5; 

    // First determine the voxelSize that will properly encode a 
    // voxel of size S.
    int voxelSizeInBits = 0;
    while (sTest > s) {
        sTest /= 2.0;
        voxelSizeInBits+=3;
    }

	unsigned int voxelSizeInBytes = (voxelSizeInBits/8)+1;
	unsigned int voxelSizeInOctets = (voxelSizeInBits/3);
	unsigned int voxelBufferSize = voxelSizeInBytes+1+3; // 1 for size, 3 for color

    // allocate our resulting buffer
    unsigned char* voxelOut = new unsigned char[voxelBufferSize];
    
    // first byte of buffer is always our size in octets
    voxelOut[0]=voxelSizeInOctets;

    sTest = 0.5;  // reset sTest so we can do this again.

    unsigned char byte = 0; // we will be adding coding bits here
    int bitInByteNDX = 0; // keep track of where we are in byte as we go
    int byteNDX = 1; // keep track of where we are in buffer of bytes as we go
    int octetsDone = 0;

    // Now we actually fill out the voxel code
    while (octetsDone < voxelSizeInOctets) {
        if (x > xTest) { 
            //<write 1 bit>
            byte = (byte << 1) | true;
            xTest += sTest/2.0; 
        } else { 
            //<write 0 bit;>
            byte = (byte << 1) | false;
            xTest -= sTest/2.0; 
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX > 7) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (y > yTest) { 
            //<write 1 bit>
            byte = (byte << 1) | true;
            yTest += sTest/2.0; 
        } else { 
            //<write 0 bit;>
            byte = (byte << 1) | false;
            yTest -= sTest/2.0; 
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX > 7) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (z > zTest) { 
            //<write 1 bit>
            byte = (byte << 1) | true;
            zTest += sTest/2.0; 
        } else { 
            //<write 0 bit;>
            byte = (byte << 1) | false;
            zTest -= sTest/2.0; 
        }
        bitInByteNDX++;
        // If we've reached the last bit of the byte, then we want to copy this byte
        // into our buffer. And get ready to start on a new byte
        if (bitInByteNDX > 7) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        octetsDone++;
        sTest /= 2.0;
    } 

    // If we've got here, and we didn't fill the last byte, we need to zero pad this
    // byte before we copy it into our buffer.
    if (bitInByteNDX > 0 && bitInByteNDX < 7) {
        // Pad the last byte
        while (bitInByteNDX <= 7) {
            byte = (byte << 1) | false;
            bitInByteNDX++;
        }
        
        // Copy it into our output buffer
        voxelOut[byteNDX]=byte;
        byteNDX++;
    }
    // copy color data
    voxelOut[byteNDX]=r;
    voxelOut[byteNDX+1]=g;
    voxelOut[byteNDX+2]=b;

    return voxelOut;
}

void printVoxelCode(unsigned char* voxelCode) {
    unsigned char octets = voxelCode[0];
	unsigned int voxelSizeInBits = octets*3;
	unsigned int voxelSizeInBytes = (voxelSizeInBits/8)+1;
	unsigned int voxelSizeInOctets = (voxelSizeInBits/3);
	unsigned int voxelBufferSize = voxelSizeInBytes+1+3; // 1 for size, 3 for color

    printf("octets=%d\n",octets);
    printf("voxelSizeInBits=%d\n",voxelSizeInBits);
    printf("voxelSizeInBytes=%d\n",voxelSizeInBytes);
    printf("voxelSizeInOctets=%d\n",voxelSizeInOctets);
    printf("voxelBufferSize=%d\n",voxelBufferSize);
    
    for(int i=0;i<voxelBufferSize;i++) {
        printf("i=%d ",i);
        outputBits(voxelCode[i]);
    }
}

#ifdef _WIN32
    void usleep(int waitTime) {
        __int64 time1 = 0, time2 = 0, sysFreq = 0;

        QueryPerformanceCounter((LARGE_INTEGER *)&time1);
        QueryPerformanceFrequency((LARGE_INTEGER *)&sysFreq);
        do {
            QueryPerformanceCounter((LARGE_INTEGER *)&time2);
        } while( (time2 - time1) < waitTime);
    }
#endif
