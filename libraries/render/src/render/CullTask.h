//
//  CullTask.h
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_CullTask_h
#define hifi_render_CullTask_h

#include "Engine.h"
#include "ViewFrustum.h"

namespace render {

    using CullFunctor = std::function<bool(const RenderArgs*, const AABox&)>;

    void cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
        const ItemBounds& inItems, ItemBounds& outItems);

    class FetchNonspatialItems {
    public:
        using JobModel = Job::ModelO<FetchNonspatialItems, ItemBounds>;
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemBounds& outItems);
    };

    class FetchSpatialTreeConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(int numItems READ getNumItems)
        Q_PROPERTY(bool freezeFrustum MEMBER freezeFrustum WRITE setFreezeFrustum)
        Q_PROPERTY(float LODAngle MEMBER lodAngle NOTIFY dirty)
    
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }

        bool freezeFrustum{ false };
    
        float lodAngle{ 2.0 };
    public slots:
        void setFreezeFrustum(bool enabled) { freezeFrustum = enabled; emit dirty(); }

    signals:
        void dirty();
    };

    class FetchSpatialTree {
        bool _freezeFrustum{ false }; // initialized by Config
        bool _justFrozeFrustum{ false };
        ViewFrustum _frozenFrutstum;
        float _lodAngle;
    public:
        using Config = FetchSpatialTreeConfig;
        using JobModel = Job::ModelO<FetchSpatialTree, ItemSpatialTree::ItemSelection, Config>;

        FetchSpatialTree() {}
        FetchSpatialTree(const ItemFilter& filter) : _filter(filter) {}

        ItemFilter _filter{ ItemFilter::Builder::opaqueShape().withoutLayered() };

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemSpatialTree::ItemSelection& outSelection);
    };

    class CullSpatialSelectionConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(int numItems READ getNumItems)
        Q_PROPERTY(bool freezeFrustum MEMBER freezeFrustum WRITE setFreezeFrustum)
        Q_PROPERTY(bool skipCulling MEMBER skipCulling WRITE setSkipCulling)
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }

        bool freezeFrustum{ false };
        bool skipCulling{ false };
    public slots:
        void setFreezeFrustum(bool enabled) { freezeFrustum = enabled; emit dirty(); }
        void setSkipCulling(bool enabled) { skipCulling = enabled; emit dirty(); }
    signals:
        void dirty();
    };

    class CullSpatialSelection {
        bool _freezeFrustum{ false }; // initialized by Config
        bool _justFrozeFrustum{ false };
        bool _skipCulling{ false };
        ViewFrustum _frozenFrutstum;
    public:
        using Config = CullSpatialSelectionConfig;
        using JobModel = Job::ModelIO<CullSpatialSelection, ItemSpatialTree::ItemSelection, ItemBounds, Config>;

        CullSpatialSelection(CullFunctor cullFunctor, RenderDetails::Type type, const ItemFilter& filter) :
            _cullFunctor{ cullFunctor },
            _detailType(type),
            _filter(filter) {}

        CullSpatialSelection(CullFunctor cullFunctor) :
            _cullFunctor{ cullFunctor } {}

        CullFunctor _cullFunctor;
        RenderDetails::Type _detailType{ RenderDetails::OTHER };
        ItemFilter _filter{ ItemFilter::Builder::opaqueShape().withoutLayered() };

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& inSelection, ItemBounds& outItems);
    };

    class FilterItemSelectionConfig : public Job::Config {
        Q_OBJECT
            Q_PROPERTY(int numItems READ getNumItems)
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }
    };

    class FilterItemSelection {
    public:
        using Config = FilterItemSelectionConfig;
        using JobModel = Job::ModelIO<FilterItemSelection, ItemBounds, ItemBounds, Config>;

        FilterItemSelection() {}
        FilterItemSelection(const ItemFilter& filter) :
            _filter(filter) {}

        ItemFilter _filter{ ItemFilter::Builder::opaqueShape().withoutLayered() };

        void configure(const Config& config);
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
    };

    class MultiFilterItemConfig : public Job::Config {
        Q_OBJECT
            Q_PROPERTY(int numItems READ getNumItems)
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }
    };

    template <int NUM_FILTERS>
    class MultiFilterItem {
    public:
        using ItemFilterArray = std::array<ItemFilter, NUM_FILTERS>;
        using ItemBoundsArray = VaryingArray<ItemBounds, NUM_FILTERS>;
        using Config = MultiFilterItemConfig;
        using JobModel = Job::ModelIO<MultiFilterItem, ItemBounds, ItemBoundsArray, Config>;

        MultiFilterItem() {}
        MultiFilterItem(const ItemFilterArray& filters) :
            _filters(filters) {}

        ItemFilterArray _filters;

        void configure(const Config& config) {}
        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBoundsArray& outItems) {
            auto& scene = sceneContext->_scene;
            
            // Clear previous values
            for (size_t i = 0; i < NUM_FILTERS; i++) {
                outItems[i].template edit<ItemBounds>().clear();
            }

            // For each item, filter it into one bucket
            for (auto itemBound : inItems) {
                auto& item = scene->getItem(itemBound.id);
                auto itemKey = item.getKey();
                for (size_t i = 0; i < NUM_FILTERS; i++) {
                    if (_filters[i].test(itemKey)) {
                        outItems[i].template edit<ItemBounds>().emplace_back(itemBound);
                        break;
                    }
                }
            }
        }
    };
}

#endif // hifi_render_CullTask_h;