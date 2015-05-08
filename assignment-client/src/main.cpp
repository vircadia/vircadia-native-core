//
//  main.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssignmentClientApp.h"

#include <QtCore/QDebug>

int main(int argc, char* argv[]) {
    AssignmentClientApp app(argc, argv);
    
    int acReturn = app.exec();
    qDebug() << "assignment-client process" <<  app.applicationPid() << "exiting with status code" << acReturn;
    
    return acReturn;
}
