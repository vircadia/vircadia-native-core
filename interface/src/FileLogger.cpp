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

class FilePersistThread : public GenericQueueThread < QString > {    
public:
    FilePersistThread(FileLogger& logger) : _logger(logger) {
        setObjectName("LogFileWriter");
    }

protected:
    virtual bool processQueueItems(const Queue& messages) {
        QFile file(_logger._fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            foreach(const QString& message, messages) {
                out << message;
            }
        }
        return true;
    }
private:
    FileLogger& _logger;
};

static FilePersistThread* _persistThreadInstance;

FileLogger::FileLogger(QObject* parent) :
    AbstractLoggerInterface(parent),
    _logData("")
{
    _persistThreadInstance = new FilePersistThread(*this);
    _persistThreadInstance->initialize(true);

    _fileName = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress = getLocalAddress();
    QDateTime now = QDateTime::currentDateTime();
    _fileName.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
}

FileLogger::~FileLogger() {
    _persistThreadInstance->terminate();
}

void FileLogger::addMessage(const QString& message) {
    _persistThreadInstance->queueItem(message);
    emit logReceived(message);
    //_logData += message;
}

void FileLogger::locateLog() {
    FileUtils::locateFile(_fileName);
}
