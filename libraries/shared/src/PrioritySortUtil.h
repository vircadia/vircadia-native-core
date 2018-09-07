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

#include "NumericalConstants.h"
#include "shared/ConicalViewFrustum.h"

//   PrioritySortUtil is a helper for sorting 3D things relative to a ViewFrustum.

const float OUT_OF_VIEW_PENALTY = -10.0f;
const float OUT_OF_VIEW_THRESHOLD = 0.5f * OUT_OF_VIEW_PENALTY;

namespace PrioritySortUtil {

    constexpr float DEFAULT_ANGULAR_COEF { 1.0f };
    constexpr float DEFAULT_CENTER_COEF { 0.5f };
    constexpr float DEFAULT_AGE_COEF { 0.25f };

    class Sortable {
    public:
        virtual glm::vec3 getPosition() const = 0;
        virtual float getRadius() const = 0;
        virtual uint64_t getTimestamp() const = 0;

        void setPriority(float priority) { _priority = priority; }
        float getPriority() const { return _priority; }
    private:
        float _priority { 0.0f };
    };

    template <typename T>
    class PriorityQueue {
    public:
        PriorityQueue() = delete;
        PriorityQueue(const ConicalViewFrustums& views) : _views(views) { }
        PriorityQueue(const ConicalViewFrustums& views, float angularWeight, float centerWeight, float ageWeight)
            : _views(views), _angularWeight(angularWeight), _centerWeight(centerWeight), _ageWeight(ageWeight)
            , _usecCurrentTime(usecTimestampNow()) {
        }

        void setViews(const ConicalViewFrustums& views) { _views = views; }

        void setWeights(float angularWeight, float centerWeight, float ageWeight) {
            _angularWeight = angularWeight;
            _centerWeight = centerWeight;
            _ageWeight = ageWeight;
            _usecCurrentTime = usecTimestampNow();
        }

        size_t size() const { return _vector.size(); }
        void push(T thing) {
            thing.setPriority(computePriority(thing));
            _vector.push_back(thing);
        }
        void reserve(size_t num) {
            _vector.reserve(num);
        }
        const std::vector<T>& getSortedVector() {
            std::sort(_vector.begin(), _vector.end(), [](const T& left, const T& right) { return left.getPriority() > right.getPriority(); });
            return _vector;
        }

    private:

        float computePriority(const T& thing) const {
            float priority = std::numeric_limits<float>::min();

            for (const auto& view : _views) {
                priority = std::max(priority, computePriority(view, thing));
            }

            return priority;
        }

        float computePriority(const ConicalViewFrustum& view, const T& thing) const {
            // priority = weighted linear combination of multiple values:
            //   (a) angular size
            //   (b) proximity to center of view
            //   (c) time since last update
            // where the relative "weights" are tuned to scale the contributing values into units of "priority".

            glm::vec3 position = thing.getPosition();
            glm::vec3 offset = position - view.getPosition();
            float distance = glm::length(offset) + 0.001f; // add 1mm to avoid divide by zero
            const float MIN_RADIUS = 0.1f; // WORKAROUND for zero size objects (we still want them to sort by distance)
            float radius = glm::max(thing.getRadius(), MIN_RADIUS);
            // Other item's angle from view centre:
            float cosineAngle = glm::dot(offset, view.getDirection()) / distance;
            float age = float((_usecCurrentTime - thing.getTimestamp()) / USECS_PER_SECOND);

            // the "age" term accumulates at the sum of all weights
            float angularSize = radius / distance;
            float priority = (_angularWeight * angularSize + _centerWeight * cosineAngle) * (age + 1.0f) + _ageWeight * age;

            // decrement priority of things outside keyhole
            if (distance - radius > view.getRadius()) {
                if (!view.intersects(offset, distance, radius)) {
                    priority += OUT_OF_VIEW_PENALTY;
                }
            }
            return priority;
        }

        ConicalViewFrustums _views;
        std::vector<T> _vector;
        float _angularWeight { DEFAULT_ANGULAR_COEF };
        float _centerWeight { DEFAULT_CENTER_COEF };
        float _ageWeight { DEFAULT_AGE_COEF };
        quint64 _usecCurrentTime { 0 };
    };
} // namespace PrioritySortUtil

  // for now we're keeping hard-coded sorted time budgets in one spot
const uint64_t MAX_UPDATE_RENDERABLES_TIME_BUDGET = 2000; // usec
const uint64_t MIN_SORTED_UPDATE_RENDERABLES_TIME_BUDGET = 1000; // usec
const uint64_t MAX_UPDATE_AVATARS_TIME_BUDGET = 2000; // usec

#endif // hifi_PrioritySortUtil_h
