//
//  VHACDUtil.h
//  tools/vhacd/src
//
//  Created by Virendra Singh on 2/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VHACDUtil_h
#define hifi_VHACDUtil_h

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono> //c++11 feature
#include <QFile>
#include <FBXReader.h>
#include <VHACD.h>

namespace vhacd {

    typedef struct {
        int meshCount;
        QVector<int> convexHullsCountList;
        QVector<QVector<VHACD::IVHACD::ConvexHull>> convexHullList;
    } ComputeResults;

    typedef struct {
        int meshCount;
        QVector<QVector<glm::vec3>> perMeshVertices;
        QVector<QVector<int>> perMeshTriangleIndices;
    } LoadFBXResults;

    class VHACDUtil {
    public:
        bool loadFBX(const QString filename, vhacd::LoadFBXResults *results);
        bool computeVHACD(vhacd::LoadFBXResults *meshes, VHACD::IVHACD::Parameters params, vhacd::ComputeResults *results)const;
        ~VHACDUtil();
    };

    class ProgressCallback : public VHACD::IVHACD::IUserCallback {
    public:
        ProgressCallback(void);
        ~ProgressCallback();

        //Couldn't follow coding guideline here due to virtual function declared in IUserCallback
        void Update(const double overallProgress, const double stageProgress, const double operationProgress, 
		    const char * const stage, const char * const operation);
    };
}
#endif //hifi_VHACDUtil_h
