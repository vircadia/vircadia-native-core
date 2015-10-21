//
//  SixenseSupportOSX.cpp
//
//
//  Created by Clement on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef __APPLE__
#include "sixense.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QLibrary>

using std::string;
using std::unique_ptr;
using SixenseBaseFunction = int (*)();
using SixenseTakeIntFunction = int (*)(int);
using SixenseTakeIntAndSixenseControllerData = int (*)(int, sixenseControllerData*);

static unique_ptr<QLibrary> SIXENSE;

void loadSixense() {
    if (!SIXENSE) {
        static const QString LIBRARY_PATH =
#ifdef SIXENSE_LIB_FILENAME
                                SIXENSE_LIB_FILENAME;
#else
                                QCoreApplication::applicationDirPath() + "/../Frameworks/libsixense_x64";
#endif
        SIXENSE.reset(new QLibrary(LIBRARY_PATH));
    }
    
    if (SIXENSE->load()){
        qDebug() << "Loaded sixense library for hydra support -" << SIXENSE->fileName();
    } else {
        qDebug() << "Sixense library at" << SIXENSE->fileName() << "failed to load:" << SIXENSE->errorString();
        qDebug() << "Continuing without hydra support.";
    }
}

void unloadSixense() {
    SIXENSE->unload();
}

template<typename... Args>
int call(const char* name, Args... args) {
    Q_ASSERT_X(SIXENSE && SIXENSE->isLoaded(), __FUNCTION__, "Sixense library not loaded");
    auto func = reinterpret_cast<int(*)(Args...)>(SIXENSE->resolve(name));
    Q_ASSERT_X(func, __FUNCTION__, string("Could not resolve ").append(name).c_str());
    return func(args...);
}

// sixense.h wrapper for OSX dynamic linking
int sixenseInit() {
    loadSixense();
    return call(__FUNCTION__);
}
int sixenseExit() {
    auto returnCode = call(__FUNCTION__);
    unloadSixense();
    return returnCode;
}

int sixenseGetMaxBases() {
    return call(__FUNCTION__);
}
int sixenseSetActiveBase(int i) {
    return call(__FUNCTION__, i);
}
int sixenseIsBaseConnected(int i) {
    return call(__FUNCTION__, i);
}

int sixenseGetMaxControllers() {
    return call(__FUNCTION__);
}
int sixenseIsControllerEnabled(int which) {
    return call(__FUNCTION__, which);
}
int sixenseGetNumActiveControllers() {
    return call(__FUNCTION__);
}

int sixenseGetHistorySize() {
    return call(__FUNCTION__);
}

int sixenseGetData(int which, int index_back, sixenseControllerData* data) {
    return call(__FUNCTION__, which, index_back, data);
}
int sixenseGetAllData(int index_back, sixenseAllControllerData* data) {
    return call(__FUNCTION__, index_back, data);
}
int sixenseGetNewestData(int which, sixenseControllerData* data) {
    return call(__FUNCTION__, which, data);
}
int sixenseGetAllNewestData(sixenseAllControllerData* data) {
    return call(__FUNCTION__, data);
}

int sixenseSetHemisphereTrackingMode(int which_controller, int state) {
    return call(__FUNCTION__, which_controller, state);
}
int sixenseGetHemisphereTrackingMode(int which_controller, int* state) {
    return call(__FUNCTION__, which_controller, state);
}
int sixenseAutoEnableHemisphereTracking(int which_controller) {
    return call(__FUNCTION__, which_controller);
}

int sixenseSetHighPriorityBindingEnabled(int on_or_off) {
    return call(__FUNCTION__, on_or_off);
}
int sixenseGetHighPriorityBindingEnabled(int* on_or_off) {
    return call(__FUNCTION__, on_or_off);
}

int sixenseTriggerVibration(int controller_id, int duration_100ms, int pattern_id) {
    return call(__FUNCTION__, controller_id, duration_100ms, pattern_id);
}

int sixenseSetFilterEnabled(int on_or_off) {
    return call(__FUNCTION__, on_or_off);
}
int sixenseGetFilterEnabled(int* on_or_off) {
    return call(__FUNCTION__, on_or_off);
}

int sixenseSetFilterParams(float near_range, float near_val, float far_range, float far_val) {
    return call(__FUNCTION__, near_range, near_val, far_range, far_val);
}
int sixenseGetFilterParams(float* near_range, float* near_val, float* far_range, float* far_val) {
    return call(__FUNCTION__, near_range, near_val, far_range, far_val);
}

int sixenseSetBaseColor(unsigned char red, unsigned char green, unsigned char blue) {
    return call(__FUNCTION__, red, green, blue);
}
int sixenseGetBaseColor(unsigned char* red, unsigned char* green, unsigned char* blue) {
    return call(__FUNCTION__, red, green, blue);
}
#endif
