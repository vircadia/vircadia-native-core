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

bool loadSixense() {
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
    return SIXENSE->isLoaded();
}

void unloadSixense() {
    SIXENSE->unload();
}

template<typename Func>
Func resolve(const char* name) {
    Q_ASSERT_X(SIXENSE && SIXENSE->isLoaded(), __FUNCTION__, "Sixense library not loaded");
    auto func = reinterpret_cast<Func>(SIXENSE->resolve(name));
    Q_ASSERT_X(func, __FUNCTION__, string("Could not resolve ").append(name).c_str());
    return func;
}

// sixense.h wrapper for OSX dynamic linking
int sixenseInit() {
    loadSixense();
    auto sixenseInit = resolve<SixenseBaseFunction>("sixenseInit");
    return sixenseInit();
}

int sixenseExit() {
    auto sixenseExit = resolve<SixenseBaseFunction>("sixenseExit");
    auto returnCode = sixenseExit();
    unloadSixense();
    return returnCode;
}

int sixenseSetFilterEnabled(int input) {
    auto sixenseSetFilterEnabled = resolve<SixenseTakeIntFunction>("sixenseSetFilterEnabled");
    return sixenseSetFilterEnabled(input);
}

int sixenseGetNumActiveControllers() {
    auto sixenseGetNumActiveControllers = resolve<SixenseBaseFunction>("sixenseGetNumActiveControllers");
    return sixenseGetNumActiveControllers();
}

int sixenseGetMaxControllers() {
    auto sixenseGetMaxControllers = resolve<SixenseBaseFunction>("sixenseGetMaxControllers");
    return sixenseGetMaxControllers();
}

int sixenseIsControllerEnabled(int input) {
    auto sixenseIsControllerEnabled = resolve<SixenseTakeIntFunction>("sixenseIsControllerEnabled");
    return sixenseIsControllerEnabled(input);
}

int sixenseGetNewestData(int input1, sixenseControllerData* intput2) {
    auto sixenseGetNewestData = resolve<SixenseTakeIntAndSixenseControllerData>("sixenseGetNewestData");
    return sixenseGetNewestData(input1, intput2);
}
#endif
