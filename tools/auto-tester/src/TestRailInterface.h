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
#include "ui/TestRailTestCasesSelectorWindow.h"
#include "ui/TestRailRunSelectorWindow.h"
#include "ui/TestRailResultsSelectorWindow.h"

#include <QDirIterator>
#include <QtXml/QDomDocument>
#include <QProcess>
#include <QString>

#include "PythonInterface.h"

class TestRailInterface : public QObject {
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

    void getReleasesFromTestRail();
    void getTestSectionsFromTestRail();
    void getRunsFromTestRail();

    void createTestRailDotPyScript();
    void createStackDotPyScript();

    bool requestTestRailTestCasesDataFromUser();
    bool requestTestRailRunDataFromUser();
    bool requestTestRailResultsDataFromUser();

    void createAddTestCasesPythonScript(const QString& testDirectory, const QString& userGitHub, const QString& branchGitHub);

    void processDirectoryPython(const QString& directory,
                                QTextStream& stream,
                                const QString& userGitHub,
                                const QString& branchGitHub);

    bool isAValidTestDirectory(const QString& directory);

    QString getObject(const QString& path);

    void updateReleasesComboData(int exitCode, QProcess::ExitStatus exitStatus);
    void updateSectionsComboData(int exitCode, QProcess::ExitStatus exitStatus);
    void updateRunsComboData(int exitCode, QProcess::ExitStatus exitStatus);

    void createTestRailRun(const QString& outputDirectory);
    void updateTestRailRunResults(const QString& testResults, const QString& tempDirectory);

    void addRun();
    void updateRunWithResults();

    void extractTestFailuresFromZippedFolder(const QString& testResults, const QString& tempDirectory);

private:
    // HighFidelity Interface project ID in TestRail
    const int INTERFACE_AUTOMATION_PROJECT_ID{ 26 };

    // Rendering suite ID
    const int INTERFACE_SUITE_ID{ 1312 };

    QDomDocument _document;

    BusyWindow _busyWindow;
    TestRailTestCasesSelectorWindow _testRailTestCasesSelectorWindow;
    TestRailRunSelectorWindow _testRailRunSelectorWindow;
    TestRailResultsSelectorWindow _testRailResultsSelectorWindow;

    QString _url;
    QString _user;
    QString _password;
    QString _projectID;
    QString _suiteID;

    QString _testDirectory;
    QString _testResults;
    QString _outputDirectory;
    QString _userGitHub;
    QString _branchGitHub;

    QStringList _releaseNames;

    QStringList _sectionNames;
    std::vector<int> _sectionIDs;

    QStringList _runNames;
    std::vector<int> _runIDs;

    QString TEMP_NAME{ "fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf" };

    PythonInterface* _pythonInterface;
    QString _pythonCommand;
};

#endif