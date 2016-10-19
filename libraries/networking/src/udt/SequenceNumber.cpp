//
//  SequenceNumber.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/23/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SequenceNumber.h"

#include <QtCore/QMetaType>

using namespace udt;

Q_DECLARE_METATYPE(SequenceNumber);

static const int sequenceNumberMetaTypeID = qRegisterMetaType<SequenceNumber>();

int udt::seqlen(const SequenceNumber& seq1, const SequenceNumber& seq2) {
    return (seq1._value <= seq2._value) ? (seq2._value - seq1._value + 1)
                                        : (seq2._value - seq1._value + SequenceNumber::MAX + 2);
}

int udt::seqoff(const SequenceNumber& seq1, const SequenceNumber& seq2) {
    if (glm::abs(seq1._value - seq2._value) < SequenceNumber::THRESHOLD) {
        return seq2._value - seq1._value;
    }
    
    if (seq1._value < seq2._value) {
        return seq2._value - seq1._value - SequenceNumber::MAX - 1;
    }
    
    return seq2._value - seq1._value + SequenceNumber::MAX + 1;
}
