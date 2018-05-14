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

class Test {
public: 
    Test();

    void startTestsEvaluation(const QString& testFolder = QString());
    void finishTestsEvaluation(bool isRunningFromCommandline, bool interactiveMode, QProgressBar* progressBar);

    void createRecursiveScript();
    void createAllRecursiveScripts();
    void createRecursiveScript(const QString& topLevelDirectory, bool interactiveMode);

    void createTest();
    void createMDFile();
    void createAllMDFiles();
    void createMDFile(const QString& topLevelDirectory);

    void createTestsOutline();

    bool compareImageLists(bool isInteractiveMode, QProgressBar* progressBar);

    QStringList createListOfAll_imagesInDirectory(const QString& imageFormat, const QString& pathToImageDirectory);

    bool isInSnapshotFilenameFormat(const QString& imageFormat, const QString& filename);

    void importTest(QTextStream& textStream, const QString& testPathname);

    void appendTestResultsToFile(const QString& testResultsFolderPath, TestFailure testFailure, QPixmap comparisonImage);

    bool createTestResultsFolderPath(const QString& directory);
    void zipAndDeleteTestResultsFolder();

    bool isAValidDirectory(const QString& pathname);
	QString extractPathFromTestsDown(const QString& fullPath);
    QString getExpectedImageDestinationDirectory(const QString& filename);
    QString getExpectedImagePartialSourceDirectory(const QString& filename);

    void copyJPGtoPNG(const QString& sourceJPGFullFilename, const QString& destinationPNGFullFilename);

private:
    const QString TEST_FILENAME { "test.js" };
    const QString TEST_RESULTS_FOLDER { "TestResults" };
    const QString TEST_RESULTS_FILENAME { "TestResults.txt" };

    QDir imageDirectory;

    MismatchWindow mismatchWindow;

    ImageComparer imageComparer;

    QString testResultsFolderPath;
    int index { 1 };

    // Expected images are in the format ExpectedImage_dddd.jpg (d == decimal digit)
    const int NUM_DIGITS { 5 };
    const QString EXPECTED_IMAGE_PREFIX { "ExpectedImage_" };

    // We have two directories to work with.
    // The first is the directory containing the test we are working with
    // The second contains the snapshots taken for test runs that need to be evaluated
    QString testDirectory;
    QString snapshotDirectory;

    QStringList expectedImagesFilenames;
    QStringList expectedImagesFullFilenames;
    QStringList resultImagesFullFilenames;

    // Used for accessing GitHub
    const QString githubUser{ "highfidelity" };
    const QString gitHubBranch { "master" };
	const QString DATETIME_FORMAT { "yyyy-MM-dd_hh-mm-ss" };

	ExtractedText getTestScriptLines(QString testFileName);
};

#endif // hifi_test_h