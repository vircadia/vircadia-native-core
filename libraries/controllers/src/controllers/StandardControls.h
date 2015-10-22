//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

namespace controller {

    // Needs to match order and values of SDL_GameControllerButton
    enum StandardButtonChannel {
        // Button quad
        A = 0,
        B,
        X,
        Y,

        // Center buttons
        BACK,
        GUIDE,
        START,

        // Stick press
        LS,
        RS,

        // Bumper press
        LB,
        RB,

        // DPad
        DU,
        DD,
        DL,
        DR,

        // These don't map to SDL types
        LEFT_PRIMARY_THUMB,
        LEFT_SECONDARY_THUMB,
        RIGHT_PRIMARY_THUMB,
        RIGHT_SECONDARY_THUMB,

        LEFT_PRIMARY_INDEX,
        LEFT_SECONDARY_INDEX,
        RIGHT_PRIMARY_INDEX,
        RIGHT_SECONDARY_INDEX,

        LEFT_GRIP,
        RIGHT_GRIP,

        NUM_STANDARD_BUTTONS
    };

    // Needs to match order and values of SDL_GameControllerAxis
    enum StandardAxisChannel {
        // Left Analog stick
        LX = 0,
        LY,
        // Right Analog stick
        RX,
        RY,
        // Triggers
        LT,
        RT,
        NUM_STANDARD_AXES
    };

    // No correlation to SDL
    enum StandardPoseChannel {
        LEFT_HAND = 0,
        RIGHT_HAND,
        HEAD,
        NUM_STANDARD_POSES
    };

    enum StandardCounts {
        TRIGGERS = 2,
        ANALOG_STICKS = 2,
        POSES = 2, // FIXME 3?  if we want to expose the head?
    };
}
