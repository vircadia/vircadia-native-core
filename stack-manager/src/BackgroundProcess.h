//
//  BackgroundProcess.h
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 07/03/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_BackgroundProcess_h
#define hifi_BackgroundProcess_h

#include "LogViewer.h"

#include <QProcess>
#include <QString>
#include <QTimer>

class BackgroundProcess : public QProcess
{
    Q_OBJECT
public:
    BackgroundProcess(const QString& program, QObject* parent = 0);

    LogViewer* getLogViewer() { return _logViewer; }
    
    const QStringList& getLastArgList() const { return _lastArgList; }
    
    void start(const QStringList& arguments);

private slots:
    void processStarted();
    void processError();
    void receivedStandardOutput();
    void receivedStandardError();

private:
    QString _program;
    QStringList _lastArgList;
    QString _logFilePath;
    LogViewer* _logViewer;
    QTimer _logTimer;
    QString _stdoutFilename;
    QString _stderrFilename;
    qint64 _stdoutFilePos;
    qint64 _stderrFilePos;
};

#endif
