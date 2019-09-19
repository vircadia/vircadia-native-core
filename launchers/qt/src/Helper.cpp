#include "Helper.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>

#if defined(Q_OS_WIN)

#include <Windows.h>

void launchClient(const QString& clientPath, const QString& homePath, const QString& defaultScriptsPath,
                  const QString& displayName, const QString& contentCachePath, QString loginResponseToken) {

    // TODO Fix parameters
    QString params = "--url " + homePath
        + " --setBookmark hqhome=\"" + homePath + "\""
        + " --defaultScriptsOverride " + defaultScriptsPath
        + " --displayName " + displayName
        + " --cache " + contentCachePath;

    if (!loginResponseToken.isEmpty()) {
        params += " --tokens \"" + loginResponseToken.replace("\"", "\\\"") + "\"";
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // start the program up
    BOOL success = CreateProcess(
        clientPath.toUtf8().data(),
        params.toUtf8().data(),
        nullptr,                 // Process handle not inheritable
        nullptr,                 // Thread handle not inheritable
        FALSE,                   // Set handle inheritance to FALSE
        CREATE_NEW_CONSOLE,      // Opens file in a separate console
        nullptr,                 // Use parent's environment block
        nullptr,                 // Use parent's starting directory 
        &si,                     // Pointer to STARTUPINFO structure
        &pi                      // Pointer to PROCESS_INFORMATION structure
    );

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    QCoreApplication::instance()->quit();
}
#endif



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
