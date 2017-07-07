//
//  SceneTask.h
//  render/src/render
//
//  Created by Sam Gateau on 6/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_SceneTask_h
#define hifi_render_SceneTask_h

#include "Engine.h"

namespace render {

    class PerformSceneTransactionConfig : public Job::Config {
        Q_OBJECT
    public:
    signals:
        void dirty();

    protected:
    };

    class PerformSceneTransaction {
    public:
        using Config = PerformSceneTransactionConfig;
        using JobModel = Job::Model<PerformSceneTransaction, Config>;

        void configure(const Config& config);
        void run(const RenderContextPointer& renderContext);
    protected:
    };


}

#endif // hifi_render_SceneTask_h
