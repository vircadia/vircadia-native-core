//
//  AutoBaker.h
//  assignment-client/src/assets
//
//  Created by Clement Brisset on 8/9/17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AutoBaker_h
#define hifi_AutoBaker_h

#include <vector>

#include "AssetUtils.h"

class AutoBaker {
public:
    enum Status {
        NotBaked,
        Pending,
        Baking,
        Baked
    };

    void addPendingBake(AssetHash hash);

    bool assetNeedsBaking(AssetHash hash);

    Status getAssetStatus(AssetHash hash);

private:
    std::vector<AssetHash> _pendingBakes;
    std::vector<AssetHash> _currentlyBaking;
};

#endif /* hifi_AutoBaker_h */
