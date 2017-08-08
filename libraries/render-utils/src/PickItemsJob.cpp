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

void PickItemsJob::configure(const Config& config) {
	_isEnabled = config.isEnabled;
}

void PickItemsJob::run(const render::RenderContextPointer& renderContext, const PickItemsJob::Input& input, PickItemsJob::Output& output) {
    output.clear();

	if (_isEnabled) {
		float minIsectDistance = std::numeric_limits<float>::max();
		auto& itemBounds = input;
		auto itemID = findNearestItem(renderContext, itemBounds, minIsectDistance);

        if (render::Item::isValidID(itemID)) {
            output.emplace_back(itemID);
        }
	}
}

render::ItemID PickItemsJob::findNearestItem(const render::RenderContextPointer& renderContext, const render::ItemBounds& inputs, float& minIsectDistance) const {
	const glm::vec3 rayOrigin = renderContext->args->getViewFrustum().getPosition();
	const glm::vec3 rayDirection = renderContext->args->getViewFrustum().getDirection();
	BoxFace face;
	glm::vec3 normal;
	float isectDistance;
	render::ItemID nearestItem = render::Item::INVALID_ITEM_ID;
	const float minDistance = 1.f;
	const float maxDistance = 50.f;

	for (const auto& itemBound : inputs) {
		if (!itemBound.bound.contains(rayOrigin) && itemBound.bound.findRayIntersection(rayOrigin, rayDirection, isectDistance, face, normal)) {
			auto& item = renderContext->_scene->getItem(itemBound.id);
			if (item.getKey().isWorldSpace() && isectDistance>minDistance && isectDistance < minIsectDistance && isectDistance<maxDistance) {
				nearestItem = itemBound.id;
				minIsectDistance = isectDistance;
			}
		}
	}
	return nearestItem;
}