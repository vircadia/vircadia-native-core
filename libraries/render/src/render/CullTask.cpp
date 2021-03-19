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
#include <OctreeUtils.h>

using namespace render;

std::unordered_set<QUuid> CullTest::_containingZones = std::unordered_set<QUuid>();
std::unordered_set<QUuid> CullTest::_prevContainingZones = std::unordered_set<QUuid>();

CullTest::CullTest(CullFunctor& functor, RenderArgs* pargs, RenderDetails::Item& renderDetails, ViewFrustumPointer antiFrustum) :
    _functor(functor),
    _args(pargs),
    _renderDetails(renderDetails),
    _antiFrustum(antiFrustum) {
    // FIXME: Keep this code here even though we don't use it yet
    /*_eyePos = _args->getViewFrustum().getPosition();
    float a = glm::degrees(Octree::getPerspectiveAccuracyAngle(_args->_sizeScale, _args->_boundaryLevelAdjust));
    auto angle = std::min(glm::radians(45.0f), a); // no worse than 45 degrees
    angle = std::max(glm::radians(1.0f / 60.0f), a); // no better than 1 minute of degree
    auto tanAlpha = tan(angle);
    _squareTanAlpha = (float)(tanAlpha * tanAlpha);
    */
}

bool CullTest::frustumTest(const AABox& bound) {
    if (!_args->getViewFrustum().boxIntersectsFrustum(bound)) {
        _renderDetails._outOfView++;
        return false;
    }
    return true;
}

bool CullTest::antiFrustumTest(const AABox& bound) {
    assert(_antiFrustum);
    if (_antiFrustum->boxInsideFrustum(bound)) {
        _renderDetails._outOfView++;
        return false;
    }
    return true;
}

bool CullTest::solidAngleTest(const AABox& bound) {
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

bool CullTest::zoneOcclusionTest(const render::Item& item) {
    return item.passesZoneOcclusionTest(_containingZones);
}

void FetchNonspatialItems::run(const RenderContextPointer& renderContext, const ItemFilter& filter, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    auto& scene = renderContext->_scene;

    outItems.clear();

    const auto& items = scene->getNonspatialSet();
    outItems.reserve(items.size());
    for (auto& id : items) {
        auto& item = scene->getItem(id);
        if (filter.test(item.getKey()) && item.passesZoneOcclusionTest(CullTest::_containingZones)) {
            outItems.emplace_back(ItemBound(id, item.getBound(renderContext->args)));
        }
    }
}

void FetchSpatialTree::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
}

void FetchSpatialTree::run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemSpatialTree::ItemSelection& outSelection) {
    if (!renderContext){
        return;
    }
    
    // start fresh
    outSelection.clear();

    auto& filter = inputs.get0();
    auto frustumResolution = inputs.get1();

    if (!filter.selectsNothing()) {
        assert(renderContext->args);
        assert(renderContext->args->hasViewFrustum());
        RenderArgs* args = renderContext->args;
        auto& scene = renderContext->_scene;

        if (!args) {
            return;
        }

        auto queryFrustum = args->getViewFrustum();
        // Eventually use a frozen frustum
        if (_freezeFrustum) {
            if (_justFrozeFrustum) {
                _justFrozeFrustum = false;
                _frozenFrustum = args->getViewFrustum();
            }
            queryFrustum = _frozenFrustum;
        }

        // Octree selection!
        float threshold = 0.0f;
        if (queryFrustum.isPerspective()) {
            threshold = args->_lodAngleHalfTan;
            if (frustumResolution.y > 0) {
                threshold = glm::max(queryFrustum.getFieldOfView() / frustumResolution.y, threshold);
            }
        } else {
            threshold = getOrthographicAccuracySize(args->_sizeScale, args->_boundaryLevelAdjust);
            glm::vec2 frustumSize = glm::vec2(queryFrustum.getWidth(), queryFrustum.getHeight());
            const auto pixelResolution = frustumResolution.x > 0 ? frustumResolution : glm::ivec2(2048, 2048);
            threshold = glm::max(threshold, glm::min(frustumSize.x / pixelResolution.x, frustumSize.y / pixelResolution.y));
        }
        scene->getSpatialTree().selectCellItems(outSelection, filter, queryFrustum, threshold);
    }
}

void CullSpatialSelection::configure(const Config& config) {
    _justFrozeFrustum = _justFrozeFrustum || (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
    _overrideSkipCulling = config.skipCulling;
}

void CullSpatialSelection::run(const RenderContextPointer& renderContext,
                               const Inputs& inputs, ItemBounds& outItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;
    auto& inSelection = inputs.get0();

    auto& details = args->_details.edit(_detailType);
    details._considered += (int)inSelection.numItems();

    // Eventually use a frozen frustum
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrustum = args->getViewFrustum();
        }
        args->pushViewFrustum(_frozenFrustum); // replace the true view frustum by the frozen one
    }

    CullTest test(_cullFunctor, args, details);

    // Now we have a selection of items to render
    outItems.clear();
    outItems.reserve(inSelection.numItems());

    const auto srcFilter = inputs.get1();
    if (!srcFilter.selectsNothing()) {
        auto filter = render::ItemFilter::Builder(srcFilter).withoutSubMetaCulled().build();

        // Now get the bound, and
        // filter individually against the _filter
        // visibility cull if partially selected ( octree cell contianing it was partial)
        // distance cull if was a subcell item ( octree cell is way bigger than the item bound itself, so now need to test per item)
        if (_skipCulling || _overrideSkipCulling) {
            // inside & fit items: filter only, culling is disabled
            {
                PerformanceTimer perfTimer("insideFitItems");
                for (auto id : inSelection.insideItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        outItems.emplace_back(itemBound);
                        if (item.getKey().isMetaCullGroup()) {
                            item.fetchMetaSubItemBounds(outItems, (*scene), args);
                        }
                    }
                }
            }

            // inside & subcell items: filter only, culling is disabled
            {
                PerformanceTimer perfTimer("insideSmallItems");
                for (auto id : inSelection.insideSubcellItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        outItems.emplace_back(itemBound);
                        if (item.getKey().isMetaCullGroup()) {
                            item.fetchMetaSubItemBounds(outItems, (*scene), args);
                        }
                    }
                }
            }

            // partial & fit items: filter only, culling is disabled
            {
                PerformanceTimer perfTimer("partialFitItems");
                for (auto id : inSelection.partialItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        outItems.emplace_back(itemBound);
                        if (item.getKey().isMetaCullGroup()) {
                            item.fetchMetaSubItemBounds(outItems, (*scene), args);
                        }
                    }
                }
            }

            // partial & subcell items: filter only, culling is disabled
            {
                PerformanceTimer perfTimer("partialSmallItems");
                for (auto id : inSelection.partialSubcellItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        outItems.emplace_back(itemBound);
                        if (item.getKey().isMetaCullGroup()) {
                            item.fetchMetaSubItemBounds(outItems, (*scene), args);
                        }
                    }
                }
            }

        } else {

            // inside & fit items: easy, just filter
            {
                PerformanceTimer perfTimer("insideFitItems");
                for (auto id : inSelection.insideItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        outItems.emplace_back(itemBound);
                        if (item.getKey().isMetaCullGroup()) {
                            item.fetchMetaSubItemBounds(outItems, (*scene), args);
                        }
                    }
                }
            }

            // inside & subcell items: filter & distance cull
            {
                PerformanceTimer perfTimer("insideSmallItems");
                for (auto id : inSelection.insideSubcellItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        if (test.solidAngleTest(itemBound.bound)) {
                            outItems.emplace_back(itemBound);
                            if (item.getKey().isMetaCullGroup()) {
                                item.fetchMetaSubItemBounds(outItems, (*scene), args);
                            }
                        }
                    }
                }
            }

            // partial & fit items: filter & frustum cull
            {
                PerformanceTimer perfTimer("partialFitItems");
                for (auto id : inSelection.partialItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        if (test.frustumTest(itemBound.bound)) {
                            outItems.emplace_back(itemBound);
                            if (item.getKey().isMetaCullGroup()) {
                                item.fetchMetaSubItemBounds(outItems, (*scene), args);
                            }
                        }
                    }
                }
            }

            // partial & subcell items:: filter & frutum cull & solidangle cull
            {
                PerformanceTimer perfTimer("partialSmallItems");
                for (auto id : inSelection.partialSubcellItems) {
                    auto& item = scene->getItem(id);
                    if (filter.test(item.getKey()) && test.zoneOcclusionTest(item)) {
                        ItemBound itemBound(id, item.getBound(args));
                        if (test.frustumTest(itemBound.bound) && test.solidAngleTest(itemBound.bound)) {
                            outItems.emplace_back(itemBound);
                            if (item.getKey().isMetaCullGroup()) {
                                item.fetchMetaSubItemBounds(outItems, (*scene), args);
                            }
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

void CullShapeBounds::run(const RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    const auto& inShapes = inputs.get0();
    const auto& cullFilter = inputs.get1();
    const auto& boundsFilter = inputs.get2();
    ViewFrustumPointer antiFrustum;
    auto& outShapes = outputs.edit0();
    auto& outBounds = outputs.edit1();

    if (!inputs[3].isNull()) {
        antiFrustum = inputs.get3();
    }
    outShapes.clear();
    outBounds = AABox();

    if (!cullFilter.selectsNothing() || !boundsFilter.selectsNothing()) {
        auto& details = args->_details.edit(_detailType);
        CullTest test(_cullFunctor, args, details, antiFrustum);
        auto scene = args->_scene;

        for (auto& inItems : inShapes) {
            auto key = inItems.first;
            auto outItems = outShapes.find(key);
            if (outItems == outShapes.end()) {
                outItems = outShapes.insert(std::make_pair(key, ItemBounds{})).first;
                outItems->second.reserve(inItems.second.size());
            }

            details._considered += (int)inItems.second.size();

            if (antiFrustum == nullptr) {
                for (auto& item : inItems.second) {
                    if (test.solidAngleTest(item.bound) && test.frustumTest(item.bound)) {
                        const auto shapeKey = scene->getItem(item.id).getKey();
                        if (cullFilter.test(shapeKey)) {
                            outItems->second.emplace_back(item);
                        }
                        if (boundsFilter.test(shapeKey)) {
                            outBounds += item.bound;
                        }
                    }
                }
            } else {
                for (auto& item : inItems.second) {
                    if (test.solidAngleTest(item.bound) && test.frustumTest(item.bound) && test.antiFrustumTest(item.bound)) {
                        const auto shapeKey = scene->getItem(item.id).getKey();
                        if (cullFilter.test(shapeKey)) {
                            outItems->second.emplace_back(item);
                        }
                        if (boundsFilter.test(shapeKey)) {
                            outBounds += item.bound;
                        }
                    }
                }
            }
            details._rendered += (int)outItems->second.size();
        }

        for (auto& items : outShapes) {
            items.second.shrink_to_fit();
        }
    }
}

void ApplyCullFunctorOnItemBounds::run(const RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& inItems = inputs.get0();
    auto& outItems = outputs;
    auto inputFrustum = inputs.get1();

    if (inputFrustum != nullptr) {
        args->pushViewFrustum(*inputFrustum);
    }

    outItems.clear();
    outItems.reserve(inItems.size());

    for (auto& item : inItems) {
        if (_cullFunctor(args, item.bound)) {
            outItems.emplace_back(item);
        }
    }

    if (inputFrustum != nullptr) {
        args->popViewFrustum();
    }
}

void ClearContainingZones::run(const RenderContextPointer& renderContext) {
    // This is a bit of a hack.  We want to do zone culling as early as possible, so we do it
    // during the RenderFetchCullSortTask (in CullSpatialSelection and FetchNonspatialItems),
    // but the zones aren't collected until after (in SetupZones).  To get around this,
    // we actually use the zones from the previous frame to render, and then clear at the beginning
    // of the next frame
    CullTest::_prevContainingZones = CullTest::_containingZones;
    CullTest::_containingZones.clear();
}