//
//  SeqNum.cpp
//
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SeqNum.h"

int udt::seqlen(const SeqNum& seq1, const SeqNum& seq2) {
    return (seq1._value <= seq2._value) ? (seq2._value - seq1._value + 1)
                                        : (seq2._value - seq1._value + SeqNum::MAX + 2);
}

int udt::seqoff(const SeqNum& seq1, const SeqNum& seq2) {
    if (glm::abs(seq1._value - seq2._value) < SeqNum::THRESHOLD) {
        return seq2._value - seq1._value;
    }
    
    if (seq1._value < seq2._value) {
        return seq2._value - seq1._value - SeqNum::MAX - 1;
    }
    
    return seq2._value - seq1._value + SeqNum::MAX + 1;
}
