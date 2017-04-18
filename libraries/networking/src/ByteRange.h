//
//  ByteRange.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 4/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ByteRange_h
#define hifi_ByteRange_h

struct ByteRange {
    int64_t fromInclusive { 0 };
    int64_t toExclusive { 0 };

    bool isSet() { return fromInclusive < 0 || fromInclusive < toExclusive; }
    int64_t size() { return toExclusive - fromInclusive; }
};


#endif // hifi_ByteRange_h
