//
//  Stage.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 2/24/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Stage_h
#define hifi_model_Stage_h

#include <gpu/Pipeline.h>

#include "Light.h"
#include "Skybox.h"

namespace model {

typedef glm::dvec3 Vec3d;
typedef glm::dvec4 Vec4d;
typedef glm::dmat4 Mat4d;
typedef glm::mat4 Mat4;

class EarthSunModel {
public:

    void setScale(float scale);
    float getScale() const { return _scale; }

    void setLatitude(float lat);
    float getLatitude() const  { return _latitude; }
    void setLongitude(float lon);
    float getLongitude() const { return _longitude; }
    void setAltitude(float altitude);
    float getAltitude() const  { return _altitude; }


    void setSurfaceOrientation(const Quat& orientation);
    const Quat& getSurfaceOrientation() const { valid(); return _surfaceOrientation; }
 
    const Vec3d& getSurfacePos() const { valid(); return _surfacePos; }

    const Mat4d& getSurfaceToWorldMat() const { valid(); return _surfaceToWorldMat; }
    const Mat4d& getWoldToSurfaceMat() const { valid(); return _worldToSurfaceMat; }

    const Mat4d& getEyeToSurfaceMat() const { valid(); return _eyeToSurfaceMat; }
    const Mat4d& getSurfaceToEyeMat() const { valid(); return _surfaceToEyeMat; }

    const Mat4d& getEyeToWorldMat() const { valid(); return _eyeToWorldMat; }
    const Mat4d& getWorldToEyeMat() const { valid(); return _worldToEyeMat; }


    //or set the surfaceToEye mat directly
    void setEyeToSurfaceMat( const Mat4d& e2s);

    const Vec3d& getEyePos() const { valid(); return _eyePos; }
    const Vec3d& getEyeDir() const { valid(); return _eyeDir; }

    void setSunLongitude(float lon);
    float getSunLongitude() const { return _sunLongitude; }

    void setSunLatitude(float lat);
    float getSunLatitude() const { return _sunLatitude; }

    const Vec3d& getWorldSunDir() const { valid(); return _sunDir; }
    const Vec3d& getSurfaceSunDir() const { valid(); return _surfaceSunDir; }


    EarthSunModel() { valid(); }

protected:
    float  _scale = 1000.0f; //Km
    float  _earthRadius = 6360.0;

    Quat    _surfaceOrientation;

    float  _longitude = 0.0f;
    float  _latitude = 0.0f;
    float  _altitude = 0.01f;
    mutable Vec3d _surfacePos;
    mutable Mat4d _worldToSurfaceMat;
    mutable Mat4d _surfaceToWorldMat;
    void updateWorldToSurface() const;
 
    mutable Mat4d _surfaceToEyeMat;
    mutable Mat4d _eyeToSurfaceMat;
    mutable Vec3d _eyeDir;
    mutable Vec3d _eyePos;
    void updateSurfaceToEye() const;
            
    mutable Mat4d _worldToEyeMat;
    mutable Mat4d _eyeToWorldMat;

    float _sunLongitude = 0.0f;
    float _sunLatitude = 0.0f;
    mutable Vec3d _sunDir;
    mutable Vec3d _surfaceSunDir;
    void updateSun() const;

    mutable bool _invalid = true;
    void invalidate() const { _invalid = true; }
    void valid() const { if (_invalid) { updateAll(); _invalid = false; } }
    void updateAll() const;

    static Mat4d evalWorldToGeoLocationMat(double longitude, double latitude, double altitude, double scale);
};

// Sun sky stage generates the rendering primitives to display a scene realistically
// at the specified location and time around earth
class SunSkyStage {
public:

    SunSkyStage();
    ~SunSkyStage();

    // time of the day (local to the position) expressed in decimal hour in the range [0.0, 24.0]
    void setDayTime(float hour);
    float getDayTime() const { return _dayTime; }

    // time of the year expressed in day in the range [0, 365]
    void setYearTime(unsigned int day);
    unsigned int getYearTime() const { return _yearTime; }

    // Origin orientation used to modify the cardinal axis alignement used.
    // THe default is north along +Z axis and west along +X axis. this orientation gets added
    // to the transform stack producing the sun light direction.
    void setOriginOrientation(const Quat& orientation);
    const Quat& getOriginOrientation() const { return _earthSunModel.getSurfaceOrientation(); }

    // Location  used to define the sun & sky is a longitude and latitude [rad] and a earth surface altitude [km]
    void setOriginLatitude(float latitude);
    void setOriginLongitude(float longitude);
    void setOriginSurfaceAltitude(float surfaceAltitude);
    void setOriginLocation(float longitude, float latitude, float surfaceAltitude);
    float getOriginLatitude() const { return _earthSunModel.getLatitude(); }
    float getOriginLongitude() const { return _earthSunModel.getLongitude(); }
    float getOriginSurfaceAltitude() const { return _earthSunModel.getAltitude(); }

    // Enable / disable the effect of the time and location on the sun direction and color
    void setSunModelEnable(bool isEnabled);
    bool isSunModelEnabled() const { return _sunModelEnable; }

    // Sun properties
    void setSunColor(const Vec3& color) { _sunLight->setColor(color); }
    const Vec3& getSunColor() const { return getSunLight()->getColor(); }
    void setSunIntensity(float intensity) { _sunLight->setIntensity(intensity); }
    float getSunIntensity() const { return getSunLight()->getIntensity(); }
    void setSunAmbientIntensity(float intensity) { _sunLight->setAmbientIntensity(intensity); }
    float getSunAmbientIntensity() const { return getSunLight()->getAmbientIntensity(); }
    void setSunAmbientSphere(const gpu::SHPointer& sphere);
    void setSunAmbientMap(const gpu::TexturePointer& map);

    // The sun direction is expressed in the world space
    void setSunDirection(const Vec3& direction);
    const Vec3& getSunDirection() const { return getSunLight()->getDirection(); }

    LightPointer getSunLight() const { valid(); return _sunLight;  }
 
    enum BackgroundMode {
        NO_BACKGROUND = 0,
        SKY_DEFAULT,
        SKY_BOX,
        SKY_DEFAULT_AMBIENT_TEXTURE,
        SKY_DEFAULT_TEXTURE,

        NUM_BACKGROUND_MODES,
    };
    void setBackgroundMode(BackgroundMode mode);
    BackgroundMode getBackgroundMode() const { return _backgroundMode; }

    // Skybox
    void setSkybox(const SkyboxPointer& skybox);
    const SkyboxPointer& getSkybox() const { valid(); return _skybox; }

protected:
    BackgroundMode _backgroundMode = SKY_DEFAULT;

    LightPointer _sunLight;
    mutable SkyboxPointer _skybox;

    float _dayTime = 12.0f;
    int _yearTime = 0;
    mutable EarthSunModel _earthSunModel;
    bool _sunModelEnable = true;
 
    mutable bool _invalid = true;
    void invalidate() const { _invalid = true; }
    void valid() const { if (_invalid) { updateGraphicsObject(); _invalid = false; } }
    void updateGraphicsObject() const;
};

typedef std::shared_ptr< SunSkyStage > SunSkyStagePointer;

};

#endif
