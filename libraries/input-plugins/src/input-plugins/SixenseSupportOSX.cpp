//
//  SixenseSupportOSX.cpp
//  libraries/input-plugins/src/input-plugins
//
//  Created by Clement on 10/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Mock implementation of sixense.h to hide dynamic linking on OS X
#if defined(__APPLE__) && defined(HAVE_SIXENSE)
#include <type_traits>

#include <sixense.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QLibrary>

#include "InputPluginsLogging.h"

#ifndef SIXENSE_LIB_FILENAME
#define SIXENSE_LIB_FILENAME QCoreApplication::applicationDirPath() + "/../Frameworks/libsixense_x64"
#endif

using Library = std::unique_ptr<QLibrary>;
static Library SIXENSE;

struct Callable {
    template<typename... Args>
    int operator() (Args&&... args){
        return reinterpret_cast<int(*)(Args...)>(function)(std::forward<Args>(args)...);
    }
    QFunctionPointer function;
};

Callable resolve(const Library& library, const char* name) {
    Q_ASSERT_X(library && library->isLoaded(), __FUNCTION__, "Sixense library not loaded");
    auto function = library->resolve(name);
    Q_ASSERT_X(function, __FUNCTION__, std::string("Could not resolve ").append(name).c_str());
    return Callable { function };
}
#define FORWARD resolve(SIXENSE, __FUNCTION__)


void loadSixense() {
    Q_ASSERT_X(!(SIXENSE && SIXENSE->isLoaded()), __FUNCTION__, "Sixense library already loaded");
    SIXENSE.reset(new QLibrary(SIXENSE_LIB_FILENAME));
    Q_CHECK_PTR(SIXENSE);
    
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


// sixense.h wrapper for OSX dynamic linking
int sixenseInit() {
    loadSixense();
    return FORWARD();
}
int sixenseExit() {
    auto returnCode = FORWARD();
    unloadSixense();
    return returnCode;
}

int sixenseGetMaxBases() {
    return FORWARD();
}
int sixenseSetActiveBase(int i) {
    return FORWARD(i);
}
int sixenseIsBaseConnected(int i) {
    return FORWARD(i);
}

int sixenseGetMaxControllers() {
    return FORWARD();
}
int sixenseIsControllerEnabled(int which) {
    return FORWARD(which);
}
int sixenseGetNumActiveControllers() {
    return FORWARD();
}

int sixenseGetHistorySize() {
    return FORWARD();
}

int sixenseGetData(int which, int index_back, sixenseControllerData* data) {
    return FORWARD(which, index_back, data);
}
int sixenseGetAllData(int index_back, sixenseAllControllerData* data) {
    return FORWARD(index_back, data);
}
int sixenseGetNewestData(int which, sixenseControllerData* data) {
    return FORWARD(which, data);
}
int sixenseGetAllNewestData(sixenseAllControllerData* data) {
    return FORWARD(data);
}

int sixenseSetHemisphereTrackingMode(int which_controller, int state) {
    return FORWARD(which_controller, state);
}
int sixenseGetHemisphereTrackingMode(int which_controller, int* state) {
    return FORWARD(which_controller, state);
}
int sixenseAutoEnableHemisphereTracking(int which_controller) {
    return FORWARD(which_controller);
}

int sixenseSetHighPriorityBindingEnabled(int on_or_off) {
    return FORWARD(on_or_off);
}
int sixenseGetHighPriorityBindingEnabled(int* on_or_off) {
    return FORWARD(on_or_off);
}

int sixenseTriggerVibration(int controller_id, int duration_100ms, int pattern_id) {
    return FORWARD(controller_id, duration_100ms, pattern_id);
}

int sixenseSetFilterEnabled(int on_or_off) {
    return FORWARD(on_or_off);
}
int sixenseGetFilterEnabled(int* on_or_off) {
    return FORWARD(on_or_off);
}

int sixenseSetFilterParams(float near_range, float near_val, float far_range, float far_val) {
    return FORWARD(near_range, near_val, far_range, far_val);
}
int sixenseGetFilterParams(float* near_range, float* near_val, float* far_range, float* far_val) {
    return FORWARD(near_range, near_val, far_range, far_val);
}

int sixenseSetBaseColor(unsigned char red, unsigned char green, unsigned char blue) {
    return FORWARD(red, green, blue);
}
int sixenseGetBaseColor(unsigned char* red, unsigned char* green, unsigned char* blue) {
    return FORWARD(red, green, blue);
}
#endif
