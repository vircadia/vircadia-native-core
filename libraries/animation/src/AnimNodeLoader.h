//
//  AnimNodeLoader.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimNodeLoader_h
#define hifi_AnimNodeLoader_h

#include <memory>

class AnimNode;

class AnimNodeLoader {
public:
    // TODO: load from url
    std::shared_ptr<AnimNode> load(const std::string& filename) const;
};

#endif // hifi_AnimNodeLoader
