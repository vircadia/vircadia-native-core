//
//  Engine.h
//  render/src/render
//
//  Created by Sam Gateau on 3/3/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Engine_h
#define hifi_render_Engine_h

#include "Context.h"
#include "Task.h"

namespace render {

// The root of the tasks, the Engine, should not be known from the Tasks,
// The SceneContext is what navigates from the engine down to the Tasks
class Engine {
public:

    Engine();
    ~Engine() {}

    // Register the scene should be [art of the init phase before running the engine
    void registerScene(const ScenePointer& scene);

    // Push a RenderContext
    void setRenderContext(const RenderContext& renderContext);
    RenderContextPointer getRenderContext() const { return _renderContext; }

    void addTask(const TaskPointer& task);
    const Tasks& getTasks() const { return _tasks; }


    void run();

protected:
    Tasks _tasks;

    SceneContextPointer _sceneContext;
    RenderContextPointer _renderContext;
};
typedef std::shared_ptr<Engine> EnginePointer;

}

#endif // hifi_render_Engine_h

