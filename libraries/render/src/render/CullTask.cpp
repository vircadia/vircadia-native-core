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

#include <OctreeUtils.h>
#include <PerfStat.h>

using namespace render;

void render::cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
                       const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());

    RenderArgs* args = renderContext->args;
    const ViewFrustum& frustum = args->getViewFrustum();

    details._considered += (int)inItems.size();

    // Culling / LOD
    for (auto item : inItems) {
        if (item.bound.isNull()) {
            outItems.emplace_back(item); // One more Item to render
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool inView;
        {
            PerformanceTimer perfTimer("boxIntersectsFrustum");
            inView = frustum.boxIntersectsFrustum(item.bound);
        }
        if (inView) {
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
    details._rendered += (int)outItems.size();
}

void FetchNonspatialItems::run(const RenderContextPointer& renderContext, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& scene = renderContext->_scene;

    outItems.clear();

    const auto& items = scene->getNonspatialSet();
    outItems.reserve(items.size());
    for (auto& id : items) {
        auto& item = scene->getItem(id);
        outItems.emplace_back(ItemBound(id, item.getBound()));
    }
}

void FetchSpatialTree::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
    _lodAngle = config.lodAngle;
}

void FetchSpatialTree::run(const RenderContextPointer& renderContext, ItemSpatialTree::ItemSelection& outSelection) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;

    // start fresh
    outSelection.clear();

    // Eventually use a frozen frustum
    auto queryFrustum = args->getViewFrustum();
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = args->getViewFrustum();
        }
        queryFrustum = _frozenFrutstum;
    }

    // Octree selection!
    float angle = glm::degrees(getAccuracyAngle(args->_sizeScale, args->_boundaryLevelAdjust));
    scene->getSpatialTree().selectCellItems(outSelection, _filter, queryFrustum, angle);
}

void CullSpatialSelection::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
    _skipCulling = config.skipCulling;
}

void CullSpatialSelection::run(const RenderContextPointer& renderContext,
    const ItemSpatialTree::ItemSelection& inSelection, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;

    auto& details = args->_details.edit(_detailType);
    details._considered += (int)inSelection.numItems();

    // Eventually use a frozen frustum
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = args->getViewFrustum();
        }
        args->pushViewFrustum(_frozenFrutstum); // replace the true view frustum by the frozen one
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
            /*_eyePos = _args->getViewFrustum().getPosition();
            float a = glm::degrees(Octree::getAccuracyAngle(_args->_sizeScale, _args->_boundaryLevelAdjust));
            auto angle = std::min(glm::radians(45.0f), a); // no worse than 45 degrees
            angle = std::max(glm::radians(1.0f / 60.0f), a); // no better than 1 minute of degree
            auto tanAlpha = tan(angle);
            _squareTanAlpha = (float)(tanAlpha * tanAlpha);
            */
        }

        bool frustumTest(const AABox& bound) {
            if (!_args->getViewFrustum().boxIntersectsFrustum(bound)) {
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

    if (_skipCulling) {
        // inside & fit items: filter only, culling is disabled
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

        // inside & subcell items: filter only, culling is disabled
        {
            PerformanceTimer perfTimer("insideSmallItems");
            for (auto id : inSelection.insideSubcellItems) {
                auto& item = scene->getItem(id);
                if (_filter.test(item.getKey())) {
                    ItemBound itemBound(id, item.getBound());
                    outItems.emplace_back(itemBound);
                }
            }
        }

        // partial & fit items: filter only, culling is disabled
        {
            PerformanceTimer perfTimer("partialFitItems");
            for (auto id : inSelection.partialItems) {
                auto& item = scene->getItem(id);
                if (_filter.test(item.getKey())) {
                    ItemBound itemBound(id, item.getBound());
                    outItems.emplace_back(itemBound);
                }
            }
        }

        // partial & subcell items: filter only, culling is disabled
        {
            PerformanceTimer perfTimer("partialSmallItems");
            for (auto id : inSelection.partialSubcellItems) {
                auto& item = scene->getItem(id);
                if (_filter.test(item.getKey())) {
                    ItemBound itemBound(id, item.getBound());
                    outItems.emplace_back(itemBound);
                }
            }
        }

    } else {

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
    }

    details._rendered += (int)outItems.size();


    // Restore frustum if using the frozen one:
    if (_freezeFrustum) {
        args->popViewFrustum();
    }

    std::static_pointer_cast<Config>(renderContext->jobConfig)->numItems = (int)outItems.size();
}
