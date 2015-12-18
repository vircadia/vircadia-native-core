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
using SceneContextPointer = std::shared_ptr<SceneContext>;

// see examples/utilities/tools/renderEngineDebug.js
const int showDisplayStatusFlag = 1;
const int showNetworkStatusFlag = 2;


class RenderContext {
public:
    class ItemsConfig {
    public:
        inline void setCounts(const ItemsConfig& items) {
            opaque.setCounts(items.opaque);
            transparent.setCounts(items.transparent);
            overlay3D.setCounts(items.overlay3D);
        };

        class Counter {
        public:
            Counter() {};
            Counter(const Counter& counter) {
                numFeed = numDrawn = 0;
                maxDrawn = counter.maxDrawn;
            };

            inline void setCounts(const Counter& counter) {
                numFeed = counter.numFeed;
                numDrawn = counter.numDrawn;
            };

            int numFeed = 0;
            int numDrawn = 0;
            int maxDrawn = -1;
        };

        class State : public Counter {
        public:
            bool render = true;
            bool cull = true;
            bool sort = true;

            Counter counter{};
        };

        // TODO: If member count increases, store counters in a map instead of multiple members
        State opaque{};
        State transparent{};
        Counter overlay3D{};
    };

    class Tone {
    public:
        int toneCurve = 3;
        float exposure = 0.0;
    };
    
    RenderContext(RenderArgs* args, ItemsConfig items, Tone tone) : _args{args}, _items{items}, _tone{tone} {};
    RenderContext() : RenderContext(nullptr, {}, {}) {};

    inline RenderArgs* getArgs() { return _args; }
    inline ItemsConfig& getItemsConfig() { return _items; }
    inline Tone& getTone() { return _tone; }
    inline int getDrawStatus() { return _drawStatus; }
    inline bool getDrawHitEffect() { return _drawHitEffect; }
    inline bool getOcclusionStatus() { return _occlusionStatus; }
    inline bool getFxaaStatus() { return _fxaaStatus; }
    void setOptions(int drawStatus, bool drawHitEffect, bool occlusion, bool fxaa, bool showOwned);

    // Debugging
    int _deferredDebugMode = -1;
    glm::vec4 _deferredDebugSize { 0.0f, -1.0f, 1.0f, 1.0f };

protected:
    RenderArgs* _args;

    // Options
    int _drawStatus = 0; // bitflag
    bool _drawHitEffect = false;
    bool _occlusionStatus = false;
    bool _fxaaStatus = false;

    ItemsConfig _items;
    Tone _tone;
};
typedef std::shared_ptr<RenderContext> RenderContextPointer;

// The base class for a task that runs on the SceneContext
class Task {
public:
    Task() {}
    ~Task() {}

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {}

protected:
};
typedef std::shared_ptr<Task> TaskPointer;
typedef std::vector<TaskPointer> Tasks;

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
