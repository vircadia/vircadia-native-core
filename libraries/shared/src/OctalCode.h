//
//  OctalCode.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 3/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctalCode_h
#define hifi_OctalCode_h

#include <vector>
#include <QString>

#include <memory>

const int BITS_IN_OCTAL = 3;
const int NUMBER_OF_COLORS = 3; // RGB!
const int SIZE_OF_COLOR_DATA = NUMBER_OF_COLORS * sizeof(unsigned char); // size in bytes
const int RED_INDEX   = 0;
const int GREEN_INDEX = 1;
const int BLUE_INDEX  = 2;

using OctalCodePtr = std::shared_ptr<unsigned char>;
using OctalCodePtrList = std::vector<OctalCodePtr>;

void printOctalCode(const unsigned char* octalCode);
size_t bytesRequiredForCodeLength(unsigned char threeBitCodes);
int branchIndexWithDescendant(const unsigned char* ancestorOctalCode, const unsigned char* descendantOctalCode);
unsigned char* childOctalCode(const unsigned char* parentOctalCode, char childNumber);

const int OVERFLOWED_OCTCODE_BUFFER = -1;
const int UNKNOWN_OCTCODE_LENGTH = -2;

/// will return -1 if there is an error in the octcode encoding, or it would overflow maxBytes
/// \param const unsigned char* octalCode the octalcode to decode
/// \param int maxBytes number of bytes that octalCode is expected to be, -1 if unknown
int numberOfThreeBitSectionsInCode(const unsigned char* octalCode, int maxBytes = UNKNOWN_OCTCODE_LENGTH);

unsigned char* chopOctalCode(const unsigned char* originalOctalCode, int chopLevels);

const int CHECK_NODE_ONLY = -1;
bool isAncestorOf(const unsigned char* possibleAncestor, const unsigned char* possibleDescendent, 
        int descendentsChild = CHECK_NODE_ONLY);

void copyFirstVertexForCode(const unsigned char* octalCode, float* output);

struct VoxelPositionSize {
    float x, y, z, s;
};
void voxelDetailsForCode(const unsigned char* octalCode, VoxelPositionSize& voxelPositionSize);

typedef enum {
    ILLEGAL_CODE = -2,
    LESS_THAN = -1,
    EXACT_MATCH = 0,
    GREATER_THAN = 1
} OctalCodeComparison;

OctalCodeComparison compareOctalCodes(const unsigned char* code1, const unsigned char* code2);

OctalCodePtr createOctalCodePtr(size_t size);
QString octalCodeToHexString(const unsigned char* octalCode);
OctalCodePtr hexStringToOctalCode(const QString& input);

#endif // hifi_OctalCode_h
