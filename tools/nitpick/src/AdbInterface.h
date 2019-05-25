//
//  AdbInterface.h
//
//  Created by Nissim Hadar on Feb 11, 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AdbInterface_h
#define hifi_AdbInterface_h

#include <QString>

class AdbInterface {
public:
    QString getAdbCommand();

private:

    QString _adbCommand;
};

#endif // hifi_AdbInterface_h
