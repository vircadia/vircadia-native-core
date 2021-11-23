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

Q_LOGGING_CATEGORY(crash_handler, "vircadia.crash_handler")

#include <assert.h>

#include <vector>
#include <string>

#include <QtCore/QAtomicInteger>
#include <QtCore/QDeadlineTimer>
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
#include <client/crashpad_info.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <BuildInfo.h>
#include <FingerprintUtils.h>
#include <UUID.h>

static const std::string BACKTRACE_URL{ CMAKE_BACKTRACE_URL };
static const std::string BACKTRACE_TOKEN{ CMAKE_BACKTRACE_TOKEN };

// ------------------------------------------------------------------------------------------------
// SpinLock - a lock that can timeout attempting to lock a block of code, and is in a busy-wait cycle while trying to acquire
//   note that this code will malfunction if you attempt to grab a lock while already holding it

class SpinLock {
public:
    SpinLock();
    void lock();
    bool lock(int msecs);
    void unlock();

private:
    QAtomicInteger<int> _lock{ 0 };

    Q_DISABLE_COPY(SpinLock)
};

class SpinLockLocker {
public:
    SpinLockLocker(SpinLock& lock, int msecs = -1);
    ~SpinLockLocker();
    bool isLocked() const;
    void unlock();
    bool relock(int msecs = -1);

private:
    SpinLock* _lock;
    bool _isLocked;

    Q_DISABLE_COPY(SpinLockLocker)
};

SpinLock::SpinLock() {
}

void SpinLock::lock() {
    while (!_lock.testAndSetAcquire(0, 1))
        ;
}

bool SpinLock::lock(int msecs) {
    QDeadlineTimer deadline(msecs);
    for (;;) {
        if (_lock.testAndSetAcquire(0, 1)) {
            return true;
        }
        if (deadline.hasExpired()) {
            return false;
        }
    }
}

void SpinLock::unlock() {
    _lock.storeRelease(0);
}

SpinLockLocker::SpinLockLocker(SpinLock& lock, int msecs /* = -1 */ ) : _lock(&lock) {
    _isLocked = _lock->lock(msecs);
}

SpinLockLocker::~SpinLockLocker() {
    if (_isLocked) {
        _lock->unlock();
    }
}

bool SpinLockLocker::isLocked() const {
    return _isLocked;
}

void SpinLockLocker::unlock() {
    if (_isLocked) {
        _lock->unlock();
        _isLocked = false;
    }
}

bool SpinLockLocker::relock(int msecs /* = -1 */ ) {
    if (!_isLocked) {
        _isLocked = _lock->lock(msecs);
    }
    return _isLocked;
}

// ------------------------------------------------------------------------------------------------

crashpad::CrashpadClient* client{ nullptr };
SpinLock crashpadAnnotationsProtect;
crashpad::SimpleStringDictionary* crashpadAnnotations{ nullptr };

#if defined(Q_OS_WIN)
static const QString CRASHPAD_HANDLER_NAME{ "crashpad_handler.exe" };
#else
static const QString CRASHPAD_HANDLER_NAME{ "crashpad_handler" };
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
// ------------------------------------------------------------------------------------------------
// The area within this #ifdef is specific to the Microsoft C++ compiler

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QLogging.h>
#include <QtCore/QTimer>

#include <Windows.h>
#include <typeinfo>

static constexpr DWORD STATUS_MSVC_CPP_EXCEPTION = 0xE06D7363;
static constexpr ULONG_PTR MSVC_CPP_EXCEPTION_SIGNATURE = 0x19930520;
static constexpr int ANNOTATION_LOCK_WEAK_ATTEMPT = 5000;  // attempt to lock the annotations list, but give up if it takes more than 5 seconds

LPTOP_LEVEL_EXCEPTION_FILTER gl_crashpadUnhandledExceptionFilter = nullptr;
QTimer unhandledExceptionTimer;  // checks occasionally in case loading an external DLL reset the unhandled exception pointer

void fatalCxxException(PEXCEPTION_POINTERS pExceptionInfo);  // extracts type information from a thrown C++ exception
LONG WINAPI firstChanceExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);  // called on any thrown exception (whether or not it's caught)
LONG WINAPI unhandledExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo);  // called on any exception without a corresponding catch

