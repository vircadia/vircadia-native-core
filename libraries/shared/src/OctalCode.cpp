//
//  OctalCode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/15/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <algorithm> // std:min
#include <cassert>
#include <cmath>
#include <cstring>

#include <QDebug>

#include "SharedUtil.h"
#include "OctalCode.h"

int numberOfThreeBitSectionsInCode(unsigned char * octalCode) {
    assert(octalCode);
    if (*octalCode == 255) {
        return *octalCode + numberOfThreeBitSectionsInCode(octalCode + 1);
    } else {
        return *octalCode;
    }
}

void printOctalCode(unsigned char * octalCode) {
    if (!octalCode) {
        qDebug("NULL\n");
    } else {
        for (int i = 0; i < bytesRequiredForCodeLength(*octalCode); i++) {
            outputBits(octalCode[i],false);
        }
        qDebug("\n");
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

void copyFirstVertexForCode(unsigned char * octalCode, float* output) {
    memset(output, 0, 3 * sizeof(float));
    
    float currentScale = 0.5;
    
    for (int i = 0; i < numberOfThreeBitSectionsInCode(octalCode); i++) {
        int sectionIndex = sectionValue(octalCode + 1 + (3 * i / 8), (3 * i) % 8);
        
        for (int j = 0; j < 3; j++) {
            output[j] += currentScale * (int)oneAtBit(sectionIndex, 5 + j);
        }
        
        currentScale *= 0.5;
    }
}

float * firstVertexForCode(unsigned char * octalCode) {
    float * firstVertex = new float[3];
    copyFirstVertexForCode(octalCode, firstVertex);
    return firstVertex;
}

OctalCodeComparison compareOctalCodes(unsigned char* codeA, unsigned char* codeB) {
    if (!codeA || !codeB) {
        return ILLEGAL_CODE;
    }

    OctalCodeComparison result = LESS_THAN; // assume it's shallower
    
    int numberOfBytes = std::min(bytesRequiredForCodeLength(*codeA), bytesRequiredForCodeLength(*codeB));
    int compare = memcmp(codeA, codeB, numberOfBytes);

    if (compare < 0) {
        result = LESS_THAN;
    } else if (compare > 0) {
        result = GREATER_THAN;
    } else {
        int codeLengthA = numberOfThreeBitSectionsInCode(codeA);
        int codeLengthB = numberOfThreeBitSectionsInCode(codeB);

        if (codeLengthA == codeLengthB) {
            // if the memcmp matched exactly, and they were the same length,
            // then these must be the same code!
            result = EXACT_MATCH;
        } else {
            // if the memcmp matched exactly, but they aren't the same length,
            // then they have a matching common parent, but they aren't the same
            if (codeLengthA < codeLengthB) {
                result = LESS_THAN;
            } else {
                result = GREATER_THAN;
            }
        }
    }
    return result;
}


char getOctalCodeSectionValue(unsigned char* octalCode, int section) {
    int startAtByte = 1 + (BITS_IN_OCTAL * section / BITS_IN_BYTE);
    char startIndexInByte = (BITS_IN_OCTAL * section) % BITS_IN_BYTE;
    unsigned char* startByte = octalCode + startAtByte;
    
    return sectionValue(startByte, startIndexInByte);
}

void setOctalCodeSectionValue(unsigned char* octalCode, int section, char sectionValue) {
    int byteForSection = (BITS_IN_OCTAL * section / BITS_IN_BYTE);
    unsigned char* byteAt = octalCode + 1 + byteForSection;
    char bitInByte = (BITS_IN_OCTAL * section) % BITS_IN_BYTE;
    char shiftBy = BITS_IN_BYTE - bitInByte - BITS_IN_OCTAL;
    const unsigned char UNSHIFTED_MASK = 0x07;
    unsigned char shiftedMask;
    unsigned char shiftedValue;
    if (shiftBy >=0) {
        shiftedMask  = UNSHIFTED_MASK << shiftBy;
        shiftedValue = sectionValue << shiftBy;
    } else {
        shiftedMask  = UNSHIFTED_MASK >> -shiftBy;
        shiftedValue = sectionValue >> -shiftBy;
    }
    unsigned char oldValue = *byteAt & ~shiftedMask;
    unsigned char newValue = oldValue | shiftedValue;
    *byteAt = newValue;
    
    // If the requested section is partially in the byte, then we
    // need to also set the portion of the section value in the next byte
    // there's only two cases where this happens, if the bit in byte is
    // 6, then it means that 1 extra bit lives in the next byte. If the
    // bit in this byte is 7 then 2 extra bits live in the next byte.
    const int FIRST_PARTIAL_BIT = 6;
    if (bitInByte >= FIRST_PARTIAL_BIT) {
        int bitsInFirstByte  = BITS_IN_BYTE  - bitInByte;
        int bitsInSecondByte = BITS_IN_OCTAL - bitsInFirstByte;
        shiftBy = BITS_IN_BYTE - bitsInSecondByte;

        shiftedMask  = UNSHIFTED_MASK << shiftBy;
        shiftedValue = sectionValue << shiftBy;

        oldValue = byteAt[1] & ~shiftedMask;
        newValue = oldValue | shiftedValue;
        byteAt[1] = newValue;
    }
}

unsigned char* chopOctalCode(unsigned char* originalOctalCode, int chopLevels) {
    int codeLength = numberOfThreeBitSectionsInCode(originalOctalCode);
    unsigned char* newCode = NULL;
    if (codeLength > chopLevels) {
        int newLength = codeLength - chopLevels;
        newCode = new unsigned char[newLength+1];
        *newCode = newLength; // set the length byte
    
        for (int section = chopLevels; section < codeLength; section++) {
            char sectionValue = getOctalCodeSectionValue(originalOctalCode, section);
            setOctalCodeSectionValue(newCode, section - chopLevels, sectionValue);
        }
    }
    return newCode;
}

unsigned char* rebaseOctalCode(unsigned char* originalOctalCode, unsigned char* newParentOctalCode, bool includeColorSpace) {
    int oldCodeLength       = numberOfThreeBitSectionsInCode(originalOctalCode);
    int newParentCodeLength = numberOfThreeBitSectionsInCode(newParentOctalCode);
    int newCodeLength       = newParentCodeLength + oldCodeLength;
    int bufferLength        = newCodeLength + (includeColorSpace ? SIZE_OF_COLOR_DATA : 0);
    unsigned char* newCode  = new unsigned char[bufferLength];
    *newCode = newCodeLength; // set the length byte

    // copy parent code section first
    for (int sectionFromParent = 0; sectionFromParent < newParentCodeLength; sectionFromParent++) {
        char sectionValue = getOctalCodeSectionValue(newParentOctalCode, sectionFromParent);
        setOctalCodeSectionValue(newCode, sectionFromParent, sectionValue);
    }
    // copy original code section next
    for (int sectionFromOriginal = 0; sectionFromOriginal < oldCodeLength; sectionFromOriginal++) {
        char sectionValue = getOctalCodeSectionValue(originalOctalCode, sectionFromOriginal);
        setOctalCodeSectionValue(newCode, sectionFromOriginal + newParentCodeLength, sectionValue);
    }
    return newCode;
}

