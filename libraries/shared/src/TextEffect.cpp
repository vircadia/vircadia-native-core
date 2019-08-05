//
//  Created by Sam Gondelman on 7/21/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextEffect.h"

const char* textEffectNames[] = {
    "none",
    "outline",
    "outline fill",
    "shadow"
};

static const size_t TEXT_EFFECT_NAMES = (sizeof(textEffectNames) / sizeof(textEffectNames[0]));

QString TextEffectHelpers::getNameForTextEffect(TextEffect effect) {
    if (((int)effect <= 0) || ((int)effect >= (int)TEXT_EFFECT_NAMES)) {
        effect = (TextEffect)0;
    }

    return textEffectNames[(int)effect];
}