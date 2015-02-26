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
    double absAltitude = _earthRadius + _altitude;
    if ( absAltitude < 0.01) {
        absAltitude = 0.01;
    }

    // Final world to local Frame
    _worldToSurfaceMat = evalWorldToGeoLocationMat(_longitude, _latitude, absAltitude, _scale);
    // and the inverse
    _surfaceToWorldMat = glm::inverse(_worldToSurfaceMat);

    _surfacePos = Vec3d(_surfaceToWorldMat * Vec4d(0.0, 0.0, 0.0, 1.0));
}

void EarthSunModel::updateSurfaceToEye() const
{

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
    _surfaceSunDir = glm::normalize(Vec3(lssd.x, lssd.y, lssd.z));
}


float moduloRange(float val, float minVal, float maxVal) {
    float range = maxVal - minVal;
    float rval = (val - minVal) / range;
    float intval;
    return modf(rval, &intval) * range + minVal;
}

void EarthSunModel::setLatitude(float lat) {
    _latitude = moduloRange(lat, -90.0f, 90.0f);
    invalidate();
}
void EarthSunModel::setLongitude(float lon) {
    _longitude = moduloRange(lon, -180.0f, 180.0f);
    invalidate();
}
void EarthSunModel::setAltitude(float altitude) {
    _altitude = moduloRange(altitude, -1000.f, 100000.f);
    invalidate();
}

void EarthSunModel::setSunLatitude(float lat) {
    _sunLatitude = moduloRange(lat, -90.0f, 90.0f);
    invalidate();
}
void EarthSunModel::setSunLongitude(float lon) {
    _sunLongitude = moduloRange(lon, -180.0f, 180.0f);
    invalidate();
}

const int NUM_DAYS_PER_YEAR = 365;
const float NUM_HOURS_PER_DAY = 24.0f;

SunSkyStage::SunSkyStage() :
    _sunLight(new Light())
{
    _sunLight->setType(Light::SUN);
 
    setSunIntensity(1.0f);
    setSunColor(Vec3(1.0f, 1.0f, 1.0f));

   // setOriginLocation(45.0f, 20.0f, 1.0f);
    setDayTime(18.0f);
    setYearTime(60.0f);
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

void SunSkyStage::setOriginLocation(float longitude, float latitude, float altitude) {
    _earthSunModel.setLongitude(longitude);
    _earthSunModel.setLatitude(latitude);
    _earthSunModel.setAltitude(altitude);
    invalidate();
}

void SunSkyStage::setSunColor(const Vec3& color) {
    _sunLight->setColor(color);
}
void SunSkyStage::setSunIntensity(float intensity) {
    _sunLight->setIntensity(intensity);
}

void SunSkyStage::updateGraphicsObject() const {
    // Always update the sunLongitude based on the current dayTIme and the current origin
    _earthSunModel.setSunLongitude(_earthSunModel.getLongitude() + (-180.0 + 360.0 * _dayTime / NUM_HOURS_PER_DAY));

    // And update the sunLAtitude as the declinaison depending of the time of the year
    _earthSunModel.setSunLatitude(-(23.0 + 44.0/60.0)*cos(glm::radians((360.0/365.0)*(_yearTime + 10)))); 

    Vec3d sunLightDir = -_earthSunModel.getSurfaceSunDir();

    _sunLight->setDirection(Vec3(sunLightDir.x, sunLightDir.y, sunLightDir.z));
}






//
//  Environment.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <QByteArray>
#include <QMutexLocker>
#include <QtDebug>

#include <GeometryUtil.h>
#include <PacketHeaders.h>
#include <PathUtils.h>
#include <ProgramObject.h>
#include <SharedUtil.h>

#include "Application.h"
#include "Camera.h"
#include "world.h"

#include "Environment.h"

uint qHash(const HifiSockAddr& sockAddr) {
    if (sockAddr.getAddress().isNull()) {
        return 0; // shouldn't happen, but if it does, zero is a perfectly valid hash
    }
    quint32 address = sockAddr.getAddress().toIPv4Address();
    return sockAddr.getPort() + qHash(QByteArray::fromRawData((char*) &address,
                                                              sizeof(address)));
}

Environment::Environment()
    : _initialized(false) {
}

Environment::~Environment() {
    if (_initialized) {
        delete _skyFromAtmosphereProgram;
        delete _skyFromSpaceProgram;
    }
}

void Environment::init() {
    if (_initialized) {
        qDebug("[ERROR] Environment is already initialized.");
        return;
    }

    _skyFromAtmosphereProgram = createSkyProgram("Atmosphere", _skyFromAtmosphereUniformLocations);
    _skyFromSpaceProgram = createSkyProgram("Space", _skyFromSpaceUniformLocations);
    
    // start off with a default-constructed environment data
    _data[HifiSockAddr()][0];

    _initialized = true;
}

void Environment::resetToDefault() {
    _data.clear();
    _data[HifiSockAddr()][0];
}

void Environment::renderAtmospheres(Camera& camera) {    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    foreach (const ServerData& serverData, _data) {
        // TODO: do something about EnvironmentData
        foreach (const EnvironmentData& environmentData, serverData) {
            renderAtmosphere(camera, environmentData);
        }
    }
}

glm::vec3 Environment::getGravity (const glm::vec3& position) {
    //
    // 'Default' gravity pulls you downward in Y when you are near the X/Z plane
    const glm::vec3 DEFAULT_GRAVITY(0.0f, -1.0f, 0.0f);
    glm::vec3 gravity(DEFAULT_GRAVITY);
    float DEFAULT_SURFACE_RADIUS = 30.0f;
    float gravityStrength;
    
    //  Weaken gravity with height
    if (position.y > 0.0f) {
        gravityStrength = 1.0f / powf((DEFAULT_SURFACE_RADIUS + position.y) / DEFAULT_SURFACE_RADIUS, 2.0f);
        gravity *= gravityStrength;
    }
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            glm::vec3 vector = environmentData.getAtmosphereCenter(position) - position;
            float surfaceRadius = environmentData.getAtmosphereInnerRadius();
            if (glm::length(vector) <= surfaceRadius) {
                //  At or inside a planet, gravity is as set for the planet
                gravity += glm::normalize(vector) * environmentData.getGravity();
            } else {
                //  Outside a planet, the gravity falls off with distance 
                gravityStrength = 1.0f / powf(glm::length(vector) / surfaceRadius, 2.0f);
                gravity += glm::normalize(vector) * environmentData.getGravity() * gravityStrength;
            }
        }
    }
    
    return gravity;
}

const EnvironmentData Environment::getClosestData(const glm::vec3& position) {
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    EnvironmentData closest;
    float closestDistance = FLT_MAX;
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            float distance = glm::distance(position, environmentData.getAtmosphereCenter(position)) -
                environmentData.getAtmosphereOuterRadius();
            if (distance < closestDistance) {
                closest = environmentData;
                closestDistance = distance;
            }
        }
    }
    return closest;
}

bool Environment::findCapsulePenetration(const glm::vec3& start, const glm::vec3& end,
                                         float radius, glm::vec3& penetration) {
    // collide with the "floor"
    bool found = findCapsulePlanePenetration(start, end, radius, glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), penetration);
    
    glm::vec3 middle = (start + end) * 0.5f;
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    foreach (const ServerData& serverData, _data) {
        foreach (const EnvironmentData& environmentData, serverData) {
            if (environmentData.getGravity() == 0.0f) {
                continue; // don't bother colliding with gravity-less environments
            }
            glm::vec3 environmentPenetration;
            if (findCapsuleSpherePenetration(start, end, radius, environmentData.getAtmosphereCenter(middle),
                    environmentData.getAtmosphereInnerRadius(), environmentPenetration)) {
                penetration = addPenetrations(penetration, environmentPenetration);
                found = true;
            }
        }
    }
    return found;
}

int Environment::parseData(const HifiSockAddr& senderAddress, const QByteArray& packet) {
    // push past the packet header
    int bytesRead = numBytesForPacketHeader(packet);

    // push past flags, sequence, timestamp
    bytesRead += sizeof(OCTREE_PACKET_FLAGS);
    bytesRead += sizeof(OCTREE_PACKET_SEQUENCE);
    bytesRead += sizeof(OCTREE_PACKET_SENT_TIME);
    
    // get the lock for the duration of the call
    QMutexLocker locker(&_mutex);
    
    EnvironmentData newData;
    while (bytesRead < packet.size()) {
        int dataLength = newData.parseData(reinterpret_cast<const unsigned char*>(packet.data()) + bytesRead,
                                           packet.size() - bytesRead);
        
        // update the mapping by address/ID
        _data[senderAddress][newData.getID()] = newData;
        
        bytesRead += dataLength;
    }
    
    // remove the default mapping, if any
    _data.remove(HifiSockAddr());
    
    return bytesRead;
}

ProgramObject* Environment::createSkyProgram(const char* from, int* locations) {
    ProgramObject* program = new ProgramObject();
    QByteArray prefix = QString(PathUtils::resourcesPath() + "/shaders/SkyFrom" + from).toUtf8();
    program->addShaderFromSourceFile(QGLShader::Vertex, prefix + ".vert");
    program->addShaderFromSourceFile(QGLShader::Fragment, prefix + ".frag");
    program->link();
    
    locations[CAMERA_POS_LOCATION] = program->uniformLocation("v3CameraPos");
    locations[LIGHT_POS_LOCATION] = program->uniformLocation("v3LightPos");
    locations[INV_WAVELENGTH_LOCATION] = program->uniformLocation("v3InvWavelength");
    locations[CAMERA_HEIGHT2_LOCATION] = program->uniformLocation("fCameraHeight2");
    locations[OUTER_RADIUS_LOCATION] = program->uniformLocation("fOuterRadius");
    locations[OUTER_RADIUS2_LOCATION] = program->uniformLocation("fOuterRadius2");
    locations[INNER_RADIUS_LOCATION] = program->uniformLocation("fInnerRadius");
    locations[KR_ESUN_LOCATION] = program->uniformLocation("fKrESun");
    locations[KM_ESUN_LOCATION] = program->uniformLocation("fKmESun");
    locations[KR_4PI_LOCATION] = program->uniformLocation("fKr4PI");
    locations[KM_4PI_LOCATION] = program->uniformLocation("fKm4PI");
    locations[SCALE_LOCATION] = program->uniformLocation("fScale");
    locations[SCALE_DEPTH_LOCATION] = program->uniformLocation("fScaleDepth");
    locations[SCALE_OVER_SCALE_DEPTH_LOCATION] = program->uniformLocation("fScaleOverScaleDepth");
    locations[G_LOCATION] = program->uniformLocation("g");
    locations[G2_LOCATION] = program->uniformLocation("g2");
    
    return program;
}

void Environment::renderAtmosphere(Camera& camera, const EnvironmentData& data) {
    glm::vec3 center = data.getAtmosphereCenter(camera.getPosition());
    
    glPushMatrix();
    glTranslatef(center.x, center.y, center.z);
    
    glm::vec3 relativeCameraPos = camera.getPosition() - center;
    float height = glm::length(relativeCameraPos);
    
    // use the appropriate shader depending on whether we're inside or outside
    ProgramObject* program;
    int* locations;
    if (height < data.getAtmosphereOuterRadius()) {
        program = _skyFromAtmosphereProgram;
        locations = _skyFromAtmosphereUniformLocations;
        
    } else {
        program = _skyFromSpaceProgram;
        locations = _skyFromSpaceUniformLocations;
    }
    
    // the constants here are from Sean O'Neil's GPU Gems entry
    // (http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html), GameEngine.cpp
    program->bind();
    program->setUniform(locations[CAMERA_POS_LOCATION], relativeCameraPos);
    glm::vec3 lightDirection = glm::normalize(data.getSunLocation());
    program->setUniform(locations[LIGHT_POS_LOCATION], lightDirection);
    program->setUniformValue(locations[INV_WAVELENGTH_LOCATION],
        1 / powf(data.getScatteringWavelengths().r, 4.0f),
        1 / powf(data.getScatteringWavelengths().g, 4.0f),
        1 / powf(data.getScatteringWavelengths().b, 4.0f));
    program->setUniformValue(locations[CAMERA_HEIGHT2_LOCATION], height * height);
    program->setUniformValue(locations[OUTER_RADIUS_LOCATION], data.getAtmosphereOuterRadius());
    program->setUniformValue(locations[OUTER_RADIUS2_LOCATION], data.getAtmosphereOuterRadius() * data.getAtmosphereOuterRadius());
    program->setUniformValue(locations[INNER_RADIUS_LOCATION], data.getAtmosphereInnerRadius());
    program->setUniformValue(locations[KR_ESUN_LOCATION], data.getRayleighScattering() * data.getSunBrightness());
    program->setUniformValue(locations[KM_ESUN_LOCATION], data.getMieScattering() * data.getSunBrightness());
    program->setUniformValue(locations[KR_4PI_LOCATION], data.getRayleighScattering() * 4.0f * PI);
    program->setUniformValue(locations[KM_4PI_LOCATION], data.getMieScattering() * 4.0f * PI);
    program->setUniformValue(locations[SCALE_LOCATION], 1.0f / (data.getAtmosphereOuterRadius() - data.getAtmosphereInnerRadius()));
    program->setUniformValue(locations[SCALE_DEPTH_LOCATION], 0.25f);
    program->setUniformValue(locations[SCALE_OVER_SCALE_DEPTH_LOCATION],
        (1.0f / (data.getAtmosphereOuterRadius() - data.getAtmosphereInnerRadius())) / 0.25f);
    program->setUniformValue(locations[G_LOCATION], -0.990f);
    program->setUniformValue(locations[G2_LOCATION], -0.990f * -0.990f);
    
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    DependencyManager::get<GeometryCache>()->renderSphere(1.0f, 100, 50, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); //Draw a unit sphere
    glDepthMask(GL_TRUE);
    
    program->release();
    
    glPopMatrix();
}
