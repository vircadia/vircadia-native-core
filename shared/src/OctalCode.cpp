//
//  OctalCode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/15/13.
//
//

#include <cmath>
#include "SharedUtil.h"
#include "OctalCode.h"

int numberOfThreeBitSectionsInCode(unsigned char * octalCode) {
    if (*octalCode == 255) {
        return *octalCode + numberOfThreeBitSectionsInCode(octalCode + 1);
    } else {
        return *octalCode;
    }
}

int bytesRequiredForCodeLength(unsigned char threeBitCodes) {
    if (threeBitCodes == 0) {
        return 1;
    } else {
        return 1 + (int)ceilf((threeBitCodes * 3) / 8.0);
    }
    
}

void printOctalCode(unsigned char * octalCode) {
    for (int i = 1; i < bytesRequiredForCodeLength(*octalCode); i++) {
        outputBits(octalCode[i]);
    }
}

unsigned char * childOctalCode(unsigned char * parentOctalCode, char childNumber) {
    
    // find the length (in number of three bit code sequences)
    // in the parent
    int parentCodeSections = parentOctalCode != NULL
        ? numberOfThreeBitSectionsInCode(parentOctalCode)
        : 0;
    
    // get the number of bytes used by the parent octal code
    int parentCodeBytes = bytesRequiredForCodeLength(parentCodeSections);
    
    // child code will have one more section than the parent
    int childCodeBytes = bytesRequiredForCodeLength(parentCodeSections + 1);
    
    // create a new buffer to hold the new octal code
    unsigned char *newCode = new unsigned char[childCodeBytes];
    
    // copy the parent code to the child
    if (parentOctalCode != NULL) {
        memcpy(newCode, parentOctalCode, parentCodeBytes);
    }    
    
    // the child octal code has one more set of three bits
    *newCode = parentCodeSections + 1;
    
    if (childCodeBytes > parentCodeBytes) {
        // we have a new byte due to the addition of the child code
        // so set it to zero for correct results when shifting later
        newCode[childCodeBytes - 1] = 0;
    }
    
    // add the child code bits to newCode
    
    // find the start bit index
    int startBit = parentCodeSections * 3;
    
    // calculate the amount of left shift required
    // this will be -1 or -2 if there's wrap
    char leftShift = 8 - (startBit % 8) - 3;
    
    if (leftShift < 0) {
        // we have a wrap-around to accomodate
        // right shift for the end of first byte
        // left shift for beginning of the second
        newCode[(startBit / 8) + 1] += childNumber >> (-1 * leftShift);
        newCode[(startBit / 8) + 2] += childNumber << (8 + leftShift);
    } else {
        // no wraparound, left shift and add
        newCode[(startBit / 8) + 1] += (childNumber << leftShift);
    }
    
    return newCode;
}
