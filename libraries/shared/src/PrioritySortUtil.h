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
#include <queue>

#include "NumericalConstants.h"
#include "ViewFrustum.h"

/*   PrioritySortUtil is a helper for sorting 3D things relative to a ViewFrustum.  To use:

(1) Derive a class from pure-virtual PrioritySortUtil::Sortable that wraps a copy of
    the Thing you want to prioritize and sort:

    class SortableWrapper: public PrioritySortUtil::Sortable {
    public:
        SortableWrapper(const Thing& thing) : _thing(thing) { }
        glm::vec3 getPosition() const override { return _thing->getPosition(); }
        float getRadius() const override { return 0.5f * _thing->getBoundingRadius(); }
        uint64_t getTimestamp() const override { return _thing->getLastTime(); }
        const Thing& getThing() const { return _thing; }
    private:
        Thing _thing;
    };

(2) Make a PrioritySortUtil::PriorityQueue<Thing> and add them to the queue:

    PrioritySortUtil::PriorityQueue<SortableWrapper> sortedThings(viewFrustum);
    std::priority_queue< PrioritySortUtil::Sortable<Thing> > sortedThings;
    for (thing in things) {
        sortedThings.push(SortableWrapper(thing));
    }

(3) Loop over your priority queue and do timeboxed work:

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

    class Sortable {
    public:
        virtual glm::vec3 getPosition() const = 0;
        virtual float getRadius() const = 0;
        virtual uint64_t getTimestamp() const = 0;

        void setPriority(float priority) { _priority = priority; }
        float getPriority() const { return _priority; }
        bool operator<(const Sortable& other) const { return _priority < other._priority; }
    private:
        float _priority { 0.0f };
    };

    template <typename T>
    class PriorityQueue {
    public:
        PriorityQueue() = delete;

        PriorityQueue(const ViewFrustum& view) : _view(view) { }

        PriorityQueue(const ViewFrustum& view, float angularWeight, float centerWeight, float ageWeight)
                : _view(view), _angularWeight(angularWeight), _centerWeight(centerWeight), _ageWeight(ageWeight)
        { }

        void setView(const ViewFrustum& view) { _view = view; }

        void setWeights(float angularWeight, float centerWeight, float ageWeight) {
            _angularWeight = angularWeight;
            _centerWeight = centerWeight;
            _ageWeight = ageWeight;
        }

        size_t size() const { return _queue.size(); }
        void push(T thing) {
            thing.setPriority(computePriority(thing));
            _queue.push(thing);
        }
        const T& top() const { return _queue.top(); }
        void pop() { return _queue.pop(); }
        bool empty() const { return _queue.empty(); }

    private:
        float computePriority(const T& thing) const {
            // priority = weighted linear combination of multiple values:
            //   (a) angular size
            //   (b) proximity to center of view
            //   (c) time since last update
            // where the relative "weights" are tuned to scale the contributing values into units of "priority".

            glm::vec3 position = thing.getPosition();
            glm::vec3 offset = position - _view.getPosition();
            float distance = glm::length(offset) + 0.001f; // add 1mm to avoid divide by zero
            const float MIN_RADIUS = 0.1f; // WORKAROUND for zero size objects (we still want them to sort by distance)
            float radius = glm::min(thing.getRadius(), MIN_RADIUS);
            float cosineAngle = (glm::dot(offset, _view.getDirection()) / distance);
            float age = (float)(usecTimestampNow() - thing.getTimestamp());

            // we modulatate "age" drift rate by the cosineAngle term to make periphrial objects sort forward
            // at a reduced rate but we don't want the "age" term to go zero or negative so we clamp it
            const float MIN_COSINE_ANGLE_FACTOR = 0.1f;
            float cosineAngleFactor = glm::max(cosineAngle, MIN_COSINE_ANGLE_FACTOR);

            float priority = _angularWeight * glm::max(radius, MIN_RADIUS) / distance
                + _centerWeight * cosineAngle
                + _ageWeight * cosineAngleFactor * age;

            // decrement priority of things outside keyhole
            if (distance - radius > _view.getCenterRadius()) {
                if (!_view.sphereIntersectsFrustum(position, radius)) {
                    constexpr float OUT_OF_VIEW_PENALTY = -10.0f;
                    priority += OUT_OF_VIEW_PENALTY;
                }
            }
            return priority;
        }

        ViewFrustum _view;
        std::priority_queue<T> _queue;
        float _angularWeight { DEFAULT_ANGULAR_COEF };
        float _centerWeight { DEFAULT_CENTER_COEF };
        float _ageWeight { DEFAULT_AGE_COEF };
    };
} // namespace PrioritySortUtil

// for now we're keeping hard-coded sorted time budgets in one spot
const uint64_t MAX_UPDATE_RENDERABLES_TIME_BUDGET = 2000; // usec
const uint64_t MIN_SORTED_UPDATE_RENDERABLES_TIME_BUDGET = 1000; // usec

#endif // hifi_PrioritySortUtil_h

