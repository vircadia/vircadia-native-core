//
//  Test.h
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_test_h
#define hifi_test_h

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QRegularExpression>
#include <QProgressBar>

#include "ImageComparer.h"
#include "ui/MismatchWindow.h"
#include "TestRailInterface.h"

class Step {
public:
	QString text;
	bool takeSnapshot;
};

using StepList = std::vector<Step*>;

class ExtractedText {
public:
    QString title;
    StepList stepList;
};

enum TestRailCreateMode {
    PYTHON,
    XML
};

class Test {
public: 
    Test();

    void startTestsEvaluation(const QString& testFolder = QString(), const QString& branchFromCommandLine = QString(), const QString& userFromCommandLine = QString());
    void finishTestsEvaluation(bool isRunningFromCommandline, bool interactiveMode, QProgressBar* progressBar);

    void createRecursiveScript();
    void createAllRecursiveScripts();
    void createRecursiveScript(const QString& topLevelDirectory, bool interactiveMode);

    void createTests();
    void createMDFile();
    void createAllMDFiles();
    void createMDFile(const QString& topLevelDirectory);

    void createTestsOutline();

    void createTestRailTestCases();
    void createTestRailRun();

    bool compareImageLists(bool isInteractiveMode, QProgressBar* progressBar);

    QStringList createListOfAll_imagesInDirectory(const QString& imageFormat, const QString& pathToImageDirectory);

    bool isInSnapshotFilenameFormat(const QString& imageFormat, const QString& filename);

    void includeTest(QTextStream& textStream, const QString& testPathname);

    void appendTestResultsToFile(const QString& testResultsFolderPath, TestFailure testFailure, QPixmap comparisonImage);

    bool createTestResultsFolderPath(const QString& directory);
    void zipAndDeleteTestResultsFolder();

    static bool isAValidDirectory(const QString& pathname);
	QString extractPathFromTestsDown(const QString& fullPath);
    QString getExpectedImageDestinationDirectory(const QString& filename);
    QString getExpectedImagePartialSourceDirectory(const QString& filename);

    ExtractedText getTestScriptLines(QString testFileName);

    void setTestRailCreateMode(TestRailCreateMode testRailCreateMode);

private:
    const QString TEST_FILENAME { "test.js" };
    const QString TEST_RESULTS_FOLDER { "TestResults" };
    const QString TEST_RESULTS_FILENAME { "TestResults.txt" };

    const double THRESHOLD{ 0.96 };

    QDir _imageDirectory;

    MismatchWindow _mismatchWindow;

    ImageComparer _imageComparer;

    QString _testResultsFolderPath;
    int _index { 1 };

    // Expected images are in the format ExpectedImage_dddd.jpg (d == decimal digit)
    const int NUM_DIGITS { 5 };
    const QString EXPECTED_IMAGE_PREFIX { "ExpectedImage_" };

    // We have two directories to work with.
    // The first is the directory containing the test we are working with
    // The second is the root directory of all tests
    // The third contains the snapshots taken for test runs that need to be evaluated
    QString _testDirectory;
    QString _testsRootDirectory;
    QString _snapshotDirectory;

    QStringList _expectedImagesFilenames;
    QStringList _expectedImagesFullFilenames;
    QStringList _resultImagesFullFilenames;

    // Used for accessing GitHub
    const QString GIT_HUB_REPOSITORY{ "hifi_tests" };

    const QString DATETIME_FORMAT{ "yyyy-MM-dd_hh-mm-ss" };

    // NOTE: these need to match the appropriate var's in autoTester.js
    //    var advanceKey = "n";
    //    var pathSeparator = ".";
    const QString ADVANCE_KEY{ "n" };
    const QString PATH_SEPARATOR{ "." };

    bool _exitWhenComplete{ false };

    TestRailInterface _testRailInterface;

    TestRailCreateMode _testRailCreateMode { PYTHON };
};

#endif // hifi_test_h