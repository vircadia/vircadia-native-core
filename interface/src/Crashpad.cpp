//
//  Crashpad.cpp
//  interface/src
//
//  Created by Clement Brisset on 01/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Crashpad.h"

#include <assert.h>

#include <QDebug>

#if HAS_CRASHPAD

#include <mutex>

#include <QStandardPaths>
#include <QDir>

#include <Windows.h>

#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/annotation_list.h>
#include <client/crashpad_info.h>

#include <BuildInfo.h>

using namespace crashpad;

static const std::string BACKTRACE_URL { CMAKE_BACKTRACE_URL };
static const std::string BACKTRACE_TOKEN { CMAKE_BACKTRACE_TOKEN };

extern QString qAppFileName();

CrashpadClient* client { nullptr };
std::mutex annotationMutex;
crashpad::SimpleStringDictionary* crashpadAnnotations { nullptr };

LONG WINAPI vectoredExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    if (!client) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_HEAP_CORRUPTION ||
        pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_STACK_BUFFER_OVERRUN) {
        client->DumpAndCrash(pExceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

bool startCrashHandler() {
    if (BACKTRACE_URL.empty() || BACKTRACE_TOKEN.empty()) {
        return false;
    }

    assert(!client);
    client = new CrashpadClient();
    std::vector<std::string> arguments;

    std::map<std::string, std::string> annotations;
    annotations["token"] = BACKTRACE_TOKEN;
    annotations["format"] = "minidump";
    annotations["version"] = BuildInfo::VERSION.toStdString();

    arguments.push_back("--no-rate-limit");

    // Setup Crashpad DB directory
    const auto crashpadDbName = "crashpad-db";
    const auto crashpadDbDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir(crashpadDbDir).mkpath(crashpadDbName); // Make sure the directory exists
    const auto crashpadDbPath = crashpadDbDir.toStdString() + "/" + crashpadDbName;

    // Locate Crashpad handler
    const std::string CRASHPAD_HANDLER_PATH = QFileInfo(qAppFileName()).absolutePath().toStdString() + "/crashpad_handler.exe";

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

    AddVectoredExceptionHandler(0, vectoredExceptionHandler);

    return client->StartHandler(handler, db, db, BACKTRACE_URL, annotations, arguments, true, true);
}

void setCrashAnnotation(std::string name, std::string value) {
    std::lock_guard<std::mutex> guard(annotationMutex);
    if (!crashpadAnnotations) {
        crashpadAnnotations = new crashpad::SimpleStringDictionary(); // don't free this, let it leak
        crashpad::CrashpadInfo* crashpad_info = crashpad::CrashpadInfo::GetCrashpadInfo();
        crashpad_info->set_simple_annotations(crashpadAnnotations);
    }
    std::replace(value.begin(), value.end(), ',', ';');
    crashpadAnnotations->SetKeyValue(name, value);
}

#else

bool startCrashHandler() {
    qDebug() << "No crash handler available.";
    return false;
}

void setCrashAnnotation(std::string name, std::string value) {
}

#endif
