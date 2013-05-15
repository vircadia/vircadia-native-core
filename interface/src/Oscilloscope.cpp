//
//  Oscilloscope.cpp
//  interface
//
//  Created by Philip on 1/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Oscilloscope.h"

#include "InterfaceConfig.h"
#include <limits>
#include <cstring>
#include <algorithm>

//  Reimplemented 4/26/13 (tosh) - don't blame Philip for bugs

using namespace std;

namespace { // everything in here only exists while compiling this .cpp file

    // one sample buffer per channel
    unsigned const N_SAMPLES_ALLOC = Oscilloscope::MAX_SAMPLES * Oscilloscope::MAX_CHANNELS;

    // adding an x-coordinate yields twice the amount of vertices
    unsigned const MAX_COORDS = Oscilloscope::MAX_SAMPLES * 2; 
    unsigned const N_COORDS_ALLOC = MAX_COORDS * Oscilloscope::MAX_CHANNELS;
    
    unsigned const N_ALLOC_TOTAL = N_SAMPLES_ALLOC + N_COORDS_ALLOC;
}


Oscilloscope::Oscilloscope(int w, int h, bool isEnabled) : 
    _valWidth(w), _valHeight(h), 
    _arrSamples(0l), _arrVertices(0l), 
    _valLowpass(0.4f), _valDownsample(3),
    enabled(isEnabled), inputPaused(false) {
    
    // allocate enough space for the sample data and to turn it into
    // vertices and since they're all 'short', do so in one shot
    _arrSamples = new short[N_ALLOC_TOTAL];
    memset(_arrSamples, 0, N_ALLOC_TOTAL * sizeof(short));
    _arrVertices = _arrSamples + N_SAMPLES_ALLOC;

    // initialize write positions
    for (unsigned ch = 0; ch < MAX_CHANNELS; ++ch) {
        _arrWritePos[ch] = MAX_SAMPLES * ch;
    }
}

Oscilloscope::~Oscilloscope() {

    delete[] _arrSamples;
}

void Oscilloscope::addSamples(unsigned ch, short const* data, unsigned n) {

    if (! enabled || inputPaused) {
        return;
    }
    
    unsigned baseOffs = MAX_SAMPLES * ch;
    unsigned endOffs = baseOffs + MAX_SAMPLES;
        
    unsigned writePos = _arrWritePos[ch];
    unsigned newWritePos = writePos + n;

    unsigned n2 = 0;
    if (newWritePos >= endOffs) {
        n2 = newWritePos - endOffs;
        newWritePos = baseOffs + n2;
        n -= n2;
    }
        
    memcpy(_arrSamples + writePos, data, n * sizeof(short));
    if (n2 > 0) {
        memcpy(_arrSamples + baseOffs, data + n, n2 * sizeof(short));
    }
    
    _arrWritePos[ch] = newWritePos;
}
        
void Oscilloscope::render(int x, int y) {

    if (! enabled) {
        return;
    }
    
    // expand data to vertex data, now
    int lowpass = -int(std::numeric_limits<short>::min()) * _valLowpass;
    unsigned downsample = _valDownsample;
    // keep half of the buffer for writing and ensure an even vertex count
    unsigned usedWidth = min(_valWidth, MAX_SAMPLES / (downsample * 2)) & ~1u;
    unsigned usedSamples = usedWidth * downsample;
    
    for (unsigned ch = 0; ch < MAX_CHANNELS; ++ch) {
 
        short const* basePtr = _arrSamples + MAX_SAMPLES * ch;
        short const* endPtr = basePtr + MAX_SAMPLES;
        short const* inPtr = _arrSamples + _arrWritePos[ch];
        short* outPtr = _arrVertices + MAX_COORDS * ch;
        int sample = 0, x = usedWidth;
        for (int i = int(usedSamples); --i >= 0 ;) {

            if (inPtr == basePtr) {
                inPtr = endPtr;
            }
            sample += ((*--inPtr - sample) * lowpass) >> 15;                    
            if (i % downsample == 0) {
                *outPtr++ = short(--x);
                *outPtr++ = short(sample);
            }
        }
    } 
    
    glLineWidth(2.0);
    glPushMatrix();
    glTranslatef((float)x + 0.0f, (float)y + _valHeight / 2.0f, 0.0f);
    glScaled(1.0f, _valHeight / 32767.0f, 1.0f);
    glVertexPointer(2, GL_SHORT, 0, _arrVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColor3f(1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES * 0, usedWidth);
    glColor3f(0.0f, 1.0f ,1.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES * 1, usedWidth);
    glColor3f(0.0f, 1.0f ,1.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES * 2, usedWidth);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

