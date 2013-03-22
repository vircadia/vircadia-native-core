//
//  SharedUtil.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#include <cstdlib>
#include <cstdio>
#include "SharedUtil.h"

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
    if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) // Error: expected unqualified-id before 'if'
    {
        // error!
    }
    CFRelease(resourcesURL); // error: expected constructor, destructor or type conversion before '(' token
    
    chdir(path); // error: expected constructor, destructor or type conversion before '(' token
    std::cout << "Current Path: " << path << std::endl; // error: expected constructor, destructor or type conversion before '<<' token
#endif
}