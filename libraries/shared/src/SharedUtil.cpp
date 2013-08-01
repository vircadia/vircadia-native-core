//
//  SharedUtil.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <time.h>

#ifdef _WIN32
#include "Syssocket.h"
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <QDebug>

#include "OctalCode.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"

uint64_t usecTimestamp(timeval *time) {
    return (time->tv_sec * 1000000 + time->tv_usec);
}

uint64_t usecTimestampNow() {
    timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000000 + now.tv_usec);
}

float randFloat () {
    return (rand() % 10000)/10000.f;
}

int randIntInRange (int min, int max) {
    return min + (rand() % ((max + 1) - min));
}

float randFloatInRange (float min,float max) {
    return min + ((rand() % 10000)/10000.f * (max-min));
}

unsigned char randomColorValue(int miniumum) {
    return miniumum + (rand() % (256 - miniumum));
}

bool randomBoolean() {
    return rand() % 2;
}

bool shouldDo(float desiredInterval, float deltaTime) {
    return randFloat() < deltaTime / desiredInterval;
}

void outputBufferBits(unsigned char* buffer, int length, bool withNewLine) {
    for (int i = 0; i < length; i++) {
        outputBits(buffer[i], false);
    }
    if (withNewLine) {
        qDebug("\n");
    }
}

