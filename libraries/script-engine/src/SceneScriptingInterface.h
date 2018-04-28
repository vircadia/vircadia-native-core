//
//  SceneScriptingInterface.h
//  libraries/script-engine
//
//  Created by Sam Gateau on 2/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SceneScriptingInterface_h
#define hifi_SceneScriptingInterface_h

#include <qscriptengine.h> // QObject
#include <DependencyManager.h> // Dependency

#include "graphics/Stage.h"

// TODO: if QT moc ever supports nested classes, subclass these to the interface instead of namespacing
namespace SceneScripting {
    
    /**jsdoc
     * @typedef Scene.Stage.Location
     * @property {number} longitude
     * @property {number} latitude
     * @property {number} altitude
     */
    class Location : public QObject {
        Q_OBJECT

    public:
        Location(graphics::SunSkyStagePointer skyStage) : _skyStage{ skyStage } {}

        Q_PROPERTY(float longitude READ getLongitude WRITE setLongitude)
        Q_PROPERTY(float latitude READ getLatitude WRITE setLatitude)
        Q_PROPERTY(float altitude READ getAltitude WRITE setAltitude)

        float getLongitude() const;
        float getLatitude() const;
        float getAltitude() const;
        void setLongitude(float longitude);
        void setLatitude(float latitude);
        void setAltitude(float altitude);

    protected:
        graphics::SunSkyStagePointer _skyStage;
    };
    using LocationPointer = std::unique_ptr<Location>;
    
    /**jsdoc
     * @typedef Scene.Stage.Time
     * @property {number} hour
     * @property {number} day
     */
    class Time : public QObject {
        Q_OBJECT

    public:
        Time(graphics::SunSkyStagePointer skyStage) : _skyStage{ skyStage } {}

        Q_PROPERTY(float hour READ getHour WRITE setHour)
        Q_PROPERTY(int day READ getDay WRITE setDay)

        float getHour() const;
        void setHour(float hour);
        int getDay() const;
        void setDay(int day);

    protected:
        graphics::SunSkyStagePointer _skyStage;
    };
    using TimePointer = std::unique_ptr<Time>;

    /**jsdoc
     * @typedef Scene.Stage.KeyLight
     * @property {Vec3} color
     * @property {number} intensity
     * @property {number} ambientIntensity
     * @property {Vec3} direction
     */
    class KeyLight : public QObject {
        Q_OBJECT

    public:
        KeyLight(graphics::SunSkyStagePointer skyStage) : _skyStage{ skyStage } {}

        Q_PROPERTY(glm::vec3 color READ getColor WRITE setColor)
        Q_PROPERTY(float intensity READ getIntensity WRITE setIntensity)
        Q_PROPERTY(float ambientIntensity READ getAmbientIntensity WRITE setAmbientIntensity)
        Q_PROPERTY(glm::vec3 direction READ getDirection WRITE setDirection)

        glm::vec3 getColor() const;
        void setColor(const glm::vec3& color);
        float getIntensity() const;
        void setIntensity(float intensity);
        float getAmbientIntensity() const;
        void setAmbientIntensity(float intensity);
        glm::vec3 getDirection() const;
        // setDirection is only effective if stage Sun model is disabled
        void setDirection(const glm::vec3& direction);

        // AmbientTexture is unscriptable - it must be set through the zone entity
        void setAmbientSphere(const gpu::SHPointer& sphere);
        void resetAmbientSphere() { setAmbientSphere(nullptr); }
        void setAmbientMap(const gpu::TexturePointer& map);

    protected:
        graphics::SunSkyStagePointer _skyStage;
    };
    using KeyLightPointer = std::unique_ptr<KeyLight>;
    
    /**jsdoc
     * @class Scene.Stage
     *
     * @hifi-interface
     * @hifi-client-entity
     *
     * @property {string} backgroundMode
     * @property {Scene.Stage.KeyLight} keyLight
     * @property {Scene.Stage.Location} location
     * @property {boolean} sunModel
     * @property {Scene.Stage.Time} time
     */
    class Stage : public QObject {
        Q_OBJECT

    public:
        Stage(graphics::SunSkyStagePointer skyStage)
            : _skyStage{ skyStage },
            _location{ new Location{ skyStage } }, _time{ new Time{ skyStage } }, _keyLight{ new KeyLight{ skyStage } }{}

        /**jsdoc
         * @function Scene.Stage.setOrientation
         * @param {Quat} orientation
         */
        Q_INVOKABLE void setOrientation(const glm::quat& orientation) const;

        Q_PROPERTY(Location* location READ getLocation)

        Location* getLocation() const { return _location.get(); }

        /**jsdoc
         * @function Scene.Stage.setLocation
         * @param {number} longitude
         * @param {number} latitude
         * @param {number} altitude
         */
        Q_INVOKABLE void setLocation(float longitude, float latitude, float altitude);

        Q_PROPERTY(Time* time READ getTime)
        Time* getTime() const { return _time.get(); }

        Q_PROPERTY(KeyLight* keyLight READ getKeyLight)
        KeyLight* getKeyLight() const { return _keyLight.get(); }

        // Enable/disable the stage sun model which uses the key light to simulate
        // the sun light based on the location of the stage relative to earth and the current time 
        Q_PROPERTY(bool sunModel READ isSunModelEnabled WRITE setSunModelEnable)
        void setSunModelEnable(bool isEnabled);
        bool isSunModelEnabled() const;

        Q_PROPERTY(QString backgroundMode READ getBackgroundMode WRITE setBackgroundMode)
        void setBackgroundMode(const QString& mode);
        QString getBackgroundMode() const;

    protected:
        graphics::SunSkyStagePointer _skyStage;
        LocationPointer _location;
        TimePointer _time;
        KeyLightPointer _keyLight;
    };
    using StagePointer = std::unique_ptr<Stage>;
};

/**jsdoc
 * @namespace Scene
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {boolean} shouldRenderAvatars
 * @property {boolean} shouldRenderEntities
 * @property {Scene.Stage} stage
 */
class SceneScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    Q_PROPERTY(bool shouldRenderAvatars READ shouldRenderAvatars WRITE setShouldRenderAvatars)
    Q_PROPERTY(bool shouldRenderEntities READ shouldRenderEntities WRITE setShouldRenderEntities)
    bool shouldRenderAvatars() const { return _shouldRenderAvatars; }
    bool shouldRenderEntities() const { return _shouldRenderEntities; }
    void setShouldRenderAvatars(bool shouldRenderAvatars);
    void setShouldRenderEntities(bool shouldRenderEntities);

    Q_PROPERTY(SceneScripting::Stage* stage READ getStage)
    SceneScripting::Stage* getStage() const { return _stage.get(); }
 
    graphics::SunSkyStagePointer getSkyStage() const;

signals:

    /**jsdoc
     * @function Scene.shouldRenderAvatarsChanged
     * @param {boolean} shouldRenderAvatars
     * @returns {Signal}
     */
    void shouldRenderAvatarsChanged(bool shouldRenderAvatars);

    /**jsdoc
     * @function Scene.shouldRenderEntitiesChanged
     * @param {boolean} shouldRenderEntities
     * @returns {Signal}
     */
    void shouldRenderEntitiesChanged(bool shouldRenderEntities);

protected:
    SceneScriptingInterface();
    ~SceneScriptingInterface() {};

    graphics::SunSkyStagePointer _skyStage = std::make_shared<graphics::SunSkyStage>();
    SceneScripting::StagePointer _stage;

    bool _shouldRenderAvatars = true;
    bool _shouldRenderEntities = true;
};

#endif // hifi_SceneScriptingInterface_h 
