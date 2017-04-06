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

        Selection(const std::string& name, const ItemIDs items);
        ~Selection();

        const std::string& getName() const { return _name; }

        const ItemIDs& getItems() const { return _items; }

    protected:
        const std::string _name;
        ItemIDs _items;
    };
    using Selections = std::vector<Selection>;

    using SelectionMap = std::map<std::string, Selection>;

}

#endif