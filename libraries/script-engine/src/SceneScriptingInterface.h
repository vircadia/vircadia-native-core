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
    
    Q_PROPERTY(bool shouldRenderAvatars READ shouldRenderAvatars WRITE setShouldRenderAvatars)
    Q_PROPERTY(bool shouldRenderEntities READ shouldRenderEntities WRITE setShouldRenderEntities)

public:
    Q_INVOKABLE void setStageOrientation(const glm::quat& orientation);

    Q_INVOKABLE void setStageLocation(float longitude, float latitude, float altitude);
    Q_INVOKABLE float getStageLocationLongitude() const;
    Q_INVOKABLE float getStageLocationLatitude() const;
    Q_INVOKABLE float getStageLocationAltitude() const;

    Q_INVOKABLE void setStageDayTime(float hour);
    Q_INVOKABLE float getStageDayTime() const;
    Q_INVOKABLE void setStageYearTime(int day);
    Q_INVOKABLE int getStageYearTime() const;

    Q_INVOKABLE void setSunColor(const glm::vec3& color);
    Q_INVOKABLE const glm::vec3& getSunColor() const;
    Q_INVOKABLE void setSunIntensity(float intensity);
    Q_INVOKABLE float getSunIntensity() const;

    model::SunSkyStagePointer getSkyStage() const;
    
    Q_INVOKABLE void setShouldRenderAvatars(bool shouldRenderAvatars) { _shouldRenderAvatars = shouldRenderAvatars; }
    Q_INVOKABLE bool shouldRenderAvatars() const { return _shouldRenderAvatars; }
    
    Q_INVOKABLE void setShouldRenderEntities(bool shouldRenderEntities) { _shouldRenderEntities = shouldRenderEntities; }
    Q_INVOKABLE bool shouldRenderEntities() const { return _shouldRenderEntities; }

protected:
    SceneScriptingInterface() {};
    ~SceneScriptingInterface() {};

    model::SunSkyStagePointer _skyStage = model::SunSkyStagePointer(new model::SunSkyStage());
    
    bool _shouldRenderAvatars = true;
    bool _shouldRenderEntities = true;
};

#endif // hifi_SceneScriptingInterface_h
