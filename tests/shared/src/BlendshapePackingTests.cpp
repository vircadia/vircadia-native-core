//
//  BlendshapePackingTests.cpp
//  tests/shared/src
//
//  Created by Ken Cooke on 6/24/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BlendshapePackingTests.h"

#include <vector>

#include <test-utils/QTestExtensions.h>

#include <GLMHelpers.h>
#include <glm/gtc/random.hpp>

struct BlendshapeOffsetUnpacked {
    glm::vec3 positionOffset;
    glm::vec3 normalOffset;
    glm::vec3 tangentOffset;
};

struct BlendshapeOffsetPacked {
    glm::uvec4 packedPosNorTan;
};

QTEST_MAIN(BlendshapePackingTests)

static void packBlendshapeOffsetTo_Pos_F32_3xSN10_Nor_3xSN10_Tan_3xSN10(glm::uvec4& packed, const BlendshapeOffsetUnpacked& unpacked) {
    float len = glm::compMax(glm::abs(unpacked.positionOffset));
    glm::vec3 normalizedPos(unpacked.positionOffset);
    if (len > 0.0f) {
        normalizedPos /= len;
    } else {
        len = 1.0f;
    }

    packed = glm::uvec4(
        glm::floatBitsToUint(len),
        glm_packSnorm3x10_1x2(glm::vec4(normalizedPos, 0.0f)),
        glm_packSnorm3x10_1x2(glm::vec4(unpacked.normalOffset, 0.0f)),
        glm_packSnorm3x10_1x2(glm::vec4(unpacked.tangentOffset, 0.0f))
    );
}

static void packBlendshapeOffsets_ref(BlendshapeOffsetUnpacked* unpacked, BlendshapeOffsetPacked* packed, int size) {
    for (int i = 0; i < size; ++i) {
        packBlendshapeOffsetTo_Pos_F32_3xSN10_Nor_3xSN10_Tan_3xSN10((*packed).packedPosNorTan, (*unpacked));
        ++unpacked;
        ++packed;
    }
}

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
//
// Runtime CPU dispatch
//
#include <CPUDetect.h>

void packBlendshapeOffsets_AVX2(float (*unpacked)[9], uint32_t (*packed)[4], int size);

static void packBlendshapeOffsets(BlendshapeOffsetUnpacked* unpacked, BlendshapeOffsetPacked* packed, int size) {
    static bool _cpuSupportsAVX2 = cpuSupportsAVX2();
    if (_cpuSupportsAVX2) {
        static_assert(sizeof(BlendshapeOffsetUnpacked) == 9 * sizeof(float), "struct BlendshapeOffsetUnpacked size doesn't match.");
        static_assert(sizeof(BlendshapeOffsetPacked) == 4 * sizeof(uint32_t), "struct BlendshapeOffsetPacked size doesn't match.");
        packBlendshapeOffsets_AVX2((float(*)[9])unpacked, (uint32_t(*)[4])packed, size);
    } else {
        packBlendshapeOffsets_ref(unpacked, packed, size);
    }
}

#else   // portable reference code
static auto& packBlendshapeOffsets = packBlendshapeOffsets_ref;
#endif

void comparePacked(BlendshapeOffsetPacked& ref, BlendshapeOffsetPacked& tst) {
    union i10i10i10i2 {
        struct {
            int x : 10;
            int y : 10;
            int z : 10;
            int w : 2;
        } data;
        uint32_t pack;
    } Ref[4], Tst[4];

    for (int i = 0; i < 4; i++) {
        Ref[i].pack = ref.packedPosNorTan[i];
        Tst[i].pack = tst.packedPosNorTan[i];
    }

    // allow 1 ULP due to rounding differences
    QCOMPARE_WITH_ABS_ERROR(Tst[0].pack, Ref[0].pack, 1);

    QCOMPARE_WITH_ABS_ERROR(Tst[1].data.x, Ref[1].data.x, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[1].data.y, Ref[1].data.y, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[1].data.z, Ref[1].data.z, 1);

    QCOMPARE_WITH_ABS_ERROR(Tst[2].data.x, Ref[2].data.x, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[2].data.y, Ref[2].data.y, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[2].data.z, Ref[2].data.z, 1);

    QCOMPARE_WITH_ABS_ERROR(Tst[3].data.x, Ref[3].data.x, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[3].data.y, Ref[3].data.y, 1);
    QCOMPARE_WITH_ABS_ERROR(Tst[3].data.z, Ref[3].data.z, 1);
}

void BlendshapePackingTests::testAVX2() {

    for (int numBlendshapeOffsets = 0; numBlendshapeOffsets < 4096; ++numBlendshapeOffsets) {

        std::vector<BlendshapeOffsetUnpacked> unpackedBlendshapeOffsets(numBlendshapeOffsets);
        std::vector<BlendshapeOffsetPacked> packedBlendshapeOffsets1(numBlendshapeOffsets);
        std::vector<BlendshapeOffsetPacked> packedBlendshapeOffsets2(numBlendshapeOffsets);

        // init test data
        if (numBlendshapeOffsets > 0) {
            unpackedBlendshapeOffsets[0] = { 
                glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
            };
        }
        for (int i = 1; i < numBlendshapeOffsets; ++i) {
            unpackedBlendshapeOffsets[i] = {
                glm::linearRand(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f)),
                glm::linearRand(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f)),
                glm::linearRand(glm::vec3(-2.0f, -2.0f, -2.0f), glm::vec3(2.0f, 2.0f, 2.0f)),
            };
        }

        // ref version
        packBlendshapeOffsets_ref(unpackedBlendshapeOffsets.data(), packedBlendshapeOffsets1.data(), numBlendshapeOffsets);

        // AVX2 version, if supported by CPU
        packBlendshapeOffsets(unpackedBlendshapeOffsets.data(), packedBlendshapeOffsets2.data(), numBlendshapeOffsets);

        // verify
        for (int i = 0; i < numBlendshapeOffsets; ++i) {
            auto ref = packedBlendshapeOffsets1.at(i);
            auto tst = packedBlendshapeOffsets2.at(i);
            comparePacked(ref, tst);
        }
    }
}
