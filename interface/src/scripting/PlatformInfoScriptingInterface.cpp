//
//  Created by Nissim Hadar on 2018/12/28
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PlatformInfoScriptingInterface.h"
#include "Application.h"
#include <shared/GlobalAppProperties.h>
#include <thread>

#include <platform/Platform.h>
#include <platform/Profiler.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#elif defined Q_OS_MAC
#include <sstream>
#endif

PlatformInfoScriptingInterface* PlatformInfoScriptingInterface::getInstance() {
    static PlatformInfoScriptingInterface sharedInstance;
    return &sharedInstance;
}


PlatformInfoScriptingInterface::PlatformInfoScriptingInterface() {
    platform::create();
    if (!platform::enumeratePlatform()) {
    }
}

PlatformInfoScriptingInterface::~PlatformInfoScriptingInterface() {
    platform::destroy();
}

QString PlatformInfoScriptingInterface::getOperatingSystemType() {
#ifdef Q_OS_WIN
    return "WINDOWS";
#elif defined Q_OS_MAC
    return "MACOS";
#else
    return "UNKNOWN";
#endif
}

QString PlatformInfoScriptingInterface::getCPUBrand() {
#ifdef Q_OS_WIN
    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];

    for (i = 0x80000000; i <= nExIds; ++i) {
       __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002) {
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000003) {
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        } else if (i == 0x80000004) {
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
        }
    }

    return CPUBrandString;
#elif defined Q_OS_MAC
    FILE* stream = popen("sysctl -n machdep.cpu.brand_string", "r");
    
    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }
    
    return QString::fromStdString(hostStream.str());
#else
    return QString("NO IMPLEMENTED");
#endif
}

unsigned int PlatformInfoScriptingInterface::getNumLogicalCores() {

    return std::thread::hardware_concurrency();
}

int PlatformInfoScriptingInterface::getTotalSystemMemoryMB() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof (statex);
    GlobalMemoryStatusEx(&statex);
    return statex.ullTotalPhys / 1024 / 1024;
#elif defined Q_OS_MAC
    FILE* stream = popen("sysctl -a | grep hw.memsize", "r");
    
    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }
    
    QString result = QString::fromStdString(hostStream.str());
    QStringList parts = result.split(' ');
    return (int)(parts[1].toDouble() / 1024 / 1024);
#else
    return -1;
#endif
}

QString PlatformInfoScriptingInterface::getGraphicsCardType() {
#ifdef Q_OS_WIN
    return qApp->getGraphicsCardType();
#elif defined Q_OS_MAC
    FILE* stream = popen("system_profiler SPDisplaysDataType | grep Chipset", "r");
    
    std::ostringstream hostStream;
    while (!feof(stream) && !ferror(stream)) {
        char buf[128];
        int bytesRead = fread(buf, 1, 128, stream);
        hostStream.write(buf, bytesRead);
    }
    
    QString result = QString::fromStdString(hostStream.str());
    QStringList parts = result.split('\n');
    for (int i = 0; i < parts.size(); ++i) {
        if (parts[i].toLower().contains("radeon") || parts[i].toLower().contains("nvidia")) {
            return parts[i];
        }
    }

    // unkown graphics card
    return "UNKNOWN";
#else
    return QString("NO IMPLEMENTED");
#endif
}

bool PlatformInfoScriptingInterface::hasRiftControllers() {
    return qApp->hasRiftControllers();
}

bool PlatformInfoScriptingInterface::hasViveControllers() {
    return qApp->hasViveControllers();
}

bool PlatformInfoScriptingInterface::has3DHTML() {
#if defined(Q_OS_ANDROID)
    return false;
#else
    return !qApp->property(hifi::properties::STANDALONE).toBool();
#endif
}

bool PlatformInfoScriptingInterface::isStandalone() {
#if defined(Q_OS_ANDROID)
    return false;
#else
    return qApp->property(hifi::properties::STANDALONE).toBool();
#endif
}

int PlatformInfoScriptingInterface::getNumCPUs() {
    return platform::getNumCPUs();
}
int PlatformInfoScriptingInterface::getMasterCPU() {
    return platform::getMasterCPU();
}
QString PlatformInfoScriptingInterface::getCPU(int index) {
    auto desc = platform::getCPU(index);
    return QString(desc.dump().c_str());
}

int PlatformInfoScriptingInterface::getNumGPUs() {
    return platform::getNumGPUs();
}
int PlatformInfoScriptingInterface::getMasterGPU() {
    return platform::getMasterGPU();
}
QString PlatformInfoScriptingInterface::getGPU(int index) {
    auto desc = platform::getGPU(index);
    return QString(desc.dump().c_str());
}

int PlatformInfoScriptingInterface::getNumDisplays() {
    return platform::getNumDisplays();
}
int PlatformInfoScriptingInterface::getMasterDisplay() {
    return platform::getMasterDisplay();
}
QString PlatformInfoScriptingInterface::getDisplay(int index) {
    auto desc = platform::getDisplay(index);
    return QString(desc.dump().c_str());
}

QString PlatformInfoScriptingInterface::getMemory() {
    auto desc = platform::getMemory();
    return QString(desc.dump().c_str());
}

QString PlatformInfoScriptingInterface::getComputer() {
    auto desc = platform::getComputer();
    return QString(desc.dump().c_str());
}

QString PlatformInfoScriptingInterface::getPlatform() {
    auto desc = platform::getAll();
    return QString(desc.dump().c_str());
}

PlatformInfoScriptingInterface::PlatformTier PlatformInfoScriptingInterface::getTierProfiled() {
    return (PlatformInfoScriptingInterface::PlatformTier) platform::Profiler::profilePlatform();
}

QStringList PlatformInfoScriptingInterface::getPlatformTierNames() {
    static const QStringList platformTierNames = { "UNKNOWN", "LOW", "MID", "HIGH" };
    return platformTierNames;
}

bool PlatformInfoScriptingInterface::isRenderMethodDeferredCapable() {
    return platform::Profiler::isRenderMethodDeferredCapable();
}
