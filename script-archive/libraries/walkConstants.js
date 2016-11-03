//
//  walkConstants.js
//  version 1.0
//
//  Created by David Wooldridge, June 2015
//  Copyright © 2015 High Fidelity, Inc.
//
//  Provides constants necessary for the operation of the walk.js script and the walkApi.js script
// 
//  Editing tools for animation data files available here: https://github.com/DaveDubUK/walkTools
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// locomotion states
STATIC = 1;
SURFACE_MOTION = 2;
AIR_MOTION = 4;

// directions
UP = 1;
DOWN = 2;
LEFT = 4;
RIGHT = 8;
FORWARDS = 16;
BACKWARDS = 32;
NONE = 64;

// waveshapes
SAWTOOTH = 1;
TRIANGLE = 2;
SQUARE = 4;

// used by walk.js and walkApi.js 
MAX_WALK_SPEED = 2.9; // peak, by observation
MAX_FT_WHEEL_INCREMENT = 25; // avoid fast walk when landing
TOP_SPEED = 300;
ON_SURFACE_THRESHOLD = 0.1; // height above surface to be considered as on the surface
TRANSITION_COMPLETE = 1000;
PITCH_MAX = 60; // maximum speed induced pitch
ROLL_MAX = 80;  // maximum speed induced leaning / banking
DELTA_YAW_MAX = 1.7; // maximum change in yaw in rad/s

// used by walkApi.js only
MOVE_THRESHOLD = 0.075; // movement dead zone
ACCELERATION_THRESHOLD = 0.2;  // detect stop to walking
DECELERATION_THRESHOLD = -6; // detect walking to stop
FAST_DECELERATION_THRESHOLD = -150; // detect flying to stop
BOUNCE_ACCELERATION_THRESHOLD = 25; // used to ignore gravity influence fluctuations after landing
GRAVITY_THRESHOLD = 3.0; // height above surface where gravity is in effect
OVERCOME_GRAVITY_SPEED = 0.5; // reaction sensitivity to jumping under gravity
LANDING_THRESHOLD = 0.35; // metres from a surface below which need to prepare for impact
MAX_TRANSITION_RECURSION = 10; // how many nested transitions are permitted
