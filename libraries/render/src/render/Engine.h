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

#include "Scene.h"

namespace render {


class SceneContext {
public:
    ScenePointer _scene;
  
    SceneContext() {}
};
typedef std::shared_ptr<SceneContext> SceneContextPointer;

// see examples/utilities/tools/renderEngineDebug.js
const int showDisplayStatusFlag = 1;
const int showNetworkStatusFlag = 2;


class RenderContext {
public:
    RenderArgs* args;

    bool _cullOpaque = true;
    bool _sortOpaque = true;
    bool _renderOpaque = true;
    bool _cullTransparent = true;
    bool _sortTransparent = true;
    bool _renderTransparent = true;

    int _numFeedOpaqueItems = 0;
    int _numDrawnOpaqueItems = 0;
    int _maxDrawnOpaqueItems = -1;
    
    int _numFeedTransparentItems = 0;
    int _numDrawnTransparentItems = 0;
    int _maxDrawnTransparentItems = -1;

    int _numFeedOverlay3DItems = 0;
    int _numDrawnOverlay3DItems = 0;
    int _maxDrawnOverlay3DItems = -1;

    int _drawItemStatus = 0;
    bool _drawHitEffect = false;

    bool _occlusionStatus = false;
    bool _fxaaStatus = false;

    RenderContext() {}
};
typedef std::shared_ptr<RenderContext> RenderContextPointer;

// THe base class for a task that runs on the SceneContext
class Task {
public:
    Task() {}
    ~Task() {}

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {}

protected:
};
typedef std::shared_ptr<Task> TaskPointer;
typedef std::vector<TaskPointer> Tasks;

// The root of the takss, the Engine, should not be known from the Tasks,
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

    // standard pipeline of tasks
    void buildStandardTaskPipeline();

protected:

    Tasks _tasks;

    SceneContextPointer _sceneContext;
    RenderContextPointer _renderContext;
};
typedef std::shared_ptr<Engine> EnginePointer;

}

#endif // hifi_render_Engine_h
