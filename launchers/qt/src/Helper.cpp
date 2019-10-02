#include "Helper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>


QString getMetaverseAPIDomain() {
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    if (processEnvironment.contains("HIFI_METAVERSE_URL")) {
        return processEnvironment.value("HIFI_METAVERSE_URL");
    }
    return "https://metaverse.highfidelity.com";
}

void swapLaunchers(const QString& oldLauncherPath, const QString& newLauncherPath) {
    if (!(QFileInfo::exists(oldLauncherPath) && QFileInfo::exists(newLauncherPath))) {
        qDebug() << "old launcher: " << oldLauncherPath << "new launcher: " << newLauncherPath << " file does not exist";
    }

    bool success = false;
#ifdef Q_OS_MAC
    success = replaceDirectory(oldLauncherPath, newLauncherPath);
#endif

    if (success) {
        qDebug() << "succeessfully replaced";
    } else {
        qDebug() << "failed";
        exit(0);
    }
}

QString getHTTPUserAgent() {
#if defined(Q_OS_WIN)
    return "HQLauncher/fixme (Windows)";
#elif defined(Q_OS_MACOS)
    return "HQLauncher/fixme (MacOS)";
#else
#error Unsupported platform
#endif
}

const QString& getInterfaceSharedMemoryName() {
    static const QString applicationName = "High Fidelity Interface - " + qgetenv("USERNAME");
    return applicationName;
}

//#ifdef Q_OS_WIN
//#include <Windows.h>
//#include <tlhelp32.h>
//#include <wincrypt.h>
//#include <tlhelp32.h>
//#include <strsafe.h>
//#include <winhttp.h>
//
//bool isProcessRunning(const wchar_t *processName, int& processID) {
//    bool exists = false;
//    PROCESSENTRY32 entry;
//    entry.dwSize = sizeof(PROCESSENTRY32);
//
//    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
//    
//    if (Process32First(snapshot, &entry)) {
//        while (Process32Next(snapshot, &entry)) {
//            if (!_wcsicmp(entry.szExeFile, processName)) {
//                exists = true;
//                processID = entry.th32ProcessID;
//                break;
//            }
//        }
//    }
//    CloseHandle(snapshot);
//    return exists;
//}
//#endif
