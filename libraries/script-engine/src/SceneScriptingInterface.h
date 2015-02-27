//
//  SceneScriptingInterface.h
//  interface/src/scripting
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SceneScriptingInterface_h
#define hifi_SceneScriptingInterface_h

#include <qscriptengine.h>

#include <DependencyManager.h>

#include "model/Stage.h"

class SceneScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_INVOKABLE void setStageOrientation(const glm::quat& orientation);
    Q_INVOKABLE void setStageLocation(float longitude, float latitude, float altitude);
    Q_INVOKABLE void setStageDayTime(float hour);
    Q_INVOKABLE void setStageYearTime(int day);

    Q_INVOKABLE void setSunColor(const glm::vec3& color);
    Q_INVOKABLE void setSunIntensity(float intensity);

    model::SunSkyStagePointer getSkyStage() const;

protected:
    SceneScriptingInterface() {};
    ~SceneScriptingInterface() {};

    model::SunSkyStagePointer _skyStage = model::SunSkyStagePointer(new model::SunSkyStage());
};

#endif // hifi_SceneScriptingInterface_h
