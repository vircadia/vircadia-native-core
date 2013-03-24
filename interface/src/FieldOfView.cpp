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

FieldOfView::FieldOfView()
    :   mat_orientation(mat4(1.0f)),
        vec_frustum_low(vec3(-1.0f,-1.0f,-1.0f)),
        vec_frustum_high(vec3(1.0f,1.0f,1.0f)),
        val_width(256.0f),
        val_height(256.0f),
        val_angle(0.61),
        val_zoom(1.0f), 
        enm_aspect_balancing(expose_less)
{
}

mat4 FieldOfView::getViewerScreenXform() const
{
    mat4 projection;
    vec3 low, high;
    calcGlFrustum(low, high);

    // perspective projection? determine correct near distance
    if (val_angle != 0.0f)
    {
        projection = translate(
                frustum(low.x, high.x, low.y, high.y, low.z, high.z),
                vec3(0.f, 0.f, -low.z) );
    }
    else
    {
        projection = ortho(low.x, high.x, low.y, high.y, low.z, high.z);
    }

    return projection;
}

mat4 FieldOfView::getWorldViewerXform() const
{
    return translate(affineInverse(mat_orientation),
            vec3(0.0f, 0.0f, -vec_frustum_high.z) );
}

mat4 FieldOfView::getWorldScreenXform() const
{
    return translate(
            getViewerScreenXform() * affineInverse(mat_orientation),
            vec3(0.0f, 0.0f, -vec_frustum_high.z) );
}

mat4 FieldOfView::getViewerWorldXform() const
{
    vec3 n_translate = vec3(0.0f, 0.0f, vec_frustum_high.z);

    return translate(
            translate(mat4(1.0f), n_translate) 
                * mat_orientation, -n_translate );
}

float FieldOfView::getPixelSize() const
{
    vec3 low, high;
    calcGlFrustum(low, high);

    return std::min(
            abs(high.x - low.x) / val_width,
            abs(high.y - low.y) / val_height);
}

void FieldOfView::calcGlFrustum(vec3& low, vec3& high) const
{
    low = vec_frustum_low;
    high = vec_frustum_high;

    // apply zoom
    float inv_zoom = 1.0f / val_zoom;
    float ax = (low.x + high.x) / 2.0f, ay = (low.y + high.y) / 2.0f;
    low.x = (low.x - ax) * inv_zoom + ax;
    high.x = (high.x - ax) * inv_zoom + ax;
    low.y = (low.y - ay) * inv_zoom + ay;
    high.y = (high.y - ay) * inv_zoom + ay;
    low.z = (low.z - high.z) * inv_zoom + high.z;

    // balance aspect
    if (enm_aspect_balancing != stretch)
    {
        float f_aspect = (high.x - low.x) / (high.y - low.y);
        float vp_aspect = val_width / val_height;

        float adj;
        if ((enm_aspect_balancing == expose_more)
                != (f_aspect > vp_aspect))
        {
            // expose_more -> f_aspect <= vp_aspect <=> adj >= 1
            // expose_less -> f_aspect  > vp_aspect <=> adj <  1
            adj = vp_aspect / f_aspect;
            low.x *= adj;
            high.x *= adj;
        }
        else
        {
            // expose_more -> f_aspect  > vp_aspect <=> adj >  1
            // expose_less -> f_aspect <= vp_aspect <=> adj <= 1
            adj = f_aspect / vp_aspect;
            low.y *= adj;
            high.y *= adj;
        }
    }

    float w = high.x - low.x, h = high.y - low.y;

    high.z -= low.z;
    low.z = val_angle == 0.0f ? 0.0f :
            sqrt(w*w+h*h) * 0.5f / tan(val_angle * 0.5f);
    high.z += low.z;
}

