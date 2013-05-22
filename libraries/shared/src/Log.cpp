//
// Log.cpp
// hifi
//
// Created by Tobias Schwinger on 4/17/13.
// Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "Log.h"

#include <cstdio>

using namespace std;
int (* printLog)(char const*, ...) = & printf;

