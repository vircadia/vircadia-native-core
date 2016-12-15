//
//  main.cpp
//  tools/ice-client/src
//
//  Created by Seth Alves on 2016-10-5
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <BuildInfo.h>

#include "ACClientApp.h"

using namespace std;

int main(int argc, char * argv[]) {
    QCoreApplication::setApplicationName(BuildInfo::AC_CLIENT_SERVER_NAME);
    QCoreApplication::setOrganizationName(BuildInfo::MODIFIED_ORGANIZATION);
    QCoreApplication::setOrganizationDomain(BuildInfo::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationVersion(BuildInfo::VERSION);

    ACClientApp app(argc, argv);

    return app.exec();
}
