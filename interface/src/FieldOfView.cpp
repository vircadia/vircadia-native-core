//
// FieldOfView.cpp
// interface
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "FieldOfView.h"

#include <math.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using namespace glm;

FieldOfView::FieldOfView() :
    _matOrientation(mat4(1.0f)),
    _vecBoundsLow(vec3(-1.0f,-1.0f,-1.0f)),
    _vecBoundsHigh(vec3(1.0f,1.0f,1.0f)),
    _valWidth(256.0f),
    _valHeight(256.0f),
    _valAngle(0.61),
    _valZoom(1.0f), 
    _enmAspectBalancing(expose_less) {
}

mat4 FieldOfView::getViewerScreenXform() const {

    mat4 projection;
    vec3 low, high;
    getFrustum(low, high);

    // perspective projection? determine correct near distance
    if (_valAngle != 0.0f) {

        projection = translate(
                frustum(low.x, high.x, low.y, high.y, low.z, high.z),
                vec3(0.f, 0.f, -low.z) );
    } else {

        projection = ortho(low.x, high.x, low.y, high.y, low.z, high.z);
    }

    return projection;
}

mat4 FieldOfView::getWorldViewerXform() const {

    return translate(affineInverse(_matOrientation),
            vec3(0.0f, 0.0f, -_vecBoundsHigh.z) );
}

mat4 FieldOfView::getWorldScreenXform() const {

    return translate(
            getViewerScreenXform() * affineInverse(_matOrientation),
            vec3(0.0f, 0.0f, -_vecBoundsHigh.z) );
}

mat4 FieldOfView::getViewerWorldXform() const {

    vec3 n_translate = vec3(0.0f, 0.0f, _vecBoundsHigh.z);

    return translate(
            translate(mat4(1.0f), n_translate) 
                * _matOrientation, -n_translate );
}

float FieldOfView::getPixelSize() const {

    vec3 low, high;
    getFrustum(low, high);

    return std::min(
            abs(high.x - low.x) / _valWidth,
            abs(high.y - low.y) / _valHeight);
}

void FieldOfView::getFrustum(vec3& low, vec3& high) const {

    low = _vecBoundsLow;
    high = _vecBoundsHigh;

    // start with uniform zoom
    float inv_zoom = 1.0f / _valZoom;
    float adj_x = inv_zoom, adj_y = inv_zoom;

    // balance aspect
    if (_enmAspectBalancing != stretch) {

        float f_aspect = (high.x - low.x) / (high.y - low.y);
        float vp_aspect = _valWidth / _valHeight;

        if ((_enmAspectBalancing == expose_more)
                != (f_aspect > vp_aspect)) {

            // expose_more -> f_aspect <= vp_aspect <=> adj >= 1
            // expose_less -> f_aspect  > vp_aspect <=> adj <  1
            adj_x = vp_aspect / f_aspect;

        } else {

            // expose_more -> f_aspect  > vp_aspect <=> adj >  1
            // expose_less -> f_aspect <= vp_aspect <=> adj <= 1
            adj_y = f_aspect / vp_aspect;
        }
    }

    // scale according to zoom / aspect correction
    float ax = (low.x + high.x) / 2.0f, ay = (low.y + high.y) / 2.0f;
    low.x = (low.x - ax) * adj_x + ax;
    high.x = (high.x - ax) * adj_x + ax;
    low.y = (low.y - ay) * adj_y + ay;
    high.y = (high.y - ay) * adj_y + ay;
    low.z = (low.z - high.z) * inv_zoom + high.z;

    // calc and apply near distance based on near diagonal and perspective
    float w = high.x - low.x, h = high.y - low.y;
    high.z -= low.z;
    low.z = _valAngle == 0.0f ? 0.0f :
            sqrt(w*w+h*h) * 0.5f / tan(_valAngle * 0.5f);
    high.z += low.z;
}

