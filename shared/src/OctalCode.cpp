//
//  OctalCode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/15/13.
//
//

#include <cmath>
#include <cstring>
#include "SharedUtil.h"
#include "OctalCode.h"

int numberOfThreeBitSectionsInCode(unsigned char * octalCode) {
    if (*octalCode == 255) {
        return *octalCode + numberOfThreeBitSectionsInCode(octalCode + 1);
    } else {
        return *octalCode;
    }
}

void printOctalCode(unsigned char * octalCode) {
    for (int i = 0; i < bytesRequiredForCodeLength(*octalCode); i++) {
        outputBits(octalCode[i]);
    }
}

char sectionValue(unsigned char * startByte, char startIndexInByte) {
    char rightShift = 8 - startIndexInByte - 3;
    
    if (rightShift < 0) {
        return ((startByte[0] << -rightShift) & 7) + (startByte[1] >> (8 + rightShift));
    } else {
        return (startByte[0] >> rightShift) & 7;
    }
}

int bytesRequiredForCodeLength(unsigned char threeBitCodes) {
    if (threeBitCodes == 0) {
        return 1;
    } else {
        return 1 + (int)ceilf((threeBitCodes * 3) / 8.0f);
    }
}

int branchIndexWithDescendant(unsigned char * ancestorOctalCode, unsigned char * descendantOctalCode) {
    int parentSections = numberOfThreeBitSectionsInCode(ancestorOctalCode);
    
    int branchStartBit = parentSections * 3;
    return sectionValue(descendantOctalCode + 1 + (branchStartBit / 8), branchStartBit % 8);
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

float * firstVertexForCode(unsigned char * octalCode) {
    float * firstVertex = new float[3];
    memset(firstVertex, 0, 3 * sizeof(float));
    
    float currentScale = 0.5;
    
    for (int i = 0; i < numberOfThreeBitSectionsInCode(octalCode); i++) {
        int sectionIndex = sectionValue(octalCode + 1 + (3 * i / 8), (3 * i) % 8);
        
        for (int j = 0; j < 3; j++) {
            firstVertex[j] += currentScale * (int)oneAtBit(sectionIndex, 5 + j);
        }
        
        currentScale *= 0.5;
    }
    
    return firstVertex;
}
