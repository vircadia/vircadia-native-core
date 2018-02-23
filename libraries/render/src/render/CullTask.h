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
        using JobModel = Job::ModelIO<FetchNonspatialItems, ItemFilter, ItemBounds>;
        void run(const RenderContextPointer& renderContext, const ItemFilter& filter, ItemBounds& outItems);
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
        ViewFrustum _frozenFrustum;
        float _lodAngle;
    public:
        using Config = FetchSpatialTreeConfig;
        using JobModel = Job::ModelIO<FetchSpatialTree, ItemFilter, ItemSpatialTree::ItemSelection, Config>;

        FetchSpatialTree() {}

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const ItemFilter& filter, ItemSpatialTree::ItemSelection& outSelection);
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
        ViewFrustum _frozenFrustum;
    public:
        using Config = CullSpatialSelectionConfig;
        using Inputs = render::VaryingSet2<ItemSpatialTree::ItemSelection, ItemFilter>;
        using JobModel = Job::ModelIO<CullSpatialSelection, Inputs, ItemBounds, Config>;

        CullSpatialSelection(CullFunctor cullFunctor, RenderDetails::Type type) :
            _cullFunctor{ cullFunctor },
            _detailType(type) {}

        CullSpatialSelection(CullFunctor cullFunctor) :
            _cullFunctor{ cullFunctor } {}

        CullFunctor _cullFunctor;
        RenderDetails::Type _detailType{ RenderDetails::OTHER };

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemBounds& outItems);
    };

}

#endif // hifi_render_CullTask_h;