//
//  TestRunner.h
//
//  Created by Nissim Hadar on 23 Jan 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QLabel>
#include <QObject>

class Worker;

class TestRunner {
public:
    void setWorkingFolder(QLabel* workingFolderLabel);
    void downloadBuildXml(void* caller);

protected:
    QString _workingFolder;

    const QString DEV_BUILD_XML_URL{ "https://highfidelity.com/dev-builds.xml" };
    const QString DEV_BUILD_XML_FILENAME{ "dev-builds.xml" };

    bool buildXMLDownloaded;
};

class Worker : public QObject {
    Q_OBJECT
public:
    void setCommandLine(const QString& commandLine);

public slots:
    int runCommand();

signals:
    void commandComplete();

private:
    QString _commandLine;
};

#endif
