//
//  Created by HifiExperiments on 2/9/21
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextAlignment.h"

const char* textAlignmentNames[] = {
    "left",
    "center",
    "right"
};

static const size_t TEXT_ALIGNMENT_NAMES = (sizeof(textAlignmentNames) / sizeof(textAlignmentNames[0]));

QString TextAlignmentHelpers::getNameForTextAlignment(TextAlignment alignment) {
    if (((int)alignment <= 0) || ((int)alignment >= (int)TEXT_ALIGNMENT_NAMES)) {
        alignment = (TextAlignment)0;
    }

    return textAlignmentNames[(int)alignment];
}