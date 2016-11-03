//
//  CrashReporter.h
//  interface/src
//
//  Created by Ryan Huffman on 11 April 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_CrashReporter_h
#define hifi_CrashReporter_h

#include <QString>

#ifdef HAS_BUGSPLAT

#include <BugSplat.h>

class CrashReporter {
public:
    CrashReporter(QString bugSplatDatabase, QString bugSplatApplicationName, QString version);
    
    MiniDmpSender mpSender;
};

#endif

#endif // hifi_CrashReporter_h