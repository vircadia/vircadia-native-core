//
//  AvatarProject.cpp
//
//
//  Created by Thijs Wenker on 12/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarProject.h"

AvatarProject* AvatarProject::openAvatarProject(QString path) {
    const auto pathToLower = path.toLower();
    if (pathToLower.endsWith(".fst")) {
        // TODO: do we open FSTs from any path?
        return new AvatarProject(path);
    }

    if (pathToLower.endsWith(".fbx")) {
        // TODO: Create FST here:

    }

    return nullptr;
}
