//
// BitVectorHelperTests.cpp
// tests/shared/src
//
// Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BitVectorHelperTests.h"

#include <SharedLogging.h>

#include "../QTestExtensions.h"
#include <QtCore/QDebug>
#include <StreamUtils.h>
#include <glm/glm.hpp>
#include <BitVectorHelpers.h>

QTEST_MAIN(BitVectorHelperTests)

void BitVectorHelperTests::sizeTest() {
    std::vector<int> sizes = {0, 6, 7, 8, 30, 31, 32, 33, 87, 88, 89, 90, 90, 91, 92, 93};
    for (auto& size : sizes) {
        const int oldWay = (int)ceil((float)size / (float)BITS_IN_BYTE);
        const int newWay = (int)calcBitVectorSize(size);
        QCOMPARE(oldWay, newWay);
    }
}

static void readWriteHelper(const std::vector<bool>& src) {

    int numBits = (int)src.size();
    int numBytes = calcBitVectorSize(numBits);
    uint8_t* bytes = new uint8_t[numBytes];
    memset(bytes, numBytes, sizeof(uint8_t));
    int numBytesWritten = writeBitVector(bytes, numBits, [&](int i) {
        return src[i];
    });
    QCOMPARE(numBytesWritten, numBytes);

    std::vector<bool> dst;
    int numBytesRead = readBitVector(bytes, numBits, [&](int i, bool value) {
        dst.push_back(value);
    });
    QCOMPARE(numBytesRead, numBytes);

    QCOMPARE(numBits, (int)src.size());
    QCOMPARE(numBits, (int)dst.size());
    for (int i = 0; i < numBits; i++) {
        bool a = src[i];
        bool b = dst[i];
        QCOMPARE(a, b);
    }
}

void BitVectorHelperTests::readWriteTest() {
    std::vector<int> sizes = {0, 6, 7, 8, 30, 31, 32, 33, 87, 88, 89, 90, 90, 91, 92, 93};

    for (auto& size : sizes) {
        std::vector<bool> allTrue(size, true);
        std::vector<bool> allFalse(size, false);
        std::vector<bool> evenSet;
        evenSet.reserve(size);
        std::vector<bool> oddSet;
        oddSet.reserve(size);
        for (int i = 0; i < size; i++) {
            bool isOdd = (i & 0x1) > 0;
            evenSet.push_back(!isOdd);
            oddSet.push_back(isOdd);
        }
        readWriteHelper(allTrue);
        readWriteHelper(allFalse);
        readWriteHelper(evenSet);
        readWriteHelper(oddSet);
    }
}