static LONG WINAPI firstChanceExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    // we're catching these exceptions on first-chance as the system state is corrupted at this point and they may not survive the exception handling mechanism
    if (client && (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_HEAP_CORRUPTION ||
                   pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_STACK_BUFFER_OVERRUN)) {
        client->DumpAndCrash(pExceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI unhandledExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    if (client && pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_MSVC_CPP_EXCEPTION) {
        fatalCxxException(pExceptionInfo);
        client->DumpAndCrash(pExceptionInfo);
    }

    if (gl_crashpadUnhandledExceptionFilter != nullptr) {
        return gl_crashpadUnhandledExceptionFilter(pExceptionInfo);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

// The following structures are modified versions of structs defined inplicitly by the Microsoft C++ compiler
// as described at http://www.geoffchappell.com/studies/msvc/language/predefined/
// They are redefined here as the definitions the compiler gives only work in 32-bit contexts and are out-of-sync
// with the internal structures when operating in a 64-bit environment
// as discovered and described here: https://stackoverflow.com/questions/39113168/c-rtti-in-a-windows-64-bit-vectoredexceptionhandler-ms-visual-studio-2015

#pragma pack(push, ehdata, 4)

struct PMD_internal {  // internal name: _PMD (no changes, so could in theory just use the original)
    int mdisp;
    int pdisp;
    int vdisp;
};

struct ThrowInfo_internal {  // internal name: _ThrowInfo (changed all pointers into __int32)
    __int32 attributes;
    __int32 pmfnUnwind;           // 32-bit RVA
    __int32 pForwardCompat;       // 32-bit RVA
    __int32 pCatchableTypeArray;  // 32-bit RVA
};

struct CatchableType_internal {  // internal name: _CatchableType (changed all pointers into __int32)
    __int32 properties;
    __int32 pType;               // 32-bit RVA
    PMD_internal thisDisplacement;
    __int32 sizeOrOffset;
    __int32 copyFunction;        // 32-bit RVA
};

#pragma warning(disable : 4200)
struct CatchableTypeArray_internal {  // internal name: _CatchableTypeArray (changed all pointers into __int32)
    int nCatchableTypes;
    __int32 arrayOfCatchableTypes[0];  // 32-bit RVA
};
#pragma warning(default : 4200)

#pragma pack(pop, ehdata)

// everything inside this function is extremely undocumented, attempting to extract
// the underlying C++ exception type (or at least its name) before throwing the whole
// mess at crashpad
// Some links describing how C++ exception handling works in an SEH context
// (since C++ exceptions are a figment of the Microsoft compiler):
//  - https://www.codeproject.com/Articles/175482/Compiler-Internals-How-Try-Catch-Throw-are-Interpr
//  - https://stackoverflow.com/questions/21888076/how-to-find-the-context-record-for-user-mode-exception-on-x64

static void fatalCxxException(PEXCEPTION_POINTERS pExceptionInfo) {
    SpinLockLocker guard(crashpadAnnotationsProtect, ANNOTATION_LOCK_WEAK_ATTEMPT);
    if (!guard.isLocked()) {
        return;
    }

    PEXCEPTION_RECORD ExceptionRecord = pExceptionInfo->ExceptionRecord;
    /*
    Exception arguments for Microsoft C++ exceptions:
    [0] signature  - magic number
    [1] void*      - variable that is being thrown
    [2] ThrowInfo* - description of the variable that was thrown
    [3] HMODULE    - (64-bit only) base address that all 32bit pointers are added to
    */

    if (ExceptionRecord->NumberParameters != 4 || ExceptionRecord->ExceptionInformation[0] != MSVC_CPP_EXCEPTION_SIGNATURE) {
        // doesn't match expected parameter counts or magic numbers
        return;
    }

    // get the ThrowInfo struct from the exception arguments
    ThrowInfo_internal* pThrowInfo = reinterpret_cast<ThrowInfo_internal*>(ExceptionRecord->ExceptionInformation[2]);
    ULONG_PTR moduleBase = ExceptionRecord->ExceptionInformation[3];
    if (moduleBase == 0 || pThrowInfo == NULL) {
        return;  // broken assumption
    }

    // get the CatchableTypeArray* struct from ThrowInfo
    if (pThrowInfo->pCatchableTypeArray == 0) {
        return;  // broken assumption
    }
    CatchableTypeArray_internal* pCatchableTypeArray =
        reinterpret_cast<CatchableTypeArray_internal*>(moduleBase + pThrowInfo->pCatchableTypeArray);
    if (pCatchableTypeArray->nCatchableTypes == 0 || pCatchableTypeArray->arrayOfCatchableTypes[0] == 0) {
        return;  // broken assumption
    }

    // get the CatchableType struct for the actual exception type from CatchableTypeArray
    CatchableType_internal* pCatchableType =
        reinterpret_cast<CatchableType_internal*>(moduleBase + pCatchableTypeArray->arrayOfCatchableTypes[0]);
    if (pCatchableType->pType == 0) {
        return;  // broken assumption
    }
    const std::type_info* type = reinterpret_cast<std::type_info*>(moduleBase + pCatchableType->pType);

    crashpadAnnotations->SetKeyValue("thrownObject", type->name());

    // After annotating the name of the actual object type, go through the other entries in CatcahleTypeArray and itemize the list of possible
    // catch() commands that could have caught this so we can find the list of its superclasses
    QString compatibleObjects;
    for (int catchTypeIdx = 1; catchTypeIdx < pCatchableTypeArray->nCatchableTypes; catchTypeIdx++) {
        CatchableType_internal* pCatchableSuperclassType =
            reinterpret_cast<CatchableType_internal*>(moduleBase + pCatchableTypeArray->arrayOfCatchableTypes[catchTypeIdx]);
        if (pCatchableSuperclassType->pType == 0) {
            return;  // broken assumption
        }
        const std::type_info* superclassType = reinterpret_cast<std::type_info*>(moduleBase + pCatchableSuperclassType->pType);

        if (!compatibleObjects.isEmpty()) {
            compatibleObjects += ", ";
        }
        compatibleObjects += superclassType->name();
    }
    crashpadAnnotations->SetKeyValue("thrownObjectLike", compatibleObjects.toStdString());
}

void checkUnhandledExceptionHook() {
    LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionFilter = SetUnhandledExceptionFilter(unhandledExceptionHandler);
    if (prevExceptionFilter != unhandledExceptionHandler) {
        qWarning() << QString("Restored unhandled exception filter (which had been changed to %1)")
                          .arg(reinterpret_cast<ULONG_PTR>(prevExceptionFilter), 16, 16, QChar('0'));
    }
}

// End of code specific to the Microsoft C++ compiler
// ------------------------------------------------------------------------------------------------
#endif  // Q_OS_WIN

// Locate the full path to the binary's directory
static QString findBinaryDir() {
    // Normally we'd just use QCoreApplication::applicationDirPath(), but we can't.
    // That function needs the QApplication to be created first, and Crashpad is initialized as early as possible,
    // which is well before QApplication, so that function throws out a warning and returns ".".
    //
    // So we must do things the hard way here. In particular this is needed to correctly handle things in AppImage
    // on Linux. On Windows and MacOS falling back to argv[0] should be fine.

#ifdef Q_OS_LINUX
    // Find outselves by looking at /proc/<PID>/exe
    pid_t ourPid = getpid();
    QString exeLink = QString("/proc/%1/exe").arg(ourPid);
    qCDebug(crash_handler) << "Looking at" << exeLink;

    QFileInfo exeLinkInfo(exeLink);
    if (exeLinkInfo.isSymLink()) {
        QFileInfo exeInfo(exeLinkInfo.symLinkTarget());
        qCDebug(crash_handler) << "exe symlink points at" << exeInfo;
        return exeInfo.absoluteDir().absolutePath();
    } else {
        qCWarning(crash_handler) << exeLink << "isn't a symlink. /proc not mounted?";
    }

#endif

    return QString();
}

bool startCrashHandler(std::string appPath) {
    if (BACKTRACE_URL.empty() || BACKTRACE_TOKEN.empty()) {
        qCCritical(crash_handler) << "Backtrace URL or token not set, crash handler disabled.";
        return false;
    }

    assert(!client);
    client = new crashpad::CrashpadClient();
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
    QDir(crashpadDbDir).mkpath(crashpadDbName);  // Make sure the directory exists
    const auto crashpadDbPath = crashpadDbDir.toStdString() + "/" + crashpadDbName;

    // Locate Crashpad handler
    QString binaryDir = findBinaryDir();
    QDir interfaceDir;

    if (!binaryDir.isEmpty()) {
        // Locating ourselves by argv[0] fails in the case of AppImage on Linux, as we get the AppImage
        // itself in there. If we have a platform-specific method, and it succeeds, we use that instead
        // of argv.
        qCDebug(crash_handler) << "Locating own directory by platform-specific method";
        interfaceDir.setPath(binaryDir);
    } else {
        qCDebug(crash_handler) << "Locating own directory by argv[0]";
        interfaceDir.setPath(QString::fromStdString(appPath));
        // argv[0] gets us the path including the binary file
        interfaceDir.cdUp();
    }

    if (!interfaceDir.exists(CRASHPAD_HANDLER_NAME)) {
        qCCritical(crash_handler) << "Failed to find" << CRASHPAD_HANDLER_NAME << "in" << interfaceDir << ", can't start crash handler";
        return false;
    }

    const std::string CRASHPAD_HANDLER_PATH = interfaceDir.filePath(CRASHPAD_HANDLER_NAME).toStdString();

    qCDebug(crash_handler) << "Crashpad handler found at" << QString::fromStdString(CRASHPAD_HANDLER_PATH);

    // Setup different file paths
    base::FilePath::StringType dbPath;
    base::FilePath::StringType handlerPath;
    dbPath.assign(crashpadDbPath.cbegin(), crashpadDbPath.cend());
    handlerPath.assign(CRASHPAD_HANDLER_PATH.cbegin(), CRASHPAD_HANDLER_PATH.cend());

    base::FilePath db(dbPath);
    base::FilePath handler(handlerPath);

    qCDebug(crash_handler) << "Opening crashpad database" << QString::fromStdString(crashpadDbPath);
    auto database = crashpad::CrashReportDatabase::Initialize(db);
    if (database == nullptr || database->GetSettings() == nullptr) {
        qCCritical(crash_handler) << "Failed to open crashpad database" << QString::fromStdString(crashpadDbPath);
        return false;
    }

    // Enable automated uploads.
    database->GetSettings()->SetUploadsEnabled(true);

    if (!client->StartHandler(handler, db, db, BACKTRACE_URL, annotations, arguments, true, true)) {
        qCCritical(crash_handler) << "Failed to start crashpad handler";
        return false;
    }

#ifdef Q_OS_WIN
    AddVectoredExceptionHandler(0, firstChanceExceptionHandler);
    gl_crashpadUnhandledExceptionFilter = SetUnhandledExceptionFilter(unhandledExceptionHandler);
#endif

    qCInfo(crash_handler) << "Crashpad initialized";
    return true;
}

void setCrashAnnotation(std::string name, std::string value) {
    if (client) {
        SpinLockLocker guard(crashpadAnnotationsProtect);
        if (!crashpadAnnotations) {
            crashpadAnnotations = new crashpad::SimpleStringDictionary();  // don't free this, let it leak
            crashpad::CrashpadInfo* crashpad_info = crashpad::CrashpadInfo::GetCrashpadInfo();
            crashpad_info->set_simple_annotations(crashpadAnnotations);
        }
        std::replace(value.begin(), value.end(), ',', ';');
        crashpadAnnotations->SetKeyValue(name, value);
    }
}

void startCrashHookMonitor(QCoreApplication* app) {
#ifdef Q_OS_WIN
    // create a timer that checks to see if our exception handler has been reset.  This may occur when a new CRT
    // is initialized, which could happen any time a DLL not compiled with the same compiler is loaded.
    // It would be nice if this were replaced with a more intelligent response; this fires once a minute which
    // may be too much (extra code running) and too little (leaving up to a 1min gap after the hook is broken)
    checkUnhandledExceptionHook();

    unhandledExceptionTimer.moveToThread(app->thread());
    QObject::connect(&unhandledExceptionTimer, &QTimer::timeout, checkUnhandledExceptionHook);
    unhandledExceptionTimer.start(60000);
#endif  // Q_OS_WIN
}

#endif  // HAS_CRASHPAD
