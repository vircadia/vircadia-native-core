//
//  VHACDUtil.h
//  tools/vhacd/src
//
//  Created by Seth Alves on 3/5/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_VHACDUtilApp_h
#define hifi_VHACDUtilApp_h

#include <QCoreApplication>

#include <FBXReader.h>

const int VHACD_RETURN_CODE_FAILURE_TO_READ = 1;
const int VHACD_RETURN_CODE_FAILURE_TO_WRITE = 2;
const int VHACD_RETURN_CODE_FAILURE_TO_CONVEXIFY = 3;


class VHACDUtilApp : public QCoreApplication {
    Q_OBJECT
public:
    VHACDUtilApp(int argc, char* argv[]);
    ~VHACDUtilApp();

    bool writeOBJ(QString outFileName, FBXGeometry& geometry, bool outputCentimeters, int whichMeshPart = -1);

    int getReturnCode() const { return _returnCode; }

private:
    int _returnCode { 0 };
};



#endif //hifi_VHACDUtilApp_h
