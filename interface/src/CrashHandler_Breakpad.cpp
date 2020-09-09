//
//  CrashHandler_Breakpad.cpp
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if HAS_BREAKPAD

#include "CrashHandler.h"

#include <mutex>

#include <client/linux/handler/exception_handler.h>
#include <client/linux/handler/minidump_descriptor.h>

#include <QDebug>
#include <QMap>
#include <QtCore/QFileInfo>
#include <QtAndroidExtras/QAndroidJniObject>

#include <BuildInfo.h>
#include <FingerprintUtils.h>
#include <SettingHelpers.h>
#include <UUID.h>

google_breakpad::ExceptionHandler* gBreakpadHandler;

std::mutex annotationMutex;
QMap<QString, QString> annotations;

static bool breakpad_dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded) {
    return succeeded;
}

QString obbDir() {
    QAndroidJniObject mediaDir = QAndroidJniObject::callStaticObjectMethod("android/os/Environment", "getExternalStorageDirectory", "()Ljava/io/File;");
    QAndroidJniObject mediaPath = mediaDir.callObjectMethod( "getAbsolutePath", "()Ljava/lang/String;" );
    QAndroidJniObject activity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    QAndroidJniObject package = activity.callObjectMethod("getPackageName", "()Ljava/lang/String;");
    QString dataAbsPath = mediaPath.toString()+"/Android/obb/" + package.toString();
    return dataAbsPath;
}

void flushAnnotations() {
    QSettings settings(obbDir() + "/annotations.json", JSON_FORMAT);
    settings.clear();
    settings.beginGroup("Annotations");
    for (auto k : annotations.keys()) {
        settings.setValue(k, annotations.value(k));
    }
    settings.endGroup();
    settings.sync();
}

bool startCrashHandler(std::string appPath) {
    annotations["version"] = BuildInfo::VERSION;
    annotations["build_number"] = BuildInfo::BUILD_NUMBER;
    annotations["build_type"] = BuildInfo::BUILD_TYPE_STRING;

    auto machineFingerPrint = uuidStringWithoutCurlyBraces(FingerprintUtils::getMachineFingerprint());
    annotations["machine_fingerprint"] = machineFingerPrint;

    flushAnnotations();

    gBreakpadHandler = new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(obbDir().toStdString()),
            nullptr, breakpad_dumpCallback, nullptr, true, -1);
    return true;
}

void setCrashAnnotation(std::string name, std::string value) {
    std::lock_guard<std::mutex> guard(annotationMutex);
    QString qName = QString::fromStdString(name);
    QString qValue = QString::fromStdString(value);
    annotations[qName] = qValue;
    flushAnnotations();
}

void startCrashHookMonitor(QCoreApplication* app) {
}

#endif
