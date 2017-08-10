//
//  AutoBaker.cpp
//  assignment-client/src/assets
//
//  Created by Clement Brisset on 8/9/17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AutoBaker.h"

void AutoBaker::addPendingBake(AssetHash hash) {
    _pendingBakes.push_back(hash);
}

bool AutoBaker::assetNeedsBaking(AssetHash hash) {
    return true;
}

AutoBaker::Status AutoBaker::getAssetStatus(AssetHash hash) {
    auto pendingIt = std::find(_pendingBakes.cbegin(), _pendingBakes.cend(), hash);
    if (pendingIt != _pendingBakes.cend()) {
        return Pending;
    }

    auto bakingIt = std::find(_currentlyBaking.cbegin(), _currentlyBaking.cend(), hash);
    if (bakingIt != _currentlyBaking.cend()) {
        return Baking;
    }

    if (assetNeedsBaking(hash)) {
        return NotBaked;
    } else {
        return Baked;
    }
}
