//
//  OctalCode.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 3/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm> // std:min
#include <cassert>
#include <cmath>
#include <cstring>

#include <QtCore/QDebug>

#include "NumericalConstants.h"
#include "OctalCode.h"
#include "SharedUtil.h"

int numberOfThreeBitSectionsInCode(const unsigned char* octalCode, int maxBytes) {
    if (maxBytes == OVERFLOWED_OCTCODE_BUFFER) {
        return OVERFLOWED_OCTCODE_BUFFER;
    }

    assert(octalCode);
    if (*octalCode == 255) {
        int newMaxBytes = (maxBytes == UNKNOWN_OCTCODE_LENGTH) ? UNKNOWN_OCTCODE_LENGTH : maxBytes - 1;
        return *octalCode + numberOfThreeBitSectionsInCode(octalCode + 1, newMaxBytes);
    } else {
        return *octalCode;
    }
}

void printOctalCode(const unsigned char* octalCode) {
    if (!octalCode) {
        qDebug("NULL");
    } else {
        QDebug continuedDebug = qDebug().nospace();
        for (size_t i = 0; i < bytesRequiredForCodeLength(numberOfThreeBitSectionsInCode(octalCode)); i++) {
            outputBits(octalCode[i], &continuedDebug);
        }
    }
}

char sectionValue(const unsigned char* startByte, char startIndexInByte) {
    char rightShift = 8 - startIndexInByte - 3;

    if (rightShift < 0) {
        return ((startByte[0] << -rightShift) & 7) + (startByte[1] >> (8 + rightShift));
    } else {
        return (startByte[0] >> rightShift) & 7;
    }
}

size_t bytesRequiredForCodeLength(unsigned char threeBitCodes) {
    if (threeBitCodes == 0) {
        return 1;
    } else {
        return 1 + ceilf((threeBitCodes * 3) / 8.0f);
    }
}

int branchIndexWithDescendant(const unsigned char* ancestorOctalCode, const unsigned char* descendantOctalCode) {
    int parentSections = numberOfThreeBitSectionsInCode(ancestorOctalCode);

    int branchStartBit = parentSections * 3;
    // Note: this does not appear to be "multi-byte length code" safe. When octal codes are larger than 255 bytes
    // long, the length code is stored in two bytes. The "1" below appears to assume that the length is always one
    // byte long.
    return sectionValue(descendantOctalCode + 1 + (branchStartBit / 8), branchStartBit % 8);
}

