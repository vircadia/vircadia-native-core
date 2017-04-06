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

Selection::Selection(const std::string& name, const ItemIDs items) :
    _name(name),
    _items(items)
{

}

Selection::~Selection() {

}

