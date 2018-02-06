//
//  SkeletonDumpApp.h
//  tools/skeleton-dump/src
//
//  Created by Anthony Thibault on 11/4/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SkeletonDumpApp_h
#define hifi_SkeletonDumpApp_h

#include <QCoreApplication>

class SkeletonDumpApp : public QCoreApplication {
    Q_OBJECT
public:
    SkeletonDumpApp(int argc, char* argv[]);
    ~SkeletonDumpApp();

    int getReturnCode() const { return _returnCode; }

private:
    int _returnCode { 0 };
};

#endif //hifi_SkeletonDumpApp_h
