//
//  CullTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CullTask.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

using namespace render;

void render::cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
                       const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);

    RenderArgs* args = renderContext->args;
    ViewFrustum* frustum = args->_viewFrustum;

    details._considered += inItems.size();
    
    // Culling / LOD
    for (auto item : inItems) {
        if (item.bound.isNull()) {
            outItems.emplace_back(item); // One more Item to render
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool outOfView;
        {
            PerformanceTimer perfTimer("boxInFrustum");
            outOfView = frustum->boxInFrustum(item.bound) == ViewFrustum::OUTSIDE;
        }
        if (!outOfView) {
            bool bigEnoughToRender;
            {
                PerformanceTimer perfTimer("shouldRender");
                bigEnoughToRender = cullFunctor(args, item.bound);
            }
            if (bigEnoughToRender) {
                outItems.emplace_back(item); // One more Item to render
            } else {
                details._tooSmall++;
            }
        } else {
            details._outOfView++;
        }
    }
    details._rendered += outItems.size();
}

struct ItemBoundSort {
    float _centerDepth = 0.0f;
    float _nearDepth = 0.0f;
    float _farDepth = 0.0f;
    ItemID _id = 0;
    AABox _bounds;

    ItemBoundSort() {}
    ItemBoundSort(float centerDepth, float nearDepth, float farDepth, ItemID id, const AABox& bounds) : _centerDepth(centerDepth), _nearDepth(nearDepth), _farDepth(farDepth), _id(id), _bounds(bounds) {}
};

struct FrontToBackSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth < right._centerDepth);
    }
};

struct BackToFrontSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth > right._centerDepth);
    }
};

void render::depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    

    // Allocate and simply copy
    outItems.clear();
    outItems.reserve(inItems.size());


    // Make a local dataset of the center distance and closest point distance
    std::vector<ItemBoundSort> itemBoundSorts;
    itemBoundSorts.reserve(outItems.size());

    for (auto itemDetails : inItems) {
        auto item = scene->getItem(itemDetails.id);
        auto bound = itemDetails.bound; // item.getBound();
        float distance = args->_viewFrustum->distanceToCamera(bound.calcCenter());

        itemBoundSorts.emplace_back(ItemBoundSort(distance, distance, distance, itemDetails.id, bound));
    }

    // sort against Z
    if (frontToBack) {
        FrontToBackSort frontToBackSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), frontToBackSort);
    } else {
        BackToFrontSort  backToFrontSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), backToFrontSort);
    }

    // FInally once sorted result to a list of itemID
    for (auto& item : itemBoundSorts) {
       outItems.emplace_back(ItemBound(item._id, item._bounds));
    }
}

void DepthSortItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    depthSortItems(sceneContext, renderContext, _frontToBack, inItems, outItems);
}


void FetchItems::configure(const Config& config) {
}

void FetchItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    auto& scene = sceneContext->_scene;

    outItems.clear();

    const auto& bucket = scene->getMasterBucket();
    const auto& items = bucket.find(_filter);
    if (items != bucket.end()) {
        outItems.reserve(items->second.size());
        for (auto& id : items->second) {
            auto& item = scene->getItem(id);
            outItems.emplace_back(ItemBound(id, item.getBound()));
        }
    }

    std::static_pointer_cast<Config>(renderContext->jobConfig)->numItems = (int)outItems.size();
}



void FetchSpatialTree::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
    _lodAngle = config.lodAngle;
}

void FetchSpatialTree::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemSpatialTree::ItemSelection& outSelection) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;

    // start fresh
    outSelection.clear();

    // Eventually use a frozen frustum
    auto queryFrustum = *args->_viewFrustum;
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = *args->_viewFrustum;
        }
        queryFrustum = _frozenFrutstum;
    }

    // Octree selection!

    float angle = glm::degrees(queryFrustum.getAccuracyAngle(args->_sizeScale, args->_boundaryLevelAdjust));


    scene->getSpatialTree().selectCellItems(outSelection, _filter, queryFrustum, angle);

}

void CullSpatialSelection::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
}

void CullSpatialSelection::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
    const ItemSpatialTree::ItemSelection& inSelection, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;

    auto& details = args->_details.edit(_detailType);
    details._considered += inSelection.numItems();

    // Eventually use a frozen frustum
    auto argFrustum = args->_viewFrustum;
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = *args->_viewFrustum;
        }
        args->_viewFrustum = &_frozenFrutstum; // replace the true view frustum by the frozen one
    }

    // Culling Frustum / solidAngle test helper class
    struct Test {
        CullFunctor _functor;
        RenderArgs* _args;
        RenderDetails::Item& _renderDetails;
        glm::vec3 _eyePos;
        float _squareTanAlpha;

        Test(CullFunctor& functor, RenderArgs* pargs, RenderDetails::Item& renderDetails) :
            _functor(functor),
            _args(pargs),
            _renderDetails(renderDetails)
        {
            // FIXME: Keep this code here even though we don't use it yet
            /*_eyePos = _args->_viewFrustum->getPosition();
            float a = glm::degrees(_args->_viewFrustum->getAccuracyAngle(_args->_sizeScale, _args->_boundaryLevelAdjust));
            auto angle = std::min(glm::radians(45.0f), a); // no worse than 45 degrees
            angle = std::max(glm::radians(1.0f / 60.0f), a); // no better than 1 minute of degree
            auto tanAlpha = tan(angle);
            _squareTanAlpha = (float)(tanAlpha * tanAlpha);
            */
        }

        bool frustumTest(const AABox& bound) {
            if (_args->_viewFrustum->boxInFrustum(bound) == ViewFrustum::OUTSIDE) {
                _renderDetails._outOfView++;
                return false;
            }
            return true;
        }

        bool solidAngleTest(const AABox& bound) {
            // FIXME: Keep this code here even though we don't use it yet
            //auto eyeToPoint = bound.calcCenter() - _eyePos;
            //auto boundSize = bound.getDimensions();
            //float test = (glm::dot(boundSize, boundSize) / glm::dot(eyeToPoint, eyeToPoint)) - squareTanAlpha;
            //if (test < 0.0f) {
            if (!_functor(_args, bound)) {
                _renderDetails._tooSmall++;
                return false;
            }
            return true;
        }
    };
    Test test(_cullFunctor, args, details);

    // Now we have a selection of items to render
    outItems.clear();
    outItems.reserve(inSelection.numItems());

    // Now get the bound, and
    // filter individually against the _filter
    // visibility cull if partially selected ( octree cell contianing it was partial)
    // distance cull if was a subcell item ( octree cell is way bigger than the item bound itself, so now need to test per item)

    // inside & fit items: easy, just filter
    {
        PerformanceTimer perfTimer("insideFitItems");
        for (auto id : inSelection.insideItems) {
            auto& item = scene->getItem(id);
            if (_filter.test(item.getKey())) {
                ItemBound itemBound(id, item.getBound());
                outItems.emplace_back(itemBound);
            }
        }
    }

    // inside & subcell items: filter & distance cull
    {
        PerformanceTimer perfTimer("insideSmallItems");
        for (auto id : inSelection.insideSubcellItems) {
            auto& item = scene->getItem(id);
            if (_filter.test(item.getKey())) {
                ItemBound itemBound(id, item.getBound());
                if (test.solidAngleTest(itemBound.bound)) {
                    outItems.emplace_back(itemBound);
                }
            }
        }
    }

    // partial & fit items: filter & frustum cull
    {
        PerformanceTimer perfTimer("partialFitItems");
        for (auto id : inSelection.partialItems) {
            auto& item = scene->getItem(id);
            if (_filter.test(item.getKey())) {
                ItemBound itemBound(id, item.getBound());
                if (test.frustumTest(itemBound.bound)) {
                    outItems.emplace_back(itemBound);
                }
            }
        }
    }

    // partial & subcell items:: filter & frutum cull & solidangle cull
    {
        PerformanceTimer perfTimer("partialSmallItems");
        for (auto id : inSelection.partialSubcellItems) {
            auto& item = scene->getItem(id);
            if (_filter.test(item.getKey())) {
                ItemBound itemBound(id, item.getBound());
                if (test.frustumTest(itemBound.bound)) {
                    if (test.solidAngleTest(itemBound.bound)) {
                        outItems.emplace_back(itemBound);
                    }
                }
            }
        }
    }

    details._rendered += outItems.size();


    // Restore frustum if using the frozen one:
    if (_freezeFrustum) {
        args->_viewFrustum = argFrustum;
    }

    std::static_pointer_cast<Config>(renderContext->jobConfig)->numItems = (int)outItems.size();
}
