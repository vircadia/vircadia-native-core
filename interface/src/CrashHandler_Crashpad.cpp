//
//  CrashHandler_Crashpad.cpp
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if HAS_CRASHPAD

#include "CrashHandler.h"

#include <assert.h>

#include <mutex>
#include <vector>
#include <string>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++14-extensions"
#endif

#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/annotation_list.h>
#include <client/crashpad_info.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <BuildInfo.h>
#include <FingerprintUtils.h>
#include <UUID.h>

using namespace crashpad;

static const std::string BACKTRACE_URL { CMAKE_BACKTRACE_URL };
static const std::string BACKTRACE_TOKEN { CMAKE_BACKTRACE_TOKEN };

CrashpadClient* client { nullptr };
std::mutex annotationMutex;
crashpad::SimpleStringDictionary* crashpadAnnotations { nullptr };

#if defined(Q_OS_WIN)
static const QString CRASHPAD_HANDLER_NAME { "crashpad_handler.exe" };
#else
static const QString CRASHPAD_HANDLER_NAME { "crashpad_handler" };
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#include <typeinfo>

void fatalCxxException(PEXCEPTION_POINTERS pExceptionInfo);


LONG WINAPI vectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    if (!client) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_HEAP_CORRUPTION ||
        pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_STACK_BUFFER_OVERRUN) {
        client->DumpAndCrash(pExceptionInfo);
    }

    if (pExceptionInfo->ExceptionRecord->ExceptionCode == 0xE06D7363) {
        fatalCxxException(pExceptionInfo);
        client->DumpAndCrash(pExceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

#pragma pack(push, ehdata, 4)

struct PMD_internal {
    int mdisp;
    int pdisp;
    int vdisp;
};

struct ThrowInfo_internal {
    __int32 attributes;
    __int32 pmfnUnwind;           // 32-bit RVA
    __int32 pForwardCompat;       // 32-bit RVA
    __int32 pCatchableTypeArray;  // 32-bit RVA
};

struct CatchableType_internal {
    __int32 properties;
    __int32 pType;                // 32-bit RVA
    PMD_internal thisDisplacement;
    __int32 sizeOrOffset;
    __int32 copyFunction;         // 32-bit RVA
};

#pragma warning(disable : 4200)
struct CatchableTypeArray_internal {
    int nCatchableTypes;
    __int32 arrayOfCatchableTypes[0]; // 32-bit RVA
};
#pragma warning(default : 4200)

#pragma pack(pop, ehdata)

// everything inside this function is extremely undocumented, attempting to extract
// the underlying C++ exception type (or at least its name) before throwing the whole
// mess at crashpad
void fatalCxxException(PEXCEPTION_POINTERS pExceptionInfo) {
    PEXCEPTION_RECORD ExceptionRecord = pExceptionInfo->ExceptionRecord;

    if(ExceptionRecord->NumberParameters != 4 || ExceptionRecord->ExceptionInformation[0] != 0x19930520) {
        // doesn't match expected parameter counts or magic numbers
        return;
    }

    //ULONG_PTR signature = ExceptionRecord->ExceptionInformation[0];
    void* pExceptionObject = reinterpret_cast<void*>(ExceptionRecord->ExceptionInformation[1]); // the object that generated the exception
    ThrowInfo_internal* pThrowInfo = reinterpret_cast<ThrowInfo_internal*>(ExceptionRecord->ExceptionInformation[2]);
    ULONG_PTR moduleBase = ExceptionRecord->ExceptionInformation[3];
    if(moduleBase == 0 || pThrowInfo == NULL) {
      return; // broken assumption
    }

    // now we start breaking the pThrowInfo internal structure apart
    if(pThrowInfo->pCatchableTypeArray == 0) {
      return; // broken assumption
    }
    CatchableTypeArray_internal* pCatchableTypeArray = reinterpret_cast<CatchableTypeArray_internal*>(moduleBase + pThrowInfo->pCatchableTypeArray);
    if(pCatchableTypeArray->nCatchableTypes == 0 || pCatchableTypeArray->arrayOfCatchableTypes[0] == 0) {
      return; // broken assumption
    }
    CatchableType_internal* pCatchableType = reinterpret_cast<CatchableType_internal*>(moduleBase + pCatchableTypeArray->arrayOfCatchableTypes[0]);
    if(pCatchableType->pType == 0) {
      return; // broken assumption
    }
    const std::type_info* type = reinterpret_cast<std::type_info*>(moduleBase + pCatchableType->pType);

    // we're crashing, not really sure it matters who's currently holding the lock (although this could in theory really mess things up
    annotationMutex.try_lock();
    crashpadAnnotations->SetKeyValue("thrownObject", type->name());

    QString compatibleObjects;
    for (int catchTypeIdx = 1; catchTypeIdx < pCatchableTypeArray->nCatchableTypes; catchTypeIdx++) {
        CatchableType_internal* pCatchableSuperclassType = reinterpret_cast<CatchableType_internal*>(moduleBase + pCatchableTypeArray->arrayOfCatchableTypes[catchTypeIdx]);
        if (pCatchableSuperclassType->pType == 0) {
          return; // broken assumption
        }
        const std::type_info* superclassType = reinterpret_cast<std::type_info*>(moduleBase + pCatchableSuperclassType->pType);

        if (!compatibleObjects.isEmpty()) {
            compatibleObjects += ", ";
        }
        compatibleObjects += superclassType->name();
    }
    crashpadAnnotations->SetKeyValue("thrownObjectLike", compatibleObjects.toStdString());
}

#endif

bool startCrashHandler(std::string appPath) {
    if (BACKTRACE_URL.empty() || BACKTRACE_TOKEN.empty()) {
        return false;
    }

    assert(!client);
    client = new CrashpadClient();
    std::vector<std::string> arguments;

    std::map<std::string, std::string> annotations;
    annotations["sentry[release]"] = BACKTRACE_TOKEN;
    annotations["sentry[contexts][app][app_version]"] = BuildInfo::VERSION.toStdString();
    annotations["sentry[contexts][app][app_build]"] = BuildInfo::BUILD_NUMBER.toStdString();
    annotations["build_type"] = BuildInfo::BUILD_TYPE_STRING.toStdString();

    auto machineFingerPrint = uuidStringWithoutCurlyBraces(FingerprintUtils::getMachineFingerprint());
    annotations["machine_fingerprint"] = machineFingerPrint.toStdString();


    arguments.push_back("--no-rate-limit");

    // Setup Crashpad DB directory
    const auto crashpadDbName = "crashpad-db";
    const auto crashpadDbDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir(crashpadDbDir).mkpath(crashpadDbName); // Make sure the directory exists
    const auto crashpadDbPath = crashpadDbDir.toStdString() + "/" + crashpadDbName;

    // Locate Crashpad handler
    const QFileInfo interfaceBinary { QString::fromStdString(appPath) };
    const QDir interfaceDir = interfaceBinary.dir();
    assert(interfaceDir.exists(CRASHPAD_HANDLER_NAME));
    const std::string CRASHPAD_HANDLER_PATH = interfaceDir.filePath(CRASHPAD_HANDLER_NAME).toStdString();

    // Setup different file paths
    base::FilePath::StringType dbPath;
    base::FilePath::StringType handlerPath;
    dbPath.assign(crashpadDbPath.cbegin(), crashpadDbPath.cend());
    handlerPath.assign(CRASHPAD_HANDLER_PATH.cbegin(), CRASHPAD_HANDLER_PATH.cend());

    base::FilePath db(dbPath);
    base::FilePath handler(handlerPath);

    auto database = crashpad::CrashReportDatabase::Initialize(db);
    if (database == nullptr || database->GetSettings() == nullptr) {
        return false;
    }

    // Enable automated uploads.
    database->GetSettings()->SetUploadsEnabled(true);

#ifdef Q_OS_WIN
    AddVectoredExceptionHandler(0, vectoredExceptionHandler);
#endif

    return client->StartHandler(handler, db, db, BACKTRACE_URL, annotations, arguments, true, true);
}

void setCrashAnnotation(std::string name, std::string value) {
    if (client) {
        std::lock_guard<std::mutex> guard(annotationMutex);
        if (!crashpadAnnotations) {
            crashpadAnnotations = new crashpad::SimpleStringDictionary(); // don't free this, let it leak
            crashpad::CrashpadInfo* crashpad_info = crashpad::CrashpadInfo::GetCrashpadInfo();
            crashpad_info->set_simple_annotations(crashpadAnnotations);
        }
        std::replace(value.begin(), value.end(), ',', ';');
        crashpadAnnotations->SetKeyValue(name, value);
    }
}

#endif
