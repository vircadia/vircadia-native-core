//
//  Oven.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QTimer>

#include "Oven.h"

static const QString OUTPUT_FOLDER = "/Users/birarda/code/hifi/lod/test-oven/export";

Oven::Oven(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _testBake(QUrl("file:///Users/birarda/code/hifi/lod/test-oven/Test-Object6.fbx"), OUTPUT_FOLDER)
{
    _testBake.start();
}