unsigned char* childOctalCode(const unsigned char* parentOctalCode, char childNumber) {

    // find the length (in number of three bit code sequences)
    // in the parent
    int parentCodeSections = parentOctalCode
        ? numberOfThreeBitSectionsInCode(parentOctalCode)
        : 0;

    // get the number of bytes used by the parent octal code
    size_t parentCodeBytes = bytesRequiredForCodeLength(parentCodeSections);

    // child code will have one more section than the parent
    size_t childCodeBytes = bytesRequiredForCodeLength(parentCodeSections + 1);

    // create a new buffer to hold the new octal code
    unsigned char* newCode = new unsigned char[childCodeBytes];

    // copy the parent code to the child
    if (parentOctalCode) {
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

void voxelDetailsForCode(const unsigned char* octalCode, VoxelPositionSize& voxelPositionSize) {
    float output[3];
    memset(&output[0], 0, 3 * sizeof(float));
    float currentScale = 1.0f;

    if (octalCode) {
        for (int i = 0; i < numberOfThreeBitSectionsInCode(octalCode); i++) {
            currentScale *= 0.5f;
            int sectionIndex = sectionValue(octalCode + 1 + (BITS_IN_OCTAL * i / BITS_IN_BYTE),
                                            (BITS_IN_OCTAL * i) % BITS_IN_BYTE);

            for (int j = 0; j < BITS_IN_OCTAL; j++) {
                output[j] += currentScale * (float)oneAtBit(sectionIndex, (BITS_IN_BYTE - BITS_IN_OCTAL) + j);
            }
        }
    }
    voxelPositionSize.x = output[0];
    voxelPositionSize.y = output[1];
    voxelPositionSize.z = output[2];
    voxelPositionSize.s = currentScale;
}

void copyFirstVertexForCode(const unsigned char* octalCode, float* output) {
    memset(output, 0, 3 * sizeof(float));

    float currentScale = 0.5f;

    for (int i = 0; i < numberOfThreeBitSectionsInCode(octalCode); i++) {
        int sectionIndex = sectionValue(octalCode + 1 + (3 * i / 8), (3 * i) % 8);

        for (int j = 0; j < 3; j++) {
            output[j] += currentScale * (int)oneAtBit(sectionIndex, 5 + j);
        }

        currentScale *= 0.5f;
    }
}

OctalCodeComparison compareOctalCodes(const unsigned char* codeA, const unsigned char* codeB) {
    if (!codeA || !codeB) {
        return ILLEGAL_CODE;
    }

    OctalCodeComparison result = LESS_THAN; // assume it's shallower

    size_t numberOfBytes = std::min(bytesRequiredForCodeLength(*codeA), bytesRequiredForCodeLength(*codeB));
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


char getOctalCodeSectionValue(const unsigned char* octalCode, int section) {
    int startAtByte = 1 + (BITS_IN_OCTAL * section / BITS_IN_BYTE);
    char startIndexInByte = (BITS_IN_OCTAL * section) % BITS_IN_BYTE;
    const unsigned char* startByte = octalCode + startAtByte;

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

unsigned char* chopOctalCode(const unsigned char* originalOctalCode, int chopLevels) {
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

bool isAncestorOf(const unsigned char* possibleAncestor, const unsigned char* possibleDescendent, int descendentsChild) {
    if (!possibleAncestor || !possibleDescendent) {
        return false;
    }

    int ancestorCodeLength = numberOfThreeBitSectionsInCode(possibleAncestor);
    if (ancestorCodeLength == 0) {
        return true; // this is the root, it's the anscestor of all
    }

    int descendentCodeLength = numberOfThreeBitSectionsInCode(possibleDescendent);

    // if the caller also include a child, then our descendent length is actually one extra!
    if (descendentsChild != CHECK_NODE_ONLY) {
        descendentCodeLength++;
    }

    if (ancestorCodeLength > descendentCodeLength) {
        return false; // if the descendent is shorter, it can't be a descendent
    }

    // compare the sections for the ancestor to the descendent
    for (int section = 0; section < ancestorCodeLength; section++) {
        char sectionValueAncestor = getOctalCodeSectionValue(possibleAncestor, section);
        char sectionValueDescendent;
        if (ancestorCodeLength <= descendentCodeLength) {
            sectionValueDescendent = getOctalCodeSectionValue(possibleDescendent, section);
        } else {
            assert(descendentsChild != CHECK_NODE_ONLY);
            sectionValueDescendent = descendentsChild;
        }
        if (sectionValueAncestor != sectionValueDescendent) {
            return false; // first non-match, means they don't match
        }
    }

    // they all match, so we are an ancestor
    return true;
}

OctalCodePtr createOctalCodePtr(size_t size) {
    return OctalCodePtr(new unsigned char[size], std::default_delete<unsigned char[]>());
}

OctalCodePtr hexStringToOctalCode(const QString& input) {
    const int HEX_NUMBER_BASE = 16;
    const int HEX_BYTE_SIZE = 2;
    int stringIndex = 0;
    int byteArrayIndex = 0;

    // allocate byte array based on half of string length
    auto bytes = createOctalCodePtr(input.length() / HEX_BYTE_SIZE);

    // loop through the string - 2 bytes at a time converting
    //  it to decimal equivalent and store in byte array
    bool ok = false;
    while (stringIndex < input.length()) {
        uint value = input.mid(stringIndex, HEX_BYTE_SIZE).toUInt(&ok, HEX_NUMBER_BASE);
        if (!ok) {
            break;
        }
        bytes.get()[byteArrayIndex] = (unsigned char)value;
        stringIndex += HEX_BYTE_SIZE;
        byteArrayIndex++;
    }

    // something went wrong
    if (!ok) {
        return nullptr;
    }
    return bytes;
}

QString octalCodeToHexString(const unsigned char* octalCode) {
    const int HEX_NUMBER_BASE = 16;
    const int HEX_BYTE_SIZE = 2;
    QString output;
    if (!octalCode) {
        output = "00";
    } else {
        for (size_t i = 0; i < bytesRequiredForCodeLength(*octalCode); i++) {
            output.append(QString("%1").arg(octalCode[i], HEX_BYTE_SIZE, HEX_NUMBER_BASE, QChar('0')).toUpper());
        }
    }
    return output;
}

