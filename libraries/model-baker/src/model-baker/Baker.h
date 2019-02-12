//
//  Baker.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/04.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_baker_Baker_h
#define hifi_baker_Baker_h

#include <QMap>

#include <hfm/HFM.h>

#include "Engine.h"

namespace baker {
    class Baker {
    public:
        Baker(const hfm::Model::Pointer& hfmModel, const QVariantHash& mapping);

        void run();

        // Outputs, available after run() is called
        hfm::Model::Pointer hfmModel;

    protected:
        EnginePointer _engine;
    };

};

#endif //hifi_baker_Baker_h
