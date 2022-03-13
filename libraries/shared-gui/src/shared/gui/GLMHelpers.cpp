//
//  GLMHelpers.cpp
//  libraries/shared-gui/src/shared/gui
//
//  Created by Stephen Birarda on 2014-08-07.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLMHelpers.h"


glm::vec4 toGlm(const QColor& color) {
    return glm::vec4(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

QMatrix4x4 fromGlm(const glm::mat4 & m) {
  return QMatrix4x4(&m[0][0]).transposed();
}
