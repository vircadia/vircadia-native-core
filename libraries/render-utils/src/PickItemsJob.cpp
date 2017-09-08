//
//  PickItemsJob.cpp
//  render-utils/src/
//
//  Created by Olivier Prat on 08/08/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PickItemsJob.h"

PickItemsJob::PickItemsJob(render::ItemKey::Flags validKeys, render::ItemKey::Flags excludeKeys) : _validKeys{ validKeys }, _excludeKeys{ excludeKeys } {
}

void PickItemsJob::configure(const Config& config) {
    _isEnabled = config.pick;
}

void PickItemsJob::run(const render::RenderContextPointer& renderContext, const PickItemsJob::Input& input, PickItemsJob::Output& output) {
    output.clear();
    if (_isEnabled) {
        float minIsectDistance = std::numeric_limits<float>::max();
        auto& itemBounds = input;
        auto item = findNearestItem(renderContext, itemBounds, minIsectDistance);

        if (render::Item::isValidID(item.id)) {
            output.push_back(item);
        }
    }
}

render::ItemBound PickItemsJob::findNearestItem(const render::RenderContextPointer& renderContext, const render::ItemBounds& inputs, float& minIsectDistance) const {
	const glm::vec3 rayOrigin = renderContext->args->getViewFrustum().getPosition();
	const glm::vec3 rayDirection = renderContext->args->getViewFrustum().getDirection();
	BoxFace face;
	glm::vec3 normal;
	float isectDistance;
	render::ItemBound nearestItem( render::Item::INVALID_ITEM_ID );
	const float minDistance = 0.2f;
	const float maxDistance = 50.f;
    render::ItemKey itemKey;

	for (const auto& itemBound : inputs) {
		if (!itemBound.bound.contains(rayOrigin) && itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
			auto& item = renderContext->_scene->getItem(itemBound.id);
            itemKey = item.getKey();
			if (itemKey.isWorldSpace() && isectDistance>minDistance && isectDistance < minIsectDistance && isectDistance<maxDistance
                && (itemKey._flags & _validKeys)!=0 && (itemKey._flags & _excludeKeys)==0) {
				nearestItem = itemBound;
				minIsectDistance = isectDistance;
			}
		}
	}
	return nearestItem;
}