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
#include <QtCore/QDir>
#include <QtGui/QDesktopServices>

#include "FileUtils.h"
#include "NetworkUtils.h"

#include "../NumericalConstants.h"
#include "../SharedUtil.h"
#include "../SharedLogging.h"

class FilePersistThread : public GenericQueueThread < QString > {
    Q_OBJECT
public:
    FilePersistThread(const FileLogger& logger);

signals:
    void rollingLogFile(QString newFilename);

protected:
    void rollFileIfNecessary(QFile& file, bool notifyListenersIfRolled = true);
    virtual bool processQueueItems(const Queue& messages) override;

private:
    const FileLogger& _logger;
    QMutex _fileMutex;
    uint64_t _lastRollTime;
};



static const QString FILENAME_FORMAT = "hifi-log_%1_%2.txt";
static const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
static const QString LOGS_DIRECTORY = "Logs";
static const QString IPADDR_WILDCARD = "[0-9]*.[0-9]*.[0-9]*.[0-9]*";
static const QString DATETIME_WILDCARD = "20[0-9][0-9]-[0,1][0-9]-[0-3][0-9]_[0-2][0-9].[0-6][0-9].[0-6][0-9]";
static const QString FILENAME_WILDCARD = "hifi-log_" + IPADDR_WILDCARD + "_" + DATETIME_WILDCARD + ".txt";
// Max log size is 512 KB. We send log files to our crash reporter, so we want to keep this relatively
// small so it doesn't go over the 2MB zipped limit for all of the files we send.
static const qint64 MAX_LOG_SIZE = 512 * 1024;
// Max log files found in the log directory is 100.
static const qint64 MAX_LOG_DIR_SIZE = 512 * 1024 * 100;
// Max log age is 1 hour
static const uint64_t MAX_LOG_AGE_USECS = USECS_PER_SECOND * 3600;

static FilePersistThread* _persistThreadInstance;

QString getLogRollerFilename() {
    QString result = FileUtils::standardPath(LOGS_DIRECTORY);
    QHostAddress clientAddress = getGuessedLocalAddress();
    QDateTime now = QDateTime::currentDateTime();
    result.append(QString(FILENAME_FORMAT).arg(clientAddress.toString(), now.toString(DATETIME_FORMAT)));
    return result;
}

const QString& getLogFilename() {
    static QString fileName = FileUtils::standardPath(LOGS_DIRECTORY) + "hifi-log.txt";
    return fileName;
}

FilePersistThread::FilePersistThread(const FileLogger& logger) : _logger(logger) {
    setObjectName("LogFileWriter");

    // A file may exist from a previous run - if it does, roll the file and suppress notifying listeners.
    QFile file(_logger._fileName);
    if (file.exists()) {
        rollFileIfNecessary(file, false);
    }
    _lastRollTime = usecTimestampNow();
}

void FilePersistThread::rollFileIfNecessary(QFile& file, bool notifyListenersIfRolled) {
    uint64_t now = usecTimestampNow();
    if ((file.size() > MAX_LOG_SIZE) || (now - _lastRollTime) > MAX_LOG_AGE_USECS) {
        QString newFileName = getLogRollerFilename();
        if (file.copy(newFileName)) {
            file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            file.close();
            qCDebug(shared) << "Rolled log file:" << newFileName;

            if (notifyListenersIfRolled) {
                emit rollingLogFile(newFileName);
            }

            _lastRollTime = now;
        }
        QStringList nameFilters;
        nameFilters << FILENAME_WILDCARD;

        QDir logQDir(FileUtils::standardPath(LOGS_DIRECTORY));
        logQDir.setNameFilters(nameFilters);
        logQDir.setSorting(QDir::Time);
        QFileInfoList filesInDir = logQDir.entryInfoList();
        qint64 totalSizeOfDir = 0;
        foreach(QFileInfo dirItm, filesInDir){
            if (totalSizeOfDir < MAX_LOG_DIR_SIZE){
                totalSizeOfDir += dirItm.size();
            } else {
                QFile file(dirItm.filePath());
                file.remove();
            }
        }
    }
}

bool FilePersistThread::processQueueItems(const Queue& messages) {
    QMutexLocker lock(&_fileMutex);
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

FileLogger::FileLogger(QObject* parent) :
    AbstractLoggerInterface(parent),
    _fileName(getLogFilename())
{
    _persistThreadInstance = new FilePersistThread(*this);
    _persistThreadInstance->initialize(true, QThread::LowestPriority);
    connect(_persistThreadInstance, &FilePersistThread::rollingLogFile, this, &FileLogger::rollingLogFile);
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

void FileLogger::sync() {
    _persistThreadInstance->process();
}

#include "FileLogger.moc"