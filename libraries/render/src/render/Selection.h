//
//  Selection.h
//  render/src/render
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Selection_h
#define hifi_render_Selection_h

#include "Item.h"

namespace render {

    class Selection {
    public:
        using Name = std::string;

        ~Selection();
        Selection();
        Selection(const Selection& selection);
        Selection& operator = (const Selection& selection);
        Selection(Selection&& selection);
        Selection& operator = (Selection&& selection);

        Selection(const Name& name, const ItemIDs& items);

        const Name& getName() const { return _name; }

        const ItemIDs& getItems() const { return _items; }

        bool isEmpty() const { return _items.empty(); }

        // Test if the ID is in the selection, return the index or -1 if not present
        static const int NOT_FOUND{ -1 };
                
        int find(ItemID id) const;
        bool contains(ItemID id) const { return find(id) > NOT_FOUND; }

    protected:
        Name _name;
        ItemIDs _items;
    };
    using Selections = std::vector<Selection>;

    using SelectionMap = std::map<const Selection::Name, Selection>;

}

#endif