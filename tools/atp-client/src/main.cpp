//
//  main.cpp
//  tools/atp-client/src
//
//  Created by Seth Alves on 2017-3-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <BuildInfo.h>

#include "ATPClientApp.h"

using namespace std;

int main(int argc, char * argv[]) {
    QCoreApplication::setApplicationName(BuildInfo::AC_CLIENT_SERVER_NAME);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    ATPClientApp app(argc, argv);

    return app.exec();
}
