//
//  Task.h
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Task_h
#define hifi_render_Task_h

#include "Scene.h"

namespace render {



class Task {
public:
    Task() {}
    ~Task() {}

    void run() {}

protected:
};

class DrawSceneTask : public Task {
public:

    DrawSceneTask() : Task() {}
    ~DrawSceneTask();

    void setup(RenderArgs* args);
    void run();
};




}

#endif // hifi_render_Task_h
