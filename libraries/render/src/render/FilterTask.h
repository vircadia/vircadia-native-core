//
//  FilterTask.h
//  render/src/render
//
//  Created by Sam Gateau on 2/2/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_FilterTask_h
#define hifi_render_FilterTask_h

#include "Engine.h"
#include "ViewFrustum.h"

namespace render {

    class MultiFilterItemsConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(int numItems READ getNumItems)
    public:
        int numItems{ 0 };
        int getNumItems() { return numItems; }
    };

    // Filter inbound of items into multiple buckets defined from the job's Filter array
    template <int NUM_FILTERS>
    class MultiFilterItems {
    public:
        using ItemFilterArray = std::array<ItemFilter, NUM_FILTERS>;
        using ItemBoundsArray = VaryingArray<ItemBounds, NUM_FILTERS>;
        using Config = MultiFilterItemsConfig;
        using JobModel = Job::ModelIO<MultiFilterItems, ItemBounds, ItemBoundsArray, Config>;

        MultiFilterItems() {}
        MultiFilterItems(const ItemFilterArray& filters) :
            _filters(filters) {}

        ItemFilterArray _filters;

        void configure(const Config& config) {}
        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBoundsArray& outItems) {
            auto& scene = renderContext->_scene;
            
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
                    }
                }
            }
        }
    };

    // Filter the items belonging to the job's keep layer
    class FilterLayeredItems {
    public:
        using JobModel = Job::ModelIO<FilterLayeredItems, ItemBounds, ItemBounds>;

        FilterLayeredItems() {}
        FilterLayeredItems(int keepLayer) :
            _keepLayer(keepLayer) {}

        int _keepLayer { 0 };

        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
    };

    // SliceItems job config defining the slice range
    class SliceItemsConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(int rangeOffset MEMBER rangeOffset)
        Q_PROPERTY(int rangeLength MEMBER rangeLength)
        Q_PROPERTY(int numItems READ getNumItems NOTIFY dirty())
        int numItems { 0 };
    public:
        int rangeOffset{ -1 };
        int rangeLength{ 1 };
        int getNumItems() { return numItems; }
        void setNumItems(int n) { numItems = n; emit dirty(); }
    signals:
        void dirty();
    };

    // Keep items in the job slice (defined from config)
    class SliceItems {
    public:
        using Config = SliceItemsConfig;
        using JobModel = Job::ModelIO<SliceItems, ItemBounds, ItemBounds, Config>;
        
        SliceItems() {}
        int _rangeOffset{ -1 };
        int _rangeLength{ 1 };
        
        void configure(const Config& config) {
            _rangeOffset = config.rangeOffset;
            _rangeLength = config.rangeLength;
        }

        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
    };
    
    // Keep items belonging to the job selection
    class SelectItems {
    public:
        using Inputs = VaryingSet2<ItemBounds, ItemBounds>;
        using JobModel = Job::ModelIO<SelectItems, Inputs, ItemBounds>;
        
        std::string _name;
        SelectItems(const Selection::Name& name) : _name(name) {}
        
        void run(const RenderContextPointer& renderContext, const Inputs& inputs, ItemBounds& outItems);
    };

    // Same as SelectItems but reorder the output to match the selection order
    class SelectSortItems {
    public:
        using JobModel = Job::ModelIO<SelectSortItems, ItemBounds, ItemBounds>;

        std::string _name;
        SelectSortItems(const Selection::Name& name) : _name(name) {}

        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
    };

    // From meta-Items, generate the sub-items
    class MetaToSubItems {
    public:
        using JobModel = Job::ModelIO<MetaToSubItems, ItemBounds, ItemIDs>;

        MetaToSubItems() {}

        void run(const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemIDs& outItems);
    };

    // From item IDs build item bounds
    class IDsToBounds {
    public:
        using JobModel = Job::ModelIO<IDsToBounds, ItemIDs, ItemBounds>;

        IDsToBounds(bool disableAABBs = false) : _disableAABBs(disableAABBs) {}

        void run(const RenderContextPointer& renderContext, const ItemIDs& inItems, ItemBounds& outItems);

    private:
        bool _disableAABBs{ false };
    };

}

#endif // hifi_render_FilterTask_h;