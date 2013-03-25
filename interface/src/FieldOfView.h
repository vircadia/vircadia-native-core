//
// FieldOfView.h
// interface
//
// Created by Tobias Schwinger on 3/21/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__FieldOfView__
#define __interface__FieldOfView__

#include <glm/glm.hpp>

/**
 * Viewing parameter encapsulation.
 */
class FieldOfView
{
        glm::mat4   mat_orientation;
        glm::vec3   vec_bounds_low;
        glm::vec3   vec_bounds_high;
        float       val_width;
        float       val_height;
        float       val_angle;
        float       val_zoom;
        int         enm_aspect_balancing;
    public:

        FieldOfView();

        // mutators

        FieldOfView& setBounds(glm::vec3 const& low, glm::vec3 const& high)
        { vec_bounds_low = low; vec_bounds_high = high; return *this; }

        FieldOfView& setOrientation(glm::mat4 const& matrix)
        { mat_orientation = matrix; return *this; }

        FieldOfView& setPerspective(float angle)
        { val_angle = angle; return *this; }

        FieldOfView& setResolution(unsigned width, unsigned height)
        { val_width = width; val_height = height; return *this; }

        FieldOfView& setZoom(float factor)
        { val_zoom = factor; return *this; }

        enum aspect_balancing
        {
            expose_more,
            expose_less,
            stretch
        };

        FieldOfView& setAspectBalancing(aspect_balancing v)
        { enm_aspect_balancing = v; return *this; }

        // dumb accessors

        glm::mat4 const& getOrientation()    const   { return mat_orientation; }
        float            getWidthInPixels()  const   { return val_width; }
        float            getHeightInPixels() const   { return val_height; }
        float            getPerspective()    const   { return val_angle; }

        // matrices

        /**
         * Returns a full transformation matrix to project world coordinates
         * onto the screen.
         */
        glm::mat4 getWorldScreenXform() const;

        /**
         * Transforms world coordinates to viewer-relative coordinates.
         *
         * This matrix can be used as the modelview matrix in legacy GL code
         * where the projection matrix is kept separately.
         */
        glm::mat4 getWorldViewerXform() const;

        /**
         * Returns the transformation to of viewer-relative coordinates back
         * to world space. 
         *
         * This matrix can be used to set up a coordinate system for avatar
         * rendering.
         */
        glm::mat4 getViewerWorldXform() const;

        /**
         * Returns the transformation of viewer-relative coordinates to the
         * screen.
         *
         * This matrix can be used as the projection matrix in legacy GL code.
         */
        glm::mat4 getViewerScreenXform() const;


        // other useful information

        /**
         * Returns the size of a pixel in world space, that is the minimum
         * in respect to x/y screen directions.
         */
        float getPixelSize() const;

        /**
         * Returns the frustum as used for the projection matrices.
         * The result depdends on the bounds, eventually aspect correction
         * for the current resolution, the perspective angle (specified in
         * respect to diagonal) and zoom.
         */
        void getFrustum(glm::vec3& low, glm::vec3& high) const;

        /**
         * Returns the z-offset from the origin to where orientation ia
         * applied.
         */
        float getTransformOffset() const { return vec_bounds_high.z; }
};

#endif

