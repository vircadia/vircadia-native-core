//
// FieldOfView.cpp
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
        glm::vec3   vec_frustum_low;
        glm::vec3   vec_frustum_high;
        float       val_width;
        float       val_height;
        float       val_angle;
        float       val_zoom;
        int         enm_aspect_balancing;
    public:

        FieldOfView();

        // mutators

        FieldOfView& setFrustum(glm::vec3 const& low, glm::vec3 const& high)
        { vec_frustum_low = low; vec_frustum_high = high; return *this; }

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

        glm::vec3 const& getFrustumLow()     const   { return vec_frustum_low; }
        glm::vec3 const& getFrustumHigh()    const   { return vec_frustum_high; }
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

    private:

        void calcGlFrustum(glm::vec3& low, glm::vec3& high) const;
};

#endif

