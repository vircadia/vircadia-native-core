//
//  Stage.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 2/24/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Stage.h"

#include <glm/gtx/transform.hpp> 
#include <math.h>
#include <qcompilerdetection.h>

using namespace model;


void EarthSunModel::updateAll() const {
    updateWorldToSurface();
    updateSurfaceToEye();
    updateSun();
}

Mat4d EarthSunModel::evalWorldToGeoLocationMat(double longitude, double latitude, double absAltitude, double scale) {
    // Longitude is along Z axis but - from east to west
    Mat4d rotLon = glm::rotate(glm::radians(longitude), Vec3d(0.0, 0.0, 1.0));
     
    // latitude is along X axis + from south to north
    Mat4d rotLat = glm::rotate(-glm::radians(latitude), Vec3d(1.0, 0.0, 0.0));

    // translation is movin to the earth surface + altiture at the radius along Y axis
    Mat4d surfaceT = glm::translate(Vec3d(0.0, -absAltitude, 0.0));

  //  Mat4d worldScale = glm::scale(Vec3d(scale));

    Mat4d worldToGeoLocMat = surfaceT * rotLat * rotLon;

    return worldToGeoLocMat;
}

void EarthSunModel::updateWorldToSurface() const {
    // Check if the final position is too close to the earth center ?
    float absAltitude = _earthRadius + _altitude;
    if ( absAltitude < 0.01f) {
        absAltitude = 0.01f;
    }

    // Final world to local Frame
    _worldToSurfaceMat = evalWorldToGeoLocationMat(_longitude, _latitude, absAltitude, _scale);
    // and the inverse
    _surfaceToWorldMat = glm::inverse(_worldToSurfaceMat);

    _surfacePos = Vec3d(_surfaceToWorldMat * Vec4d(0.0, 0.0, 0.0, 1.0));
}

void EarthSunModel::updateSurfaceToEye() const {
    _surfaceToEyeMat = glm::inverse(_eyeToSurfaceMat);
    _worldToEyeMat = _surfaceToEyeMat * _worldToSurfaceMat;
    _eyeToWorldMat = _surfaceToWorldMat * _eyeToSurfaceMat;
    _eyePos = Vec3d(_eyeToWorldMat * Vec4d(0.0, 0.0, 0.0, 1.0) );
    _eyeDir = Vec3d(_eyeToWorldMat * Vec4d(0.0, 0.0, -1.0, 0.0) );
}

void EarthSunModel::updateSun() const {
    // Longitude is along Y axis but - from east to west
    Mat4d rotSunLon;

    Mat4d rotSun = evalWorldToGeoLocationMat(_sunLongitude, _sunLatitude, _earthRadius, _scale);
    rotSun = glm::inverse(rotSun);

    _sunDir = Vec3d(rotSun * Vec4d(0.0, 1.0, 0.0, 0.0)); 

    // sun direction is looking up toward Y axis at the specified sun lat, long
    Vec3d lssd = Vec3d(_worldToSurfaceMat * Vec4d(_sunDir.x, _sunDir.y, _sunDir.z, 0.0));
    
    // apply surface rotation offset
    glm::dquat dSurfOrient(_surfaceOrientation);
    lssd = glm::rotate(dSurfOrient, lssd);

    _surfaceSunDir = glm::normalize(Vec3(lssd.x, lssd.y, lssd.z));
}

void EarthSunModel::setSurfaceOrientation(const Quat& orientation) {
    _surfaceOrientation = orientation;
    invalidate();
}

double moduloRange(double val, double minVal, double maxVal) {
    double range = maxVal - minVal;
    double rval = (val - minVal) / range;
    rval =  rval - floor(rval);
    return rval * range + minVal;
}

const float MAX_LONGITUDE = 180.0f;
const float MAX_LATITUDE = 90.0f;

float validateLongitude(float lon) {
    return moduloRange(lon, -MAX_LONGITUDE, MAX_LONGITUDE);
}

float validateLatitude(float lat) {
    return moduloRange(lat, -MAX_LATITUDE, MAX_LATITUDE);
}

float validateAltitude(float altitude) {
    const float MIN_ALTITUDE = -1000.0f;
    const float MAX_ALTITUDE = 100000.0f;
    return std::min(std::max(altitude, MIN_ALTITUDE), MAX_ALTITUDE);
}

void EarthSunModel::setLatitude(float lat) {
    _latitude = validateLatitude(lat);
    invalidate();
}
void EarthSunModel::setLongitude(float lon) {
    _longitude = validateLongitude(lon);
    invalidate();
}
void EarthSunModel::setAltitude(float altitude) {
    _altitude = validateAltitude(altitude);
    invalidate();
}

void EarthSunModel::setSunLatitude(float lat) {
    _sunLatitude = validateLatitude(lat);
    invalidate();
}
void EarthSunModel::setSunLongitude(float lon) {
    _sunLongitude = validateLongitude(lon);
    invalidate();
}

