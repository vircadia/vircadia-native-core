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
        R1 = 0,   // R1 = in physics simulation and client will bid for simulation ownership
        R2,       // R2 = in physics simulation but client prefers to NOT have simulation ownership
        R3,       // R3 = are NOT in physics simulation but yes kinematically animated when velocities are non-zero
        R4,       // R4 = known to workload but outside R3, not in physics, not animated if moving
        UNKNOWN,  // UNKNOWN = known to workload but unsorted
        INVALID,  // INVALID = not known to workload
    };

    static constexpr uint32_t NUM_KNOWN_REGIONS = uint32_t(Region::R4 - Region::R1 + 1); // R1 through R4 inclusive
    static constexpr uint32_t NUM_TRACKED_REGIONS = uint32_t(Region::R3 - Region::R1 + 1); // R1 through R3 inclusive
    static const uint8_t NUM_REGION_TRANSITIONS = NUM_KNOWN_REGIONS * (NUM_KNOWN_REGIONS - 1);

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
    uint8_t p = prevIndex + Region::NUM_KNOWN_REGIONS * newIndex;
    if (0 == (p % (Region::NUM_KNOWN_REGIONS + 1))) {
        return -1;
    }
    return p - (1 + p / (Region::NUM_KNOWN_REGIONS + 1));
}

} // namespace workload

#endif // hifi_workload_Region_h
