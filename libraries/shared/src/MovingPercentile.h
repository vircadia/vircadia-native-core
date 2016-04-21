//
//  MovingPercentile.h
//  libraries/shared/src
//
//  Created by Yixin Wang on 6/4/2014
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MovingPercentile_h
#define hifi_MovingPercentile_h

#include <qlist.h>

class MovingPercentile {

public:
    MovingPercentile(int numSamples, float percentile = 0.5f);

    // FIXME - this class is only currently used in calculating the clockSkew, which is a signed 64bit int, not a double
    // I am somewhat tempted to make this a type sensitive template and/or swith to using qint64's internally
    void updatePercentile(quint64 sample);
    quint64 getValueAtPercentile() const { return _valueAtPercentile; }

private:
    const int _numSamples;
    const float _percentile;

    QList<quint64> _samplesSorted;
    QList<int> _sampleIds;      // incrementally assigned, is cyclic
    int _newSampleId;

    int _indexOfPercentile;
    quint64 _valueAtPercentile;
};

#endif
