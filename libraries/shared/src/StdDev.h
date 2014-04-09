//
//  StdDev.h
//  libraries/shared/src
//
//  Created by Philip Rosedale on 3/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__StdDev__
#define __hifi__StdDev__

class StDev {
    public:
        StDev();
        void reset();
        void addValue(float v);
        float getAverage();
        float getStDev();
        int getSamples() const { return sampleCount; }
    private:
        float * data;
        int sampleCount;
};

#endif /* defined(__hifi__StdDev__) */
