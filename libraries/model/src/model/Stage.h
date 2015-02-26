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

#include "Light.h"

namespace model {

typedef glm::dvec3 Vec3d;
typedef glm::dvec4 Vec4d;
typedef glm::dmat4 Mat4d;
typedef glm::mat4 Mat4;

class EarthSunModel {
public:
    enum Preset
    {
        Toulouse = 0,
        SanFrancisco,
        Sydney,
        Num_Presets,
    };
    static const std::string PresetNames[Num_Presets];
    //void assignPreset( Preset p);
    //Preset preset() const { return mPreset; }

    void setScale(float scale);
    float getScale() const { return _scale; }

    void setLatitude(float lat);
    float getLatitude() const  { return _latitude; }
    void setLongitude(float lon);
    float getLongitude() const { return _longitude; }
    void setAltitude(float altitude);
    float getAltitude() const  { return _altitude; }

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
    double  _scale = 1000.0; //Km
    double  _earthRadius = 6360.0;

    double  _longitude = 0.0;
    double  _latitude = 0.0;
    double  _altitude = 0.01;
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

    double _sunLongitude = 0.0;
    double _sunLatitude = 0.0;
    mutable Vec3d _sunDir;
    mutable Vec3d _surfaceSunDir;
    void updateSun() const;

    mutable bool _invalid = true;
    void invalidate() const { _invalid = true; }
    void valid() const { if (_invalid) { updateAll(); _invalid = false; } }
    void updateAll() const;

    static Mat4d evalWorldToGeoLocationMat(double longitude, double latitude, double altitude, double scale);
};

namespace gpu {
    class Batch;
};

class ProgramObject;

class Skybox {
public:
    void recordBatch(gpu::Batch& batch, const Transform& viewTransform, const Mat4& projection);

    Skybox();
    ~Skybox();
protected:
    ProgramObject* createSkyProgram(const char* from, int* locations);
    ProgramObject* _skyFromAtmosphereProgram;
    ProgramObject* _skyFromSpaceProgram;
    enum {
        CAMERA_POS_LOCATION,
        LIGHT_POS_LOCATION,
        INV_WAVELENGTH_LOCATION,
        CAMERA_HEIGHT2_LOCATION,
        OUTER_RADIUS_LOCATION,
        OUTER_RADIUS2_LOCATION,
        INNER_RADIUS_LOCATION,
        KR_ESUN_LOCATION,
        KM_ESUN_LOCATION,
        KR_4PI_LOCATION,
        KM_4PI_LOCATION,
        SCALE_LOCATION,
        SCALE_DEPTH_LOCATION,
        SCALE_OVER_SCALE_DEPTH_LOCATION,
        G_LOCATION,
        G2_LOCATION,
        LOCATION_COUNT
    };
    
    int _skyFromAtmosphereUniformLocations[LOCATION_COUNT];
    int _skyFromSpaceUniformLocations[LOCATION_COUNT];
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

    // Location  used to define the sun & sky is a longitude and latitude [rad] and a earth surface altitude [km]
    void setOriginLocation(float longitude, float latitude, float surfaceAltitude);
    float getOriginLatitude() const { return _earthSunModel.getLatitude(); }
    float getOriginLongitude() const { return _earthSunModel.getLongitude(); }
    float getOriginSurfaceAltitude() const { return _earthSunModel.getAltitude(); }

    // Sun properties
    void setSunColor(const Vec3& color);
    const Vec3& getSunColor() const { return getSunLight()->getColor(); }
    void setSunIntensity(float intensity);
    float getSunIntensity() const { return getSunLight()->getIntensity(); }

    LightPointer getSunLight() const { valid(); return _sunLight;  }
 
protected:
    LightPointer _sunLight;

    // default day is 1st of january at noun
    float _dayTime = 12.0f;
    int _yearTime = 0;

    mutable EarthSunModel _earthSunModel;
 
    mutable bool _invalid = true;
    void invalidate() const { _invalid = true; }
    void valid() const { if (_invalid) { updateGraphicsObject(); _invalid = false; } }
    void updateGraphicsObject() const;
};

typedef QSharedPointer< SunSkyStage > SunSkyStagePointer;

};

#endif
