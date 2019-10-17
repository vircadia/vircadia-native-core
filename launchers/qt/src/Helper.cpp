#include "Helper.h"

#include "PathUtils.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QTextStream>
#include <QStandardPaths>
#include <iostream>
//#incluce <QTextStream>


QString getMetaverseAPIDomain() {
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    if (processEnvironment.contains("HIFI_METAVERSE_URL")) {
        return processEnvironment.value("HIFI_METAVERSE_URL");
    }
    return "https://metaverse.highfidelity.com";
}


void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    Q_UNUSED(context);

    QString date = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString txt = QString("[%1] ").arg(date);

    switch (type) {
      case QtDebugMsg:
         txt += QString("{Debug} \t\t %1").arg(message);
         break;
      case QtWarningMsg:
         txt += QString("{Warning} \t %1").arg(message);
         break;
      case QtCriticalMsg:
         txt += QString("{Critical} \t %1").arg(message);
         break;
      case QtFatalMsg:
          txt += QString("{Fatal} \t\t %1").arg(message);
         break;
      case QtInfoMsg:
          txt += QString("{Info} \t %1").arg(message);
          break;
   }

    QDir logsDir = PathUtils::getLogsDirectory();
    logsDir.mkpath(logsDir.absolutePath());
    QString filename = logsDir.absoluteFilePath("Log.txt");

    QFile outFile(filename);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream textStream(&outFile);
    std::cout << txt.toStdString() << "\n";
    textStream << txt << "\n";
    outFile.close();
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


void cleanLogFile() {
    QDir launcherDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    launcherDirectory.mkpath(launcherDirectory.absolutePath());
    QString filename = launcherDirectory.absoluteFilePath("Log.txt");
    QString tmpFilename = launcherDirectory.absoluteFilePath("Log-last.txt");
    if (QFile::exists(filename)) {
        if (QFile::exists(tmpFilename)) {
            QFile::remove(tmpFilename);
        }
        QFile::rename(filename, tmpFilename);
        QFile::remove(filename);
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
