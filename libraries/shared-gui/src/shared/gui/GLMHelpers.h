//
//  GLMHelpers.h
//  libraries/shared-gui/src/shared/gui
//
//  Created by Stephen Birarda on 2014-08-07.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_shared_gui_GLMHelpers_h
#define hifi_shared_gui_GLMHelpers_h

#include <GLMHelpers.h> // from shared library
#include <QtGui/QMatrix4x4>
#include <QtGui/QColor>

vec4 toGlm(const QColor& color);
QMatrix4x4 fromGlm(const glm::mat4 & m);

#endif // hifi_shared_gui_GLMHelpers_h
