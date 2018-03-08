//
//  OvenCLIApplication.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 2/20/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OvenCLIApplication_h
#define hifi_OvenCLIApplication_h

#include <QtCore/QCoreApplication>

#include "Oven.h"

class OvenCLIApplication : public QCoreApplication, public Oven {
    Q_OBJECT
public:
    OvenCLIApplication(int argc, char* argv[]);

    static OvenCLIApplication* instance() { return dynamic_cast<OvenCLIApplication*>(QCoreApplication::instance()); }
};

#endif // hifi_OvenCLIApplication_h
