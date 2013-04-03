//
// starfield/data/BrightnessLevel.h
// interface
//
// Created by Tobias Schwinger on 3/29/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__starfield__data__BrightnessLevel__
#define __interface__starfield__data__BrightnessLevel__

#ifndef __interface__Starfield_impl__
#error "This is an implementation file - not intended for direct inclusion."
#endif

#include "starfield/Config.h"
#include "starfield/data/InputVertex.h"
#include "starfield/data/GpuVertex.h"

namespace starfield {

    typedef nuint BrightnessLevel;


#if STARFIELD_SAVE_MEMORY
    const unsigned BrightnessBits = 16u;
#else
    const unsigned BrightnessBits = 18u;
#endif
    const BrightnessLevel BrightnessMask = (1u << (BrightnessBits)) - 1u;
    
    typedef std::vector<BrightnessLevel> BrightnessLevels;

    BrightnessLevel getBrightness(unsigned c) {

        unsigned r = (c >> 16) & 0xff;
        unsigned g = (c >> 8) & 0xff;
        unsigned b = c & 0xff;
#if STARFIELD_SAVE_MEMORY
        return BrightnessLevel((r*r+g*g+b*b) >> 2);
#else
        return BrightnessLevel(r*r+g*g+b*b);
#endif
    }


    struct GreaterBrightness {

        bool operator()(InputVertex const& lhs, InputVertex const& rhs) const {
            return getBrightness(lhs.getColor())
                    > getBrightness(rhs.getColor());
        }
        bool operator()(BrightnessLevel lhs, GpuVertex const& rhs) const {
            return lhs > getBrightness(rhs.getColor());;
        }
        bool operator()(BrightnessLevel lhs, BrightnessLevel rhs) const {
            return lhs > rhs;
        }
    };

} // anonymous namespace

#endif

