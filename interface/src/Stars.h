//
// Stars.h
// interface
//
// Created by Tobias Schwinger on 3/22/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Stars__
#define __interface__Stars__

#include "FieldOfView.h"

/**
 * Starfield rendering component.
 */
class Stars 
{
        struct body;
        body* ptr_body;

    public:
        Stars();
        ~Stars();

        /**
         * Reads input file from URL. Returns true upon success.
         *
         * The limit parameter allows to reduce the number of stars
         * that are loaded, keeping the brightest ones.
         */
        bool readInput(const char* url, unsigned limit = 200000);

        /**
         * Renders the starfield from a local viewer's perspective.
         * The parameter specifies the field of view.
         */
        void render(FieldOfView const& fov);

        /**
         * Sets the resolution for FOV culling.
         *
         * The parameter determines the number of tiles in azimuthal 
         * and altitudinal directions. 
         *
         * GPU resources are updated upon change in which case 'true'
         * is returned.
         */
        bool setResolution(unsigned k);

        /**
         * Allows to alter the number of stars to be rendered given a
         * factor. The least brightest ones are omitted first.
         *
         * The further parameters determine when GPU resources should
         * be reallocated. Its value is fractional in respect to the
         * last number of stars 'n' that caused 'n * (1+overalloc)' to
         * be allocated. When the next call to setLOD causes the total
         * number of stars that could be rendered to drop below 'n * 
         * (1-realloc)' or rises above 'n * (1+realloc)' GPU resources
         * are updated. Note that all parameters must be fractions,
         * that is within the range [0;1] and that 'overalloc' must be
         * greater than or equal to 'realloc'.
         *
         * The current level of detail is returned as a float in [0;1].
         */
        float changeLOD(float factor,
                float overalloc = 0.25, float realloc = 0.15);

    private:
        // don't copy/assign
        Stars(Stars const&); // = delete;
        Stars& operator=(Stars const&); // delete;
};


#endif

