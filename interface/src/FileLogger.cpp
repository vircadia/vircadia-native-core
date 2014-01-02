//
//  FileLogger.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 12/22/13.
//
//

#include "FileLogger.h"
#include "HifiSockAddr.h"
#include <FileUtils.h>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QDesktopServices>

FileLogger::FileLogger() : _logData(NULL) {
    setExtraDebugging(false);
    _fileName = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir logDir(_fileName);
    if (!logDir.exists(_fileName)) {
        logDir.mkdir(_fileName);
    }

    QHostAddress clientAddress = QHostAddress(getHostOrderLocalAddress());
    QDateTime now = QDateTime::currentDateTime();
    _fileName.append(QString("/hifi-log_%1_%2.txt").arg(clientAddress.toString(), now.toString("yyyy-MM-dd_hh.mm.ss")));
}

void FileLogger::addMessage(QString message) {
    QMutexLocker locker(&_mutex);
    emit logReceived(message);
    _logData.append(message);

    QFile file(_fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out <<  message;
    }
}

void FileLogger::locateLog() {
    FileUtils::LocateFile(_fileName);
}
