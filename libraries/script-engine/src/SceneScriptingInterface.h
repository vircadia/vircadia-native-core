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

    // Enable/disable the stage sun model which uses the key light to simulate
    // the sun light based on the location of the stage trelative to earth and the current time 
    Q_INVOKABLE void setStageSunModelEnable(bool isEnabled);
    Q_INVOKABLE bool isStageSunModelEnabled() const;


    Q_INVOKABLE void setKeyLightColor(const glm::vec3& color);
    Q_INVOKABLE glm::vec3 getKeyLightColor() const;
    Q_INVOKABLE void setKeyLightIntensity(float intensity);
    Q_INVOKABLE float getKeyLightIntensity() const;
    Q_INVOKABLE void setKeyLightAmbientIntensity(float intensity);
    Q_INVOKABLE float getKeyLightAmbientIntensity() const;

    // setKeyLightDIrection is only effective if stage Sun model is disabled
    Q_INVOKABLE void setKeyLightDirection(const glm::vec3& direction);

    Q_INVOKABLE glm::vec3 getKeyLightDirection() const;




    model::SunSkyStagePointer getSkyStage() const;
    
    Q_INVOKABLE void setShouldRenderAvatars(bool shouldRenderAvatars);
    Q_INVOKABLE bool shouldRenderAvatars() const { return _shouldRenderAvatars; }
    
    Q_INVOKABLE void setShouldRenderEntities(bool shouldRenderEntities);
    Q_INVOKABLE bool shouldRenderEntities() const { return _shouldRenderEntities; }
    
signals:
    void shouldRenderAvatarsChanged(bool shouldRenderAvatars);
    void shouldRenderEntitiesChanged(bool shouldRenderEntities);
protected:
    SceneScriptingInterface() {};
    ~SceneScriptingInterface() {};

    model::SunSkyStagePointer _skyStage = model::SunSkyStagePointer(new model::SunSkyStage());
    
    bool _shouldRenderAvatars = true;
    bool _shouldRenderEntities = true;
};

#endif // hifi_SceneScriptingInterface_h
