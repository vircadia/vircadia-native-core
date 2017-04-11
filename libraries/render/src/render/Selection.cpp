//
//  Selection.cpp
//  render/src/render
//
//  Created by Sam Gateau on 4/4/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Selection.h"

#include "Logging.h"

using namespace render;


Selection::~Selection() {

}

Selection::Selection() :
_name(),
_items()
{
}

Selection::Selection(const Selection& selection) :
_name(selection._name),
_items(selection._items)
{
}

Selection& Selection::operator= (const Selection& selection) {
    _name = (selection._name);
    _items = (selection._items);
    return (*this);
}

Selection::Selection(Selection&& selection) :
_name(selection._name),
_items(selection._items)
{
}


Selection& Selection::operator= (Selection&& selection) {
    _name = (selection._name);
    _items = (selection._items);
    return (*this);
}

Selection::Selection(const Name& name, const ItemIDs& items) :
    _name(name),
    _items(items)
{
}

int Selection::find(ItemID id) const {
    int index = 0;
    for (auto selected : _items) {
        if (selected == id) {
            return index;
        }
        index++;
    }
    return NOT_FOUND;
}
