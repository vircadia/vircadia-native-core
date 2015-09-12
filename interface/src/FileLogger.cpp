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

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtGui/QDesktopServices>

#include <NumericalConstants.h>
#include <FileUtils.h>
#include <SharedUtil.h>

#include "HifiSockAddr.h"

static const QString FILENAME_FORMAT = "hifi-log_%1_%2.txt";
static const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
static const QString LOGS_DIRECTORY = "Logs";
// Max log size is 1 MB
static const uint64_t MAX_LOG_SIZE = 1024 * 1024;
// Max log age is 1 hour
static const uint64_t MAX_LOG_AGE_USECS = USECS_PER_SECOND * 3600;

QString getLogRollerFilename() {
    QString result = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress = getLocalAddress();
    QDateTime now = QDateTime::currentDateTime();
    result.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
    return result;
}

const QString& getLogFilename() {
    static QString fileName = FileUtils::standardPath(LOGS_DIRECTORY) + "hifi-log.txt";
    return fileName;
}


class FilePersistThread : public GenericQueueThread < QString > {    
public:
    FilePersistThread(const FileLogger& logger) : _logger(logger) {
        setObjectName("LogFileWriter");
    }

protected:
    void rollFileIfNecessary(QFile& file) {
        uint64_t now = usecTimestampNow();
        if ((file.size() > (qint64)MAX_LOG_SIZE) || (now - _lastRollTime) > MAX_LOG_AGE_USECS) {
            QString newFileName = getLogRollerFilename();
            if (file.copy(newFileName)) {
                _lastRollTime = now;
                file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                file.close();
                qDebug() << "Rolled log file: " << newFileName;
            }
        }
    }

    virtual bool processQueueItems(const Queue& messages) {
        QFile file(_logger._fileName);
        rollFileIfNecessary(file);
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            foreach(const QString& message, messages) {
                out << message;
            }
        }
        return true;
    }
private:
    const FileLogger& _logger;
    uint64_t _lastRollTime = 0; 
};

static FilePersistThread* _persistThreadInstance;

FileLogger::FileLogger(QObject* parent) :
    AbstractLoggerInterface(parent), _fileName(getLogFilename())
{
    _persistThreadInstance = new FilePersistThread(*this);
    _persistThreadInstance->initialize(true, QThread::LowestPriority);

}

FileLogger::~FileLogger() {
    _persistThreadInstance->terminate();
}

void FileLogger::addMessage(const QString& message) {
    _persistThreadInstance->queueItem(message);
    emit logReceived(message);
}

void FileLogger::locateLog() {
    FileUtils::locateFile(_fileName);
}

QString FileLogger::getLogData() {
    QString result;
    QFile f(_fileName);
    if (f.open(QFile::ReadOnly | QFile::Text)) {
        result = QTextStream(&f).readAll();
    }
    return result;
}
