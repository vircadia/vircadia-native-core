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
    unsigned const MAX_SAMPLES = Oscilloscope::MAX_SAMPLES_PER_CHANNEL * Oscilloscope::MAX_CHANNELS;

    // adding an x-coordinate yields twice the amount of vertices
    unsigned const MAX_COORDS_PER_CHANNEL = Oscilloscope::MAX_SAMPLES_PER_CHANNEL * 2;
    // allocated once for each channel 
    unsigned const MAX_COORDS = MAX_COORDS_PER_CHANNEL * Oscilloscope::MAX_CHANNELS;

    // total amount of memory to allocate (in 16-bit integers)
    unsigned const N_INT16_TO_ALLOC = MAX_SAMPLES + MAX_COORDS;
}


Oscilloscope::Oscilloscope(int w, int h, bool isEnabled) : 
    enabled(isEnabled),
    inputPaused(false),
    _width(w),
    _height(h), 
    _samples(0l),
    _vertices(0l),
    // some filtering (see details in Log.h)
    _lowPassCoeff(0.4f),
    // three in -> one out
    _downsampleRatio(3) {

    // allocate enough space for the sample data and to turn it into
    // vertices and since they're all 'short', do so in one shot
    _samples = new short[N_INT16_TO_ALLOC];
    memset(_samples, 0, N_INT16_TO_ALLOC * sizeof(short));
    _vertices = _samples + MAX_SAMPLES;

    // initialize write positions to start of each channel's region
    for (unsigned ch = 0; ch < MAX_CHANNELS; ++ch) {
        _writePos[ch] = MAX_SAMPLES_PER_CHANNEL * ch;
    }
}

Oscilloscope::~Oscilloscope() {

    delete[] _samples;
}

void Oscilloscope::addSamples(unsigned ch, short const* data, unsigned n) {

    if (! enabled || inputPaused) {
        return;
    }

    // determine start/end offset of this channel's region
    unsigned baseOffs = MAX_SAMPLES_PER_CHANNEL * ch;
    unsigned endOffs = baseOffs + MAX_SAMPLES_PER_CHANNEL;

    // fetch write position for this channel
    unsigned writePos = _writePos[ch];

    // determine write position after adding the samples 
    unsigned newWritePos = writePos + n;
    unsigned n2 = 0;
    if (newWritePos >= endOffs) {
        // passed boundary of the circular buffer? -> we need to copy two blocks
        n2 = newWritePos - endOffs;
        newWritePos = baseOffs + n2;
        n -= n2;
    }

    // copy data 
    memcpy(_samples + writePos, data, n * sizeof(short));
    if (n2 > 0) {
        memcpy(_samples + baseOffs, data + n, n2 * sizeof(short));
    }

    // set new write position for this channel 
    _writePos[ch] = newWritePos;
}
        
void Oscilloscope::render(int x, int y) {

    if (! enabled) {
        return;
    }

    // fetch low pass factor (and convert to fix point) / downsample factor
    int lowPassFixPt = -int(std::numeric_limits<short>::min()) * _lowPassCoeff;
    unsigned downsample = _downsampleRatio;
    // keep half of the buffer for writing and ensure an even vertex count
    unsigned usedWidth = min(_width, MAX_SAMPLES_PER_CHANNEL / (downsample * 2)) & ~1u;
    unsigned usedSamples = usedWidth * downsample;
    
    // expand samples to vertex data
    for (unsigned ch = 0; ch < MAX_CHANNELS; ++ch) {
        // for each channel: determine memory regions
        short const* basePtr = _samples + MAX_SAMPLES_PER_CHANNEL * ch;
        short const* endPtr = basePtr + MAX_SAMPLES_PER_CHANNEL;
        short const* inPtr = _samples + _writePos[ch];
        short* outPtr = _vertices + MAX_COORDS_PER_CHANNEL * ch;
        int sample = 0, x = usedWidth;
        for (int i = int(usedSamples); --i >= 0 ;) {
            if (inPtr == basePtr) {
                // handle boundary, reading the circular sample buffer
                inPtr = endPtr;
            }
            // read and (eventually) filter sample
            sample += ((*--inPtr - sample) * lowPassFixPt) >> 15;
            // write every nth as y with a corresponding x-coordinate 
            if (i % downsample == 0) {
                *outPtr++ = short(--x);
                *outPtr++ = short(sample);
            }
        }
    } 

    // set up rendering state (vertex data lives at _vertices)
    glLineWidth(1.0);
    glDisable(GL_LINE_SMOOTH);
    glPushMatrix();
    glTranslatef((float)x + 0.0f, (float)y + _height / 2.0f, 0.0f);
    glScaled(1.0f, _height / 32767.0f, 1.0f);
    glVertexPointer(2, GL_SHORT, 0, _vertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    // render channel 0
    glColor3f(1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES_PER_CHANNEL * 0, usedWidth);

    // render channel 1
    glColor3f(0.0f, 1.0f ,1.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES_PER_CHANNEL * 1, usedWidth);

    // render channel 2
    glColor3f(0.0f, 1.0f ,0.0f);
    glDrawArrays(GL_LINES, MAX_SAMPLES_PER_CHANNEL * 2, usedWidth);

    // reset rendering state
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
}

