//
//  TestRailInterface.h
//
//  Created by Nissim Hadar on 6 Jul 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_test_testrail_interface_h
#define hifi_test_testrail_interface_h

#include "ui/BusyWindow.h"
#include "ui/TestRailSelectorWindow.h"
#include <QDirIterator>
#include <QtXml/QDomDocument>
#include <QString>

class TestRailInterface : public QObject{
    Q_OBJECT

public:
    TestRailInterface();

    void createTestSuiteXML(const QString& testDirectory,
                            const QString& outputDirectory,
                            const QString& userGitHub,
                            const QString& branchGitHub);

    void createTestSuitePython(const QString& testDirectory,
                               const QString& outputDirectory,
                               const QString& userGitHub,
                               const QString& branchGitHub);

    QDomElement processDirectoryXML(const QString& directory,
                                    const QString& useGitHubr,
                                    const QString& branchGitHub,
                                    const QDomElement& element);

    QDomElement processTestXML(const QString& fullDirectory,
                               const QString& test,
                               const QString& userGitHub,
                               const QString& branchGitHub,
                               const QDomElement& element);

    void processTestPython(const QString& fullDirectory,
                           QTextStream& stream,
                           const QString& userGitHub,
                           const QString& branchGitHub);

    void createTestRailDotPyScript(const QString& outputDirectory);
    void createStackDotPyScript(const QString& outputDirectory);
    void requestDataFromUser();
    void createAddTestCasesPythonScript(const QString& testDirectory,
                                        const QString& outputDirectory,
                                        const QString& userGitHub,
                                        const QString& branchGitHub);

    void processDirectoryPython(const QString& directory,
                                QTextStream& stream,
                                const QString& userGitHub,
                                const QString& branchGitHub);

    bool isAValidTestDirectory(const QString& directory);
    QString getObject(const QString& path);

private:
    QDomDocument _document;

    BusyWindow _busyWindow;
    TestRailSelectorWindow _testRailSelectorWindow;

    QString _url;
    QString _user;
    QString _password;
    QString _project;
};

#endif