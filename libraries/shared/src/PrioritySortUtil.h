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

namespace PrioritySortUtil {
    // PrioritySortUtil is a helper template for sorting 3D objects relative to a ViewFrustum.
    // To use this utility:
    //
    // (1) Declare and implent the following methods for your "object" type T:
    //
    //     glm::vec3 PrioritySortUtil<typename T>::getObjectPosition(const T&);
    //     float PrioritySortUtil<typename T>::getObjectRadius(const T&);
    //     uint64_t PrioritySortUtil<typename T>::getObjectAge(const T&);
    //
    // (2) Below the implementation in (1):
    //
    //     #include <PrioritySortUtil.h>
    //
    // (3) Create a PriorityCalculator instance:
    //
    //     PrioritySortUtil::PriorityCalculator<T> calculator(viewFrustum);
    //
    // (4) Loop over your objects and insert the into a priority_queue:
    //
    //     std::priority_queue< PrioritySortUtil::Sortable<T> > sortedObjects;
    //     for (T obj in objects) {
    //         float priority = calculator.computePriority(obj);
    //         PrioritySortUtil::Sortable<T> entry(obj, priority);
    //         sortedObjects.push(entry);
    //     }

    template <typename T>
    class Sortable {
    public:
        Sortable(const T& object, float sortPriority) : _object(object), _priority(sortPriority) {}
        const T& getObject() const { return  _object; }
        void setPriority(float priority) { _priority = priority; }
        bool operator<(const Sortable& other) const { return _priority < other._priority; }
    private:
        T _object;
        float _priority;
    };

    constexpr float DEFAULT_ANGULAR_COEF { 1.0f };
    constexpr float DEFAULT_CENTER_COEF { 0.5f };
    constexpr float DEFAULT_AGE_COEF { 0.25f / (float)(USECS_PER_SECOND) };

    template <typename T>
    class PriorityCalculator {
    public:
        PriorityCalculator() = delete;

        PriorityCalculator(const ViewFrustum& view) : _view(view) {
            cacheView();
        }

        PriorityCalculator(const ViewFrustum& view, float angularWeight, float centerWeight, float ageWeight)
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

        float computePriority(T& object) const {
            // priority = weighted linear combination of multiple values:
            //   (a) angular size
            //   (b) proximity to center of view
            //   (c) time since last update
            // where the relative "weights" are tuned to scale the contributing values into units of "priority".

            glm::vec3 position = PrioritySortUtil::getObjectPosition(object);
            glm::vec3 offset = position - _viewPosition;
            float distance = glm::length(offset) + 0.001f; // add 1mm to avoid divide by zero
            float radius = PrioritySortUtil::getObjectRadius(object);

            float priority = _angularWeight * (radius / distance)
                + _centerWeight * (glm::dot(offset, _viewForward) / distance)
                + _ageWeight * (float)(usecTimestampNow() - PrioritySortUtil::getObjectAge(object));

            // decrement priority of things outside keyhole
            if (distance + radius > _viewRadius) {
                if (!_view.sphereIntersectsFrustum(position, radius)) {
                    constexpr float OUT_OF_VIEW_PENALTY = -10.0f;
                    priority += OUT_OF_VIEW_PENALTY;
                }
            }
            return priority;
        }

    private:
        void cacheView() {
            // assuming we'll prioritize many objects: cache these values
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
}
#endif // hifi_PrioritySortUtil_h

