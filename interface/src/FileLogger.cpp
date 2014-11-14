//
//  FileLogger.cpp
//  interface/src
//
//  Created by Stojce Slavkovski on 12/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FileLogger.h"
#include "HifiSockAddr.h"
#include <FileUtils.h>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QDesktopServices>

const QString FILENAME_FORMAT = "hifi-log_%1_%2.txt";
const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
const QString LOGS_DIRECTORY = "Logs";

FileLogger::FileLogger(QObject* parent) :
    AbstractLoggerInterface(parent),
    _logData("")
{
    setExtraDebugging(false);

    _fileName = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress = getLocalAddress();
    QDateTime now = QDateTime::currentDateTime();
    _fileName.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
}

void FileLogger::addMessage(QString message) {
    QMutexLocker locker(&_mutex);
    emit logReceived(message);
    _logData += message;

    QFile file(_fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out <<  message;
    }
}

void FileLogger::locateLog() {
    FileUtils::locateFile(_fileName);
}
