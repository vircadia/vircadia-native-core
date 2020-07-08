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

    // Culling Frustum / solidAngle test helper class
    struct CullTest {
        CullFunctor _functor;
        RenderArgs* _args;
        RenderDetails::Item& _renderDetails;
        ViewFrustumPointer _antiFrustum;
        glm::vec3 _eyePos;
        float _squareTanAlpha;

        CullTest(CullFunctor& functor, RenderArgs* pargs, RenderDetails::Item& renderDetails, ViewFrustumPointer antiFrustum = nullptr);

        bool frustumTest(const AABox& bound);
        bool antiFrustumTest(const AABox& bound);
        bool solidAngleTest(const AABox& bound);
        bool zoneOcclusionTest(const render::Item& item);

        static std::unordered_set<QUuid> _containingZones;
        static std::unordered_set<QUuid> _prevContainingZones;
    };

    class FetchNonspatialItems {
    public:
        using JobModel = Job::ModelIO<FetchNonspatialItems, ItemFilter, ItemBounds>;
        void run(const RenderContextPointer& renderContext, const ItemFilter& filter, ItemBounds& outItems);
    };

    class FetchSpatialTreeConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(int numItems READ getNumItems)
        Q_PROPERTY(bool freezeFrustum MEMBER freezeFrustum WRITE setFreezeFrustum)
    
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }

        bool freezeFrustum{ false };
    
    public slots:
        void setFreezeFrustum(bool enabled) { freezeFrustum = enabled; emit dirty(); }

    signals:
        void dirty();
    };

    class FetchSpatialTree {
        bool _freezeFrustum{ false }; // initialized by Config
        bool _justFrozeFrustum{ false };
        ViewFrustum _frozenFrustum;

    public:
        using Config = FetchSpatialTreeConfig;
        using Inputs = render::VaryingSet2<ItemFilter, glm::ivec2>;
        using JobModel = Job::ModelIO<FetchSpatialTree, Inputs, ItemSpatialTree::ItemSelection, Config>;

        FetchSpatialTree() {}

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemSpatialTree::ItemSelection& outSelection);
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
    public:
        using Config = CullSpatialSelectionConfig;
        using Inputs = render::VaryingSet2<ItemSpatialTree::ItemSelection, ItemFilter>;
        using JobModel = Job::ModelIO<CullSpatialSelection, Inputs, ItemBounds, Config>;

        CullSpatialSelection(CullFunctor cullFunctor, bool skipCulling, RenderDetails::Type type) :
            _cullFunctor(cullFunctor),
            _skipCulling(skipCulling),
            _detailType(type) {}

        CullFunctor _cullFunctor;
        bool _skipCulling { false };
        RenderDetails::Type _detailType{ RenderDetails::OTHER };

        bool _freezeFrustum { false }; // initialized by Config
        bool _justFrozeFrustum { false };
        bool _overrideSkipCulling { false };
        ViewFrustum _frozenFrustum;

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemBounds& outItems);
    };

    class CullShapeBounds {
    public:
        using Inputs = render::VaryingSet4<ShapeBounds, ItemFilter, ItemFilter, ViewFrustumPointer>;
        using Outputs = render::VaryingSet2<ShapeBounds, AABox>;
        using JobModel = Job::ModelIO<CullShapeBounds, Inputs, Outputs>;

        CullShapeBounds(CullFunctor cullFunctor, RenderDetails::Type type) :
            _cullFunctor{ cullFunctor },
            _detailType(type) {}

        CullShapeBounds(CullFunctor cullFunctor) :
            _cullFunctor{ cullFunctor } {
        }

        void run(const RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

    private:

        CullFunctor _cullFunctor;
        RenderDetails::Type _detailType{ RenderDetails::OTHER };

    };

    class ApplyCullFunctorOnItemBounds {
    public:
        using Inputs = render::VaryingSet2<ItemBounds, ViewFrustumPointer>;
        using Outputs = ItemBounds;
        using JobModel = Job::ModelIO<ApplyCullFunctorOnItemBounds, Inputs, Outputs>;

        ApplyCullFunctorOnItemBounds(render::CullFunctor cullFunctor) : _cullFunctor(cullFunctor) {}
        void run(const RenderContextPointer& renderContext, const Inputs& inputs, Outputs& outputs);

    private:

        render::CullFunctor _cullFunctor;
    };

    class ClearContainingZones {
    public:
        using JobModel = Job::Model<ClearContainingZones>;
        void run(const RenderContextPointer& renderContext);
    };
}

#endif // hifi_render_CullTask_h;