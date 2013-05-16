//
//  Oscilloscope.h
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Oscilloscope__
#define __interface__Oscilloscope__

#include <cassert>

class Oscilloscope {
public:
    static unsigned const MAX_CHANNELS = 3;
    static unsigned const MAX_SAMPLES = 4096; // per channel

    Oscilloscope(int width, int height, bool isEnabled);
    ~Oscilloscope();

    volatile bool enabled;
    volatile bool inputPaused;

    void addSamples(unsigned ch, short const* data, unsigned n);
    
    void render(int x, int y);

    void setLowpass(float w) { assert(w > 0.0f && w <= 1.0f); _valLowpass = w; }
    void setDownsampling(unsigned f) { assert(f > 0); _valDownsample = f; }
    
private:
    // don't copy/assign
    Oscilloscope(Oscilloscope const&); // = delete;
    Oscilloscope& operator=(Oscilloscope const&); // = delete;    

    // implementation
    inline short* bufferBase(int i, int channel);

    unsigned        _valWidth;
    unsigned        _valHeight;
    short*          _arrSamples;
    short*          _arrVertices;
    unsigned        _arrWritePos[MAX_CHANNELS];

    float           _valLowpass;
    unsigned        _valDownsample;
};

#endif /* defined(__interface__oscilloscope__) */
