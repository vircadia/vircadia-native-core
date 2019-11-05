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

#define PROCEDURAL_BUFFER_INPUTS 7

#define PROCEDURAL_UNIFORM_CUSTOM 220

#define PROCEDURAL_TEXTURE_CHANNEL0 2
#define PROCEDURAL_TEXTURE_CHANNEL1 3
#define PROCEDURAL_TEXTURE_CHANNEL2 4
#define PROCEDURAL_TEXTURE_CHANNEL3 5

// <!

namespace procedural { namespace slot {

namespace buffer {
enum Bufffer {
    Inputs = PROCEDURAL_BUFFER_INPUTS,
};
}

namespace uniform {
enum Uniform {
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
