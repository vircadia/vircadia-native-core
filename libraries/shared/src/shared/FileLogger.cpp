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
    void rollFileIfNecessary(QFile& file, bool force = false, bool notifyListenersIfRolled = true);
    virtual bool processQueueItems(const Queue& messages) override;

private:
    const FileLogger& _logger;
    QMutex _fileMutex;
};

static const QString FILENAME_FORMAT = "vircadia-log_%1%2.txt";
static const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
static const QString LOGS_DIRECTORY = "Logs";
static const QString DATETIME_WILDCARD = "20[0-9][0-9]-[01][0-9]-[0-3][0-9]_[0-2][0-9]\\.[0-6][0-9]\\.[0-6][0-9]";
static const QString SESSION_WILDCARD = "[0-9a-z]{8}(-[0-9a-z]{4}){3}-[0-9a-z]{12}";
static QRegExp LOG_FILENAME_REGEX { "vircadia-log_" + DATETIME_WILDCARD + "(_" + SESSION_WILDCARD + ")?\\.txt" };
static QUuid SESSION_ID;

// Max log size is 512 KB. We send log files to our crash reporter, so we want to keep this relatively
// small so it doesn't go over the 2MB zipped limit for all of the files we send.
static const qint64 MAX_LOG_SIZE = 512 * 1024;
// Max log files found in the log directory is 100.
static const qint64 MAX_LOG_DIR_SIZE = 512 * 1024 * 100;

static FilePersistThread* _persistThreadInstance;

QString getLogRollerFilename() {
    QString result = FileUtils::standardPath(LOGS_DIRECTORY);
    QDateTime now = QDateTime::currentDateTime();
    QString fileSessionID;

    if (!SESSION_ID.isNull()) {
        fileSessionID = "_" + SESSION_ID.toString().replace("{", "").replace("}", "");
    }

    result.append(QString(FILENAME_FORMAT).arg(now.toString(DATETIME_FORMAT), fileSessionID));
    return result;
}

const QString& getLogFilename() {
    static QString fileName = FileUtils::standardPath(LOGS_DIRECTORY) + "vircadia-log.txt";
    return fileName;
}

FilePersistThread::FilePersistThread(const FileLogger& logger) : _logger(logger) {
    setObjectName("LogFileWriter");

    // A file may exist from a previous run - if it does, roll the file and suppress notifying listeners.
    QFile file(_logger._fileName);
    if (file.exists()) {
        rollFileIfNecessary(file, true, false);
    }
}

void FilePersistThread::rollFileIfNecessary(QFile& file, bool force, bool notifyListenersIfRolled) {
    if (force || (file.size() > MAX_LOG_SIZE)) {
        QString newFileName = getLogRollerFilename();
        if (file.copy(newFileName)) {
            file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            file.close();

            if (notifyListenersIfRolled) {
                emit rollingLogFile(newFileName);
            }
        }

        QDir logDir(FileUtils::standardPath(LOGS_DIRECTORY));
        logDir.setSorting(QDir::Time);
        logDir.setFilter(QDir::Files);
        qint64 totalSizeOfDir = 0;
        QFileInfoList filesInDir = logDir.entryInfoList();
        for (auto& fileInfo : filesInDir) {
            if (!LOG_FILENAME_REGEX.exactMatch(fileInfo.fileName())) {
                continue;
            }
            totalSizeOfDir += fileInfo.size();
            if (totalSizeOfDir > MAX_LOG_DIR_SIZE){
                qDebug() << "Removing log file: " << fileInfo.fileName();
                QFile oldLogFile(fileInfo.filePath());
                oldLogFile.remove();
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
        for (const QString& message : messages) {
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

void FileLogger::setSessionID(const QUuid& message) {
    // This is for the output of log files. Once the application is first started, 
    // this function runs and grabs the AccountManager Session ID and saves it here.
    SESSION_ID = message;
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