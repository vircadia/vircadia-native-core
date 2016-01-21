//
//  RenderScriptingInterface.h
//  libraries/render-utils
//
//  Created by Zach Pomerantz on 12/16/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderScriptingInterface_h
#define hifi_RenderScriptingInterface_h

#include <qscriptengine.h> // QObject
#include <DependencyManager.h> // Dependency

#include "render/Engine.h"

namespace RenderScripting {
    using State = render::RenderContext::ItemsConfig::State;
    using Counter = render::RenderContext::ItemsConfig::Counter;
        
    class ItemCounter : public QObject, public Counter {
        Q_OBJECT

    public:
        Q_PROPERTY(int numFeed READ getNumFeed)
        Q_PROPERTY(int numDrawn READ getNumDrawn)
        Q_PROPERTY(int maxDrawn MEMBER maxDrawn)

    protected:
        int getNumFeed() const { return numFeed; }
        int getNumDrawn() const { return numDrawn; }
    };
    using ItemCounterPointer = std::unique_ptr<ItemCounter>;

    class ItemState : public QObject, public State {
        Q_OBJECT

    public:
        Q_PROPERTY(bool render MEMBER render)
        Q_PROPERTY(bool cull MEMBER cull)
        Q_PROPERTY(bool sort MEMBER sort)

        Q_PROPERTY(int numFeed READ getNumFeed)
        Q_PROPERTY(int numDrawn READ getNumDrawn)
        Q_PROPERTY(int maxDrawn MEMBER maxDrawn)

    protected:
        int getNumFeed() const { return numFeed; }
        int getNumDrawn() const { return numDrawn; }
    };
    using ItemStatePointer = std::unique_ptr<ItemState>;

    class Tone : public QObject, public render::RenderContext::Tone {
        Q_OBJECT

    public:
        Q_PROPERTY(float exposure MEMBER exposure)
        Q_PROPERTY(QString curve READ getCurve WRITE setCurve)

        QString getCurve() const;
        int getCurveValue() const { return toneCurve; }
        void setCurve(const QString& curve);
    };
    using TonePointer = std::unique_ptr<Tone>;
    
    class AmbientOcclusion : public QObject, public render::RenderContext::AmbientOcclusion {
        Q_OBJECT
        
    public:
        Q_PROPERTY(int resolutionLevel MEMBER resolutionLevel)
        Q_PROPERTY(float radius MEMBER radius)
        Q_PROPERTY(float level MEMBER level)
        Q_PROPERTY(int numSamples MEMBER numSamples)
        Q_PROPERTY(float numSpiralTurns MEMBER numSpiralTurns)
        Q_PROPERTY(bool ditheringEnabled MEMBER ditheringEnabled)
        Q_PROPERTY(float falloffBias MEMBER falloffBias)
        Q_PROPERTY(float edgeSharpness MEMBER edgeSharpness)
        Q_PROPERTY(int blurRadius MEMBER blurRadius)
        Q_PROPERTY(float blurDeviation MEMBER blurDeviation)
        Q_PROPERTY(double gpuTime MEMBER gpuTime)
    };
    using AmbientOcclusionPointer = std::unique_ptr<AmbientOcclusion>;
};

class RenderScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
 public:
    Q_PROPERTY(RenderScripting::ItemState* opaque READ getOpaque)
    Q_PROPERTY(RenderScripting::ItemState* transparent READ getTransparent)
    Q_PROPERTY(RenderScripting::ItemCounter* overlay3D READ getOverlay3D)

    Q_PROPERTY(RenderScripting::Tone* tone READ getTone)
    Q_PROPERTY(RenderScripting::AmbientOcclusion* ambientOcclusion READ getAmbientOcclusion)
    
    Q_PROPERTY(int displayItemStatus MEMBER _drawStatus)
    Q_PROPERTY(bool displayHitEffect MEMBER _drawHitEffect)

    Q_PROPERTY(int deferredDebugMode MEMBER _deferredDebugMode)
    Q_PROPERTY(glm::vec4 deferredDebugSize MEMBER _deferredDebugSize)

    render::RenderContext getRenderContext();
    void setItemCounts(const render::RenderContext::ItemsConfig& items);

    // FIXME: It is ugly, we need a cleaner solution
    void setJobGPUTimes(double aoTime);

protected:
    RenderScriptingInterface();
    ~RenderScriptingInterface() {};

    RenderScripting::ItemState* getOpaque() const { return _opaque.get(); }
    RenderScripting::ItemState* getTransparent() const { return _transparent.get(); }
    RenderScripting::ItemCounter* getOverlay3D() const { return _overlay3D.get(); }

    RenderScripting::Tone* getTone() const { return _tone.get(); }
    RenderScripting::AmbientOcclusion* getAmbientOcclusion() const { return _ambientOcclusion.get(); }

    RenderScripting::ItemStatePointer _opaque = RenderScripting::ItemStatePointer{new RenderScripting::ItemState{}};
    RenderScripting::ItemStatePointer _transparent = RenderScripting::ItemStatePointer{new RenderScripting::ItemState{}};
    RenderScripting::ItemCounterPointer _overlay3D = RenderScripting::ItemCounterPointer{new RenderScripting::ItemCounter{}};

    RenderScripting::TonePointer _tone = RenderScripting::TonePointer{ new RenderScripting::Tone{} };
    
    RenderScripting::AmbientOcclusionPointer _ambientOcclusion = RenderScripting::AmbientOcclusionPointer{ new RenderScripting::AmbientOcclusion{} };

    // Options
    int _drawStatus = 0;
    bool _drawHitEffect = false;

    // Debugging
    int _deferredDebugMode = -1;
    glm::vec4 _deferredDebugSize { 0.0f, -1.0f, 1.0f, 1.0f };
};

#endif // hifi_RenderScriptingInterface_h
