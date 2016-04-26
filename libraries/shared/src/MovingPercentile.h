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

    void updatePercentile(qint64 sample);
    qint64 getValueAtPercentile() const { return _valueAtPercentile; }

private:
    const int _numSamples;
    const float _percentile;

    QList<qint64> _samplesSorted;
    QList<int> _sampleIds;      // incrementally assigned, is cyclic
    int _newSampleId;

    int _indexOfPercentile;
    qint64 _valueAtPercentile;
};

#endif
