//
// avatars_Log.cpp
// hifi
//
// Created by Tobias Schwinger on 4/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "shared_Log.h"

#include <cstdio>

namespace avatars_lib {
    using namespace std;

    int (* printLog)(char const*, ...) = & printf;
}
