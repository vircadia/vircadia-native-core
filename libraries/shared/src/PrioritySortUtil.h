//
//  PrioritySortUtil.h
//
//  Created by Andrew Meadows on 2017-11-08
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_PrioritySortUtil_h
#define hifi_PrioritySortUtil_h

#include <glm/glm.hpp>
#include "ViewFrustum.h"

/*   PrioritySortUtil is a helper for sorting 3D things relative to a ViewFrustum.  To use:

(1) Derive a class from pure-virtual PrioritySortUtil::Prioritizable
    that wraps the Thing you want to prioritize and sort:

     class PrioritizableThing : public PrioritySortUtil::Prioritizable {
     public:
        PrioritizableThing(const Thing& thing) : _thing(thing) {}
        glm::vec3 getPosition() const override { return _thing.getPosition(); }
        float getRadius() const const override { return _thing.getBoundingRadius(); }
        uint64_t getTimestamp() const override { return _thing.getLastUpdated(); }
     private:
        // Yes really: the data member is a const reference!
        // PrioritizableThing only needs enough scope to compute a priority.
        const Thing& _thing;
     }

(2) Loop over your things and insert each into a priority_queue:

    PrioritySortUtil::Prioritizer prioritizer(viewFrustum);
    std::priority_queue< PrioritySortUtil::Sortable<Thing> > sortedThings;
    for (thing in things) {
        float priority = prioritizer.computePriority(PrioritySortUtil::PrioritizableThing(thing));
        sortedThings.push(PrioritySortUtil::Sortable<Thing> entry(thing, priority));
    }

(4) Loop over your priority queue and do timeboxed work:

    uint64_t cutoffTime = usecTimestampNow() + TIME_BUDGET;
    while (!sortedThings.empty()) {
        const Thing& thing = sortedThings.top();
        // ...do work on thing...
        sortedThings.pop();
        if (usecTimestampNow() > cutoffTime) {
            break;
        }
    }

*/

namespace PrioritySortUtil {

    constexpr float DEFAULT_ANGULAR_COEF { 1.0f };
    constexpr float DEFAULT_CENTER_COEF { 0.5f };
    constexpr float DEFAULT_AGE_COEF { 0.25f / (float)(USECS_PER_SECOND) };

    template <typename T>
    class Sortable {
    public:
        Sortable(const T& thing, float sortPriority) : _thing(thing), _priority(sortPriority) {}
        const T& getThing() const { return  _thing; }
        void setPriority(float priority) { _priority = priority; }
        bool operator<(const Sortable& other) const { return _priority < other._priority; }
    private:
        T _thing;
        float _priority;
    };

    // Prioritizable isn't a template because templates can't have pure-virtual methods.
    class Prioritizable {
    public:
        virtual glm::vec3 getPosition() const = 0;
        virtual float getRadius() const = 0;
        virtual uint64_t getTimestamp() const = 0;
    };

    template <typename T>
    class Prioritizer {
    public:
        Prioritizer() = delete;

        Prioritizer(const ViewFrustum& view) : _view(view) {
            cacheView();
        }

        Prioritizer(const ViewFrustum& view, float angularWeight, float centerWeight, float ageWeight)
                : _view(view), _angularWeight(angularWeight), _centerWeight(centerWeight), _ageWeight(ageWeight)
        {
            cacheView();
        }

        void setView(const ViewFrustum& view) { _view = view; }

        void setWeights(float angularWeight, float centerWeight, float ageWeight) {
            _angularWeight = angularWeight;
            _centerWeight = centerWeight;
            _ageWeight = ageWeight;
        }

        float computePriority(const Prioritizable& prioritizableThing) const {
            // priority = weighted linear combination of multiple values:
            //   (a) angular size
            //   (b) proximity to center of view
            //   (c) time since last update
            // where the relative "weights" are tuned to scale the contributing values into units of "priority".

            glm::vec3 position = prioritizableThing.getPosition();
            glm::vec3 offset = position - _viewPosition;
            float distance = glm::length(offset) + 0.001f; // add 1mm to avoid divide by zero
            float radius = prioritizableThing.getRadius();

            float priority = _angularWeight * (radius / distance)
                + _centerWeight * (glm::dot(offset, _viewForward) / distance)
                + _ageWeight * (float)(usecTimestampNow() - prioritizableThing.getTimestamp());

            // decrement priority of things outside keyhole
            if (distance - radius > _viewRadius) {
                if (!_view.sphereIntersectsFrustum(position, radius)) {
                    constexpr float OUT_OF_VIEW_PENALTY = -10.0f;
                    priority += OUT_OF_VIEW_PENALTY;
                }
            }
            return priority;
        }

    private:
        void cacheView() {
            // assuming we'll prioritize many things: cache these values
            _viewPosition = _view.getPosition();
            _viewForward = _view.getDirection();
            _viewRadius = _view.getCenterRadius();
        }

        ViewFrustum _view;
        glm::vec3 _viewPosition;
        glm::vec3 _viewForward;
        float _viewRadius;
        float _angularWeight { DEFAULT_ANGULAR_COEF };
        float _centerWeight { DEFAULT_CENTER_COEF };
        float _ageWeight { DEFAULT_AGE_COEF };
    };
} // namespace PrioritySortUtil

#endif // hifi_PrioritySortUtil_h

