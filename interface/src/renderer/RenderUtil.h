//
//  RenderUtil.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderUtil_h
#define hifi_RenderUtil_h

/// Renders a quad from (-1, -1, 0) to (1, 1, 0) with texture coordinates from (sMin, 0) to (sMax, 1).
void renderFullscreenQuad(float sMin = 0.0f, float sMax = 1.0f);

#endif // hifi_RenderUtil_h
