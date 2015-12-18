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

class RenderScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
 public:
     Q_INVOKABLE void setEngineRenderOpaque(bool renderOpaque) { _items._opaque._render = renderOpaque; };
    Q_INVOKABLE bool doEngineRenderOpaque() const { return _items._opaque._render; }
    Q_INVOKABLE void setEngineRenderTransparent(bool renderTransparent) { _items._transparent._render = renderTransparent; };
    Q_INVOKABLE bool doEngineRenderTransparent() const { return _items._transparent._render; }
    
    Q_INVOKABLE void setEngineCullOpaque(bool cullOpaque) { _items._opaque._cull = cullOpaque; };
    Q_INVOKABLE bool doEngineCullOpaque() const { return _items._opaque._cull; }
    Q_INVOKABLE void setEngineCullTransparent(bool cullTransparent) { _items._transparent._cull = cullTransparent; };
    Q_INVOKABLE bool doEngineCullTransparent() const { return _items._transparent._cull; }

    Q_INVOKABLE void setEngineSortOpaque(bool sortOpaque) { _items._opaque._sort = sortOpaque; };
    Q_INVOKABLE bool doEngineSortOpaque() const { return _items._opaque._sort; }
    Q_INVOKABLE void setEngineSortTransparent(bool sortTransparent) { _items._transparent._sort = sortTransparent; };
    Q_INVOKABLE bool doEngineSortTransparent() const { return _items._transparent._sort; }

    Q_INVOKABLE int getEngineNumDrawnOpaqueItems() { return _items._opaque._numDrawn; }
    Q_INVOKABLE int getEngineNumDrawnTransparentItems() { return _items._transparent._numDrawn; }
    Q_INVOKABLE int getEngineNumDrawnOverlay3DItems() { return _items._overlay3D._numDrawn; }

    Q_INVOKABLE int getEngineNumFeedOpaqueItems() { return _items._opaque._numFeed; }
    Q_INVOKABLE int getEngineNumFeedTransparentItems() { return _items._transparent._numFeed; }
    Q_INVOKABLE int getEngineNumFeedOverlay3DItems() { return _items._overlay3D._numFeed; }

    Q_INVOKABLE void setEngineMaxDrawnOpaqueItems(int count) { _items._opaque._maxDrawn = count; }
    Q_INVOKABLE int getEngineMaxDrawnOpaqueItems() { return _items._opaque._maxDrawn; }
    Q_INVOKABLE void setEngineMaxDrawnTransparentItems(int count) { _items._transparent._maxDrawn = count; }
    Q_INVOKABLE int getEngineMaxDrawnTransparentItems() { return _items._transparent._maxDrawn; }
    Q_INVOKABLE void setEngineMaxDrawnOverlay3DItems(int count) { _items._overlay3D._maxDrawn = count; }
    Q_INVOKABLE int getEngineMaxDrawnOverlay3DItems() { return _items._overlay3D._maxDrawn; }
    
    Q_INVOKABLE void setEngineDeferredDebugMode(int mode) { _deferredDebugMode = mode; }
    Q_INVOKABLE int getEngineDeferredDebugMode() { return _deferredDebugMode; }
    Q_INVOKABLE void setEngineDeferredDebugSize(glm::vec4 size) { _deferredDebugSize = size; }
    Q_INVOKABLE glm::vec4 getEngineDeferredDebugSize() { return _deferredDebugSize; }
    
    Q_INVOKABLE void setEngineDisplayItemStatus(int display) { _drawStatus = display; }
    Q_INVOKABLE int doEngineDisplayItemStatus() { return _drawStatus; }

    Q_INVOKABLE void setEngineDisplayHitEffect(bool display) { _drawHitEffect = display; }
    Q_INVOKABLE bool doEngineDisplayHitEffect() { return _drawHitEffect; }

    Q_INVOKABLE void setEngineToneMappingExposure(float exposure) { _tone._exposure = exposure; }
    Q_INVOKABLE float getEngineToneMappingExposure() const { return _tone._exposure; }

    Q_INVOKABLE void setEngineToneMappingToneCurve(const QString& curve);
    Q_INVOKABLE QString getEngineToneMappingToneCurve() const;
    int getEngineToneMappingToneCurveValue() const { return _tone._toneCurve; }

    inline int getDrawStatus() { return _drawStatus; }
    inline bool getDrawHitEffect() { return _drawHitEffect; }
    inline const render::RenderContext::ItemsMeta& getItemsMeta() { return _items; }
    inline const render::RenderContext::Tone& getTone() { return _tone;  }
    void setItemCounts(const render::RenderContext::ItemsMeta& items) { _items.setCounts(items); };

protected:
    RenderScriptingInterface();
    ~RenderScriptingInterface() {};

    render::RenderContext::ItemsMeta _items;
    render::RenderContext::Tone _tone;

    // Options
    int _drawStatus = 0;
    bool _drawHitEffect = false;

    // Debugging
    int _deferredDebugMode = -1;
    glm::vec4 _deferredDebugSize { 0.0f, -1.0f, 1.0f, 1.0f };
};

#endif // hifi_RenderScriptingInterface_h
