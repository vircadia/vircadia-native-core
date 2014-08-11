//
//  AudioFilter.cpp
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/10/14.
//
//

#include <math.h>
#include <vector>
#include <SharedUtil.h>
#include "AudioRingBuffer.h"
#include "AudioFilter.h"

template<>
AudioFilterPEQ3::FilterParameter AudioFilterPEQ3::_profiles[ AudioFilterPEQ3::_profileCount ][ AudioFilterPEQ3::_filterCount ] = {

    { { 300., 1., 1. },     { 1000., 1., 1. },      { 4000., 1., 1. }       },      // flat response (default)
    { { 300., 1., 1. },     { 1000., 1., 1. },      { 4000., .1, 1. }       },      // treble cut
    { { 300., .1, 1. },     { 1000., 1., 1. },      { 4000., 1., 1. }       },      // bass cut
    { { 300., 1.5, 0.71 },  { 1000., .5, 1. },      { 4000., 1.5, 0.71 }    }       // smiley curve
};

