//
//  SequenceNumberStats.h
//  libraries/networking/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SequenceNumberStats_h
#define hifi_SequenceNumberStats_h

#include "SharedUtil.h"

class SequenceNumberStats {
public:
    SequenceNumberStats();

    void sequenceNumberReceived(quint16 incoming, const bool wantExtraDebugging = false);

    quint32 getNumReceived() const { return _numReceived; }
    quint32 getNumUnreasonable() const { return _numUnreasonable; }
    quint32 getNumOutOfOrder() const { return _numEarly + _numLate; }
    quint32 getNumEarly() const { return _numEarly; }
    quint32 getNumLate() const { return _numLate; }
    quint32 getNumLost() const { return _numLost; }
    quint32 getNumRecovered() const { return _numRecovered; }
    quint32 getNumDuplicate() const { return _numDuplicate; }
    const QSet<quint16>& getMissingSet() const { return _missingSet; }

private:
    quint16 _lastReceived;
    QSet<quint16> _missingSet;

    quint32 _numReceived;
    quint32 _numUnreasonable;
    quint32 _numEarly;
    quint32 _numLate;
    quint32 _numLost;
    quint32 _numRecovered;
    quint32 _numDuplicate;
};

#endif // hifi_SequenceNumberStats_h
