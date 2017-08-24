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
#include <shared/Algorithms.h>

using namespace alg;

void AutoBaker::addPendingBake(AssetHash hash) {
    _pendingBakes.push_back(hash);

    // Maybe start baking it right away
}

bool AutoBaker::assetNeedsBaking(AssetPath path, AssetHash hash) {
    return path.endsWith(".fbx");
}

BakingStatus AutoBaker::getAssetStatus(AssetHash hash) {
    if (find(_pendingBakes, hash) != std::end(_pendingBakes)) {
        return Pending;
    }

    if (find(_currentlyBaking, hash) != std::end(_currentlyBaking)) {
        return Baking;
    }

    return NotBaked;
}
