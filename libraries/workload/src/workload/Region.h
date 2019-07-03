//
//  Region.h
//  libraries/workload/src/workload
//
//  Created by Sam Gateau 2018.03.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_workload_Region_h
#define hifi_workload_Region_h

namespace workload {

class Region {
public:
    using Type = uint8_t;

    enum Name : uint8_t {
        R1 = 0,
        R2,
        R3,
        R4,
        UNKNOWN,
        INVALID,
    };

    static const uint8_t NUM_CLASSIFICATIONS = 4;
    static const uint8_t NUM_TRANSITIONS = NUM_CLASSIFICATIONS * (NUM_CLASSIFICATIONS - 1);

    static const uint8_t NUM_VIEW_REGIONS = (NUM_CLASSIFICATIONS - 1);

    static uint8_t computeTransitionIndex(uint8_t prevIndex, uint8_t newIndex);

};

inline uint8_t Region::computeTransitionIndex(uint8_t prevIndex, uint8_t newIndex) {
    // given prevIndex and newIndex compute an index into the transition list,
    // where the lists between unchanged indices don't exist (signaled by index = -1).
    //
    // Given an NxN array
    // let p = i + N * j
    //
    // then k = -1                   when i == j
    //        = p - (1 + p/(N+1))    when i != j
    //
    //    i   0       1       2       3
    // j  +-------+-------+-------+-------+
    //    |p = 0  |    1  |    2  |    3  |
    // 0  |       |       |       |       |
    //    |k = -1 |    0  |    1  |    2  |
    //    +-------+-------+-------+-------+
    //    |     4 |    5  |    6  |    7  |
    // 1  |       |       |       |       |
    //    |     3 |    -1 |    4  |    5  |
    //    +-------+-------+-------+-------+
    //    |     8 |     9 |   10  |   11  |
    // 2  |       |       |       |       |
    //    |     6 |     7 |    -1 |    8  |
    //    +-------+-------+-------+-------+
    //    |    12 |    13 |    14 |    15 |
    // 3  |       |       |       |       |
    //    |     9 |    10 |    11 |    -1 |
    //    +-------+-------+-------+-------+
    uint8_t p = prevIndex + Region::NUM_CLASSIFICATIONS * newIndex;
    if (0 == (p % (Region::NUM_CLASSIFICATIONS + 1))) {
        return -1;
    }
    return p - (1 + p / (Region::NUM_CLASSIFICATIONS + 1));
}

} // namespace workload

#endif // hifi_workload_Region_h