void outputBits(unsigned char byte, bool withNewLine) {
    if (isalnum(byte)) {
        qDebug("[ %d (%c): ", byte, byte);
    } else {
        qDebug("[ %d (0x%x): ", byte, byte);
    }
    
    for (int i = 0; i < 8; i++) {
        qDebug("%d", byte >> (7 - i) & 1);
    }
    qDebug(" ] ");
    
    if (withNewLine) {
        qDebug("\n");
    }
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

void setAtBit(unsigned char& byte, int bitIndex) {
    byte += (1 << (7 - bitIndex));
}

int  getSemiNibbleAt(unsigned char& byte, int bitIndex) {
    return (byte >> (6 - bitIndex) & 3); // semi-nibbles store 00, 01, 10, or 11
}

void setSemiNibbleAt(unsigned char& byte, int bitIndex, int value) {
    //assert(value <= 3 && value >= 0);
    byte += ((value & 3) << (6 - bitIndex)); // semi-nibbles store 00, 01, 10, or 11
}

bool isInEnvironment(const char* environment) {
    char* environmentString = getenv("HIFI_ENVIRONMENT");
    
    if (environmentString && strcmp(environmentString, environment) == 0) {
        return true;
    } else {
        return false;
    }
}

void switchToResourcesParentIfRequired() {
#ifdef __APPLE__
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    char path[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
        // error!
    }
    CFRelease(resourcesURL);
    
    chdir(path);
    chdir("..");
#endif
}

void loadRandomIdentifier(unsigned char* identifierBuffer, int numBytes) {
    // seed the the random number generator
    srand(time(NULL));
    
    for (int i = 0; i < numBytes; i++) {
        identifierBuffer[i] = rand() % 256;
    }
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

void sharedMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message) {
    fprintf(stdout, "%s", message.toLocal8Bit().constData());
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
//
// Complaints:  Brad :)
#define GUESS_OF_VOXELCODE_SIZE 10
#define MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE 1500
#define SIZE_OF_COLOR_DATA sizeof(rgbColor)
bool createVoxelEditMessage(unsigned char command, short int sequence, 
        int voxelCount, VoxelDetail* voxelDetails, unsigned char*& bufferOut, int& sizeOut) {
        
    bool success = true; // assume the best
    int messageSize = MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE; // just a guess for now
    int actualMessageSize = 3;
    unsigned char* messageBuffer = new unsigned char[messageSize];
    
    int numBytesPacketHeader = populateTypeAndVersion(messageBuffer, command);
    unsigned short int* sequenceAt = (unsigned short int*) &messageBuffer[numBytesPacketHeader];
  
    *sequenceAt = sequence;
    unsigned char* copyAt = &messageBuffer[numBytesPacketHeader + sizeof(sequence)];

    for (int i = 0; i < voxelCount && success; i++) {
        // get the coded voxel
        unsigned char* voxelData = pointToVoxel(voxelDetails[i].x,voxelDetails[i].y,voxelDetails[i].z,
            voxelDetails[i].s,voxelDetails[i].red,voxelDetails[i].green,voxelDetails[i].blue);
            
        int lengthOfVoxelData = bytesRequiredForCodeLength(*voxelData)+SIZE_OF_COLOR_DATA;
        
        // make sure we have room to copy this voxel
        if (actualMessageSize+lengthOfVoxelData > MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE) {
            success = false;
        } else {
            // add it to our message
            memcpy(copyAt, voxelData, lengthOfVoxelData);
            copyAt += lengthOfVoxelData;
            actualMessageSize += lengthOfVoxelData;
        }
        // cleanup
        delete[] voxelData;
    }

    if (success) {    
        // finally, copy the result to the output
        bufferOut = new unsigned char[actualMessageSize];
        sizeOut = actualMessageSize;
        memcpy(bufferOut, messageBuffer, actualMessageSize);
    }
    
    delete[] messageBuffer; // clean up our temporary buffer
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
    xTest = yTest = zTest = sTest = 0.5f;

    // First determine the voxelSize that will properly encode a 
    // voxel of size S.
    unsigned int voxelSizeInOctets = 1;
    while (sTest > s) {
        sTest /= 2.0;
        voxelSizeInOctets++;
    }

    unsigned int voxelSizeInBytes = bytesRequiredForCodeLength(voxelSizeInOctets); // (voxelSizeInBits/8)+1;
    unsigned int voxelBufferSize = voxelSizeInBytes + sizeof(rgbColor); // 3 for color

    // allocate our resulting buffer
    unsigned char* voxelOut = new unsigned char[voxelBufferSize];

    // first byte of buffer is always our size in octets
    voxelOut[0]=voxelSizeInOctets;

    sTest = 0.5f; // reset sTest so we can do this again.

    unsigned char byte = 0; // we will be adding coding bits here
    int bitInByteNDX = 0; // keep track of where we are in byte as we go
    int byteNDX = 1; // keep track of where we are in buffer of bytes as we go
    int octetsDone = 0;

    // Now we actually fill out the voxel code
    while (octetsDone < voxelSizeInOctets) {
        if (x >= xTest) {
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
        if (bitInByteNDX == 8) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (y >= yTest) {
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
        if (bitInByteNDX == 8) {
            voxelOut[byteNDX]=byte;
            byteNDX++;
            bitInByteNDX=0;
            byte=0;
        }

        if (z >= zTest) {
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
        if (bitInByteNDX == 8) {
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
    if (bitInByteNDX > 0 && bitInByteNDX < 8) {
        // Pad the last byte
        while (bitInByteNDX < 8) {
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

    qDebug("octets=%d\n",octets);
    qDebug("voxelSizeInBits=%d\n",voxelSizeInBits);
    qDebug("voxelSizeInBytes=%d\n",voxelSizeInBytes);
    qDebug("voxelSizeInOctets=%d\n",voxelSizeInOctets);
    qDebug("voxelBufferSize=%d\n",voxelBufferSize);
    
    for(int i=0;i<voxelBufferSize;i++) {
        qDebug("i=%d ",i);
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

// Inserts the value and key into three arrays sorted by the key array, the first array is the value,
// the second array is a sorted key for the value, the third array is the index for the value in it original
// non-sorted array 
// returns -1 if size exceeded
// originalIndexArray is optional
int insertIntoSortedArrays(void* value, float key, int originalIndex, 
                           void** valueArray, float* keyArray, int* originalIndexArray, 
                           int currentCount, int maxCount) {
            
    if (currentCount < maxCount) {
        int i = 0;
        if (currentCount > 0) {
            while (i < currentCount && key > keyArray[i]) {
                i++;
            }
            // i is our desired location
            // shift array elements to the right
            if (i < currentCount && i+1 < maxCount) {
                memmove(&valueArray[i + 1], &valueArray[i], sizeof(void*) * (currentCount - i));
                memmove(&keyArray[i + 1], &keyArray[i], sizeof(float) * (currentCount - i));
                if (originalIndexArray) {
                    memmove(&originalIndexArray[i + 1], &originalIndexArray[i], sizeof(int) * (currentCount - i));
                }
            }
        }
        // place new element at i
        valueArray[i] = value;
        keyArray[i] = key;
        if (originalIndexArray) {
            originalIndexArray[i] = originalIndex;
        }
        return currentCount + 1;
    }
    return -1; // error case
}

int removeFromSortedArrays(void* value, void** valueArray, float* keyArray, int* originalIndexArray, 
                           int currentCount, int maxCount) {

    int i = 0;
    if (currentCount > 0) {
        while (i < currentCount && value != valueArray[i]) {
            i++;
        }
        
        if (value == valueArray[i] && i < currentCount) {
            // i is the location of the item we were looking for
            // shift array elements to the left
            memmove(&valueArray[i], &valueArray[i + 1], sizeof(void*) * ((currentCount-1) - i));
            memmove(&keyArray[i], &keyArray[i + 1], sizeof(float) * ((currentCount-1) - i));
            if (originalIndexArray) {
                memmove(&originalIndexArray[i], &originalIndexArray[i + 1], sizeof(int) * ((currentCount-1) - i));
            }
            return currentCount-1;
        }
    }
    return -1; // error case
}
