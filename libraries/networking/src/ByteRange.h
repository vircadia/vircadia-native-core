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

    bool isSet() const { return fromInclusive < 0 || fromInclusive < toExclusive; }
    int64_t size() const { return toExclusive - fromInclusive; }

    // byte ranges are invalid if:
    // (1) the toExclusive of the range is negative
    // (2) the toExclusive of the range is less than the fromInclusive, and isn't zero
    // (3) the fromExclusive of the range is negative, and the toExclusive isn't zero
    bool isValid() {
        return toExclusive >= 0
                && (toExclusive >= fromInclusive || toExclusive == 0)
                && (fromInclusive >= 0 || toExclusive == 0);
    }

    void fixupRange(int64_t fileSize) {
        if (!isSet()) {
            // if the byte range is not set, force it to be from 0 to the end of the file
            fromInclusive = 0;
            toExclusive = fileSize;
        }

        if (fromInclusive > 0 && toExclusive == 0) {
            // we have a left side of the range that is non-zero
            // if the RHS of the range is zero, set it to the end of the file now
            toExclusive = fileSize;
        } else if (-fromInclusive >= fileSize) {
            // we have a negative range that is equal or greater than the full size of the file
            // so we just set this to be a range across the entire file, from 0
            fromInclusive = 0;
            toExclusive = fileSize;
        }
    }
};


#endif // hifi_ByteRange_h
