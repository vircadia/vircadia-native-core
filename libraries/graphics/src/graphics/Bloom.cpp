//
//  Bloom.cpp
//  libraries/graphics/src/graphics
//
//  Created by Sam Gondelman on 8/7/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Bloom.h"

using namespace graphics;

const float Bloom::INITIAL_BLOOM_INTENSITY { 0.25f };
const float Bloom::INITIAL_BLOOM_THRESHOLD { 0.7f };
const float Bloom::INITIAL_BLOOM_SIZE { 0.9f };