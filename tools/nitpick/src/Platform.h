//
//  Platform.h
//
//  Created by Nissim Hadar on 2 Apr 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_Platform_h
#define hifi_Platform_h

#include <QString>

class Platform {
public:
    static QString getGraphicsCardType();
};

#endif