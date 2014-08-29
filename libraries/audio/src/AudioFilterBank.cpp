//
//  AudioFilterBank.cpp
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/10/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>
#include <SharedUtil.h>
#include "AudioRingBuffer.h"
#include "AudioFilter.h"
#include "AudioFilterBank.h"

template<>
AudioFilterLSF1s::FilterParameter 
AudioFilterLSF1s::_profiles[ AudioFilterLSF1s::_profileCount ][ AudioFilterLSF1s::_filterCount ] = {
    //  Freq        Gain    Slope
    { { 1000.0f,    1.0f,   1.0f }      }      // flat response (default)
};

template<>
AudioFilterHSF1s::FilterParameter 
AudioFilterHSF1s::_profiles[ AudioFilterHSF1s::_profileCount ][ AudioFilterHSF1s::_filterCount ] = {
    //  Freq        Gain    Slope
    { { 1000.0f,    1.0f,   1.0f }      }      // flat response (default)
};

template<>
AudioFilterPEQ1s::FilterParameter 
AudioFilterPEQ1s::_profiles[ AudioFilterPEQ1s::_profileCount ][ AudioFilterPEQ1s::_filterCount ] = {
    //  Freq        Gain    Q
    { { 1000.0f,    1.0f,   1.0f }      }      // flat response (default)
};

template<>
AudioFilterPEQ3m::FilterParameter 
AudioFilterPEQ3m::_profiles[ AudioFilterPEQ3m::_profileCount ][ AudioFilterPEQ3m::_filterCount ] = {

    //  Freq    Gain  Q               Freq     Gain  Q              Freq     Gain   Q
    { { 300.0f, 1.0f, 1.0f },       { 1000.0f, 1.0f, 1.0f },      { 4000.0f, 1.0f,  1.0f }      },  // flat response (default)
    { { 300.0f, 1.0f, 1.0f },       { 1000.0f, 1.0f, 1.0f },      { 4000.0f, 0.1f,  1.0f }      },  // treble cut
    { { 300.0f, 0.1f, 1.0f },       { 1000.0f, 1.0f, 1.0f },      { 4000.0f, 1.0f,  1.0f }      },  // bass cut
    { { 300.0f, 1.5f, 0.71f },      { 1000.0f, 0.5f, 1.0f },      { 4000.0f, 1.50f, 0.71f }     }   // smiley curve
};