const int NUM_DAYS_PER_YEAR = 365;
const float NUM_HOURS_PER_DAY = 24.0f;
const float NUM_HOURS_PER_HALF_DAY = NUM_HOURS_PER_DAY * 0.5f;

SunSkyStage::SunSkyStage() :
    _sunLight(std::make_shared<Light>()),
    _skybox(std::make_shared<Skybox>())
{
    _sunLight->setType(Light::SUN);
    // Default ambient sphere (for lack of skybox)
    _sunLight->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset::OLD_TOWN_SQUARE);
 
    setSunIntensity(1.0f);
    setSunAmbientIntensity(0.5f);
    setSunColor(Vec3(1.0f, 1.0f, 1.0f));

    // Default origin location is a special place in the world...
    setOriginLocation(122.407f, 37.777f, 0.03f);
    // Noun 
    setDayTime(12.0f);
    // Begining of march
    setYearTime(60.0f);

    _skybox->setColor(Color(1.0f, 0.0f, 0.0f));
}

SunSkyStage::~SunSkyStage() {
}

void SunSkyStage::setDayTime(float hour) {
    _dayTime = moduloRange(hour, 0.f, NUM_HOURS_PER_DAY);
    invalidate();
}

void SunSkyStage::setYearTime(unsigned int day) {
    _yearTime = day % NUM_DAYS_PER_YEAR;
    invalidate();
}

void SunSkyStage::setOriginOrientation(const Quat& orientation) {
    _earthSunModel.setSurfaceOrientation(orientation);
    invalidate();
}

void SunSkyStage::setOriginLongitude(float longitude) {
    _earthSunModel.setLongitude(longitude);
    invalidate();
}

void SunSkyStage::setOriginLatitude(float latitude) {
    _earthSunModel.setLatitude(latitude);
    invalidate();
}

void SunSkyStage::setOriginSurfaceAltitude(float altitude) {
    _earthSunModel.setAltitude(altitude);
    invalidate();
}

void SunSkyStage::setOriginLocation(float longitude, float latitude, float altitude) {
    _earthSunModel.setLongitude(longitude);
    _earthSunModel.setLatitude(latitude);
    _earthSunModel.setAltitude(altitude);
    invalidate();
}

void SunSkyStage::setSunModelEnable(bool isEnabled) {
    _sunModelEnable = isEnabled;
    invalidate();
}

void SunSkyStage::setSunAmbientSphere(const gpu::SHPointer& sphere) {
    if (sphere) {
        _sunLight->setAmbientSphere(*sphere);
    } else {
        const gpu::SphericalHarmonics::Preset DEFAULT_AMBIENT_SPHERE = gpu::SphericalHarmonics::OLD_TOWN_SQUARE;
        _sunLight->setAmbientSpherePreset(DEFAULT_AMBIENT_SPHERE);
    }
}
void SunSkyStage::setSunAmbientMap(const gpu::TexturePointer& map) {
    _sunLight->setAmbientMap(map);
}

void SunSkyStage::setSunDirection(const Vec3& direction) {
    if (!isSunModelEnabled()) {
        _sunLight->setDirection(direction);
    }
}

// THe sun declinaison calculus is taken from https://en.wikipedia.org/wiki/Position_of_the_Sun
double evalSunDeclinaison(double dayNumber) {
    return -(23.0 + 44.0/60.0)*cos(glm::radians((360.0/365.0)*(dayNumber + 10.0)));
}

void SunSkyStage::updateGraphicsObject() const {
    // Always update the sunLongitude based on the current dayTime and the current origin
    // The day time is supposed to be local at the origin
    float signedNormalizedDayTime = (_dayTime - NUM_HOURS_PER_HALF_DAY) / NUM_HOURS_PER_HALF_DAY;
    float sunLongitude = _earthSunModel.getLongitude() + (MAX_LONGITUDE * signedNormalizedDayTime);
    _earthSunModel.setSunLongitude(sunLongitude);

    // And update the sunLAtitude as the declinaison depending of the time of the year
    _earthSunModel.setSunLatitude(evalSunDeclinaison(_yearTime)); 

    if (isSunModelEnabled()) {
        Vec3d sunLightDir = -_earthSunModel.getSurfaceSunDir();
        _sunLight->setDirection(Vec3(sunLightDir.x, sunLightDir.y, sunLightDir.z));

        double originAlt = _earthSunModel.getAltitude();
        _sunLight->setPosition(Vec3(0.0f, originAlt, 0.0f));
    }
}

void SunSkyStage::setBackgroundMode(BackgroundMode mode) {
    _backgroundMode = mode;
    invalidate();
}

void SunSkyStage::setSkybox(const SkyboxPointer& skybox) {
    _skybox = skybox;
    invalidate();
}
