//
//  BoxBase.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple axis aligned box class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BoxBase_h
#define hifi_BoxBase_h

#include <glm/glm.hpp>
#include <QString>

/*@jsdoc
* <p>A <code>BoxFace</code> specifies the face of an axis-aligned (AA) box.
* <table>
*   <thead>
*     <tr><th>Value</th><th>Description</th></tr>
*   </thead>
*   <tbody>
*     <tr><td><code>"MIN_X_FACE"</code></td><td>The minimum x-axis face.</td></tr>
*     <tr><td><code>"MAX_X_FACE"</code></td><td>The maximum x-axis face.</td></tr>
*     <tr><td><code>"MIN_Y_FACE"</code></td><td>The minimum y-axis face.</td></tr>
*     <tr><td><code>"MAX_Y_FACE"</code></td><td>The maximum y-axis face.</td></tr>
*     <tr><td><code>"MIN_Z_FACE"</code></td><td>The minimum z-axis face.</td></tr>
*     <tr><td><code>"MAX_Z_FACE"</code></td><td>The maximum z-axis face.</td></tr>
*     <tr><td><code>"UNKNOWN_FACE"</code></td><td>Unknown value.</td></tr>
*   </tbody>
* </table>
* @typedef {string} BoxFace
*/
enum BoxFace {
    MIN_X_FACE,
    MAX_X_FACE,
    MIN_Y_FACE,
    MAX_Y_FACE,
    MIN_Z_FACE,
    MAX_Z_FACE,
    UNKNOWN_FACE
};

QString boxFaceToString(BoxFace face);
BoxFace boxFaceFromString(const QString& face);

enum BoxVertex {
    BOTTOM_LEFT_NEAR   = 0,
    BOTTOM_RIGHT_NEAR  = 1,
    TOP_RIGHT_NEAR     = 2,
    TOP_LEFT_NEAR      = 3,
    BOTTOM_LEFT_FAR    = 4,
    BOTTOM_RIGHT_FAR   = 5,
    TOP_RIGHT_FAR      = 6,
    TOP_LEFT_FAR       = 7
};

const int FACE_COUNT = 6;

#endif // hifi_BoxBase_h
