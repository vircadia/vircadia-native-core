// <!
//  Created by Bradley Austin Davis on 2018/05/25
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
// !>

// <@if not ENTITIES_SHADER_CONSTANTS_H@>
// <@def ENTITIES_SHADER_CONSTANTS_H@>

// Hack comment to absorb the extra '//' scribe prepends

#ifndef ENTITIES_SHADER_CONSTANTS_H
#define ENTITIES_SHADER_CONSTANTS_H

// Polyvox
#define ENTITIES_TEXTURE_POLYVOX_XMAP 0
#define ENTITIES_TEXTURE_POLYVOX_YMAP 1
#define ENTITIES_TEXTURE_POLYVOX_ZMAP 2



// <!

namespace entities_renderer { namespace slot {

namespace texture {
enum Texture {
    PolyvoxXMap = ENTITIES_TEXTURE_POLYVOX_XMAP,
    PolyvoxYMap = ENTITIES_TEXTURE_POLYVOX_YMAP,
    PolyvoxZMap = ENTITIES_TEXTURE_POLYVOX_ZMAP,
};
} // namespace texture

} } // namespace entities::slot

// !>
// Hack Comment

#endif // ENTITIES_SHADER_CONSTANTS_H

// <@if 1@>
// Trigger Scribe include
// <@endif@> <!def that !>

// <@endif@>

// Hack Comment
