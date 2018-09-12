// <!
//  Created by Bradley Austin Davis on 2018/05/25
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
// !>

// <@if not PROCEDURAL_SHADER_CONSTANTS_H@>
// <@def PROCEDURAL_SHADER_CONSTANTS_H@>

// Hack comment to absorb the extra '//' scribe prepends

#ifndef PROCEDURAL_SHADER_CONSTANTS_H
#define PROCEDURAL_SHADER_CONSTANTS_H

#define PROCEDURAL_UNIFORM_TIME 200
#define PROCEDURAL_UNIFORM_DATE 201
#define PROCEDURAL_UNIFORM_FRAME_COUNT 202
#define PROCEDURAL_UNIFORM_POSITION 203
#define PROCEDURAL_UNIFORM_SCALE 204
#define PROCEDURAL_UNIFORM_ORIENTATION 205
// Additional space because orientation will take up 3-4 locations, being a matrix
#define PROCEDURAL_UNIFORM_CHANNEL_RESOLUTION 209
#define PROCEDURAL_UNIFORM_CUSTOM 220

#define PROCEDURAL_TEXTURE_CHANNEL0 0
#define PROCEDURAL_TEXTURE_CHANNEL1 1
#define PROCEDURAL_TEXTURE_CHANNEL2 2
#define PROCEDURAL_TEXTURE_CHANNEL3 3

// <!

namespace procedural { namespace slot {

namespace uniform {
enum Uniform {
    Time = PROCEDURAL_UNIFORM_TIME,
    Date = PROCEDURAL_UNIFORM_DATE,
    FrameCount = PROCEDURAL_UNIFORM_FRAME_COUNT,
    Position = PROCEDURAL_UNIFORM_POSITION,
    Scale = PROCEDURAL_UNIFORM_SCALE,
    Orientation = PROCEDURAL_UNIFORM_ORIENTATION,
    ChannelResolution = PROCEDURAL_UNIFORM_CHANNEL_RESOLUTION,
    Custom = PROCEDURAL_UNIFORM_CUSTOM,
};
}

namespace texture {
enum Texture {
    Channel0 = PROCEDURAL_TEXTURE_CHANNEL0,
    Channel1 = PROCEDURAL_TEXTURE_CHANNEL1,
    Channel2 = PROCEDURAL_TEXTURE_CHANNEL2,
    Channel3 = PROCEDURAL_TEXTURE_CHANNEL3,
};
} // namespace texture

} } // namespace procedural::slot

// !>
// Hack Comment

#endif // PROCEDURAL_SHADER_CONSTANTS_H

// <@if 1@>
// Trigger Scribe include
// <@endif@> <!def that !>

// <@endif@>

// Hack Comment
