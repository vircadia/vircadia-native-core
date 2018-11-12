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
#include <OBJReader.h>
#include <VHACD.h>

namespace vhacd {
    class VHACDUtil {
    public:
        void setVerbose(bool verbose) { _verbose = verbose; }

        bool loadFBX(const QString filename, HFMModel& result);

        void fattenMesh(const HFMMesh& mesh, const glm::mat4& modelOffset, HFMMesh& result) const;

        bool computeVHACD(HFMModel& hfmModel,
                          VHACD::IVHACD::Parameters params,
                          HFMModel& result,
                          float minimumMeshSize, float maximumMeshSize);

        void getConvexResults(VHACD::IVHACD* convexifier, HFMMesh& resultMesh) const;

        ~VHACDUtil();

    private:
        bool _verbose { false };
    };

    class ProgressCallback : public VHACD::IVHACD::IUserCallback {
    public:
        ProgressCallback(void);
        ~ProgressCallback();

        // Couldn't follow coding guideline here due to virtual function declared in IUserCallback
        void Update(const double overallProgress, const double stageProgress, const double operationProgress,
            const char * const stage, const char * const operation) override;
    };
}

AABox getAABoxForMeshPart(const HFMMeshPart &meshPart);

#endif //hifi_VHACDUtil_h
