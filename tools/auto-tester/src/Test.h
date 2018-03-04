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

class Test {
public: 
    Test();

    void startTestsEvaluation();
    void finishTestsEvaluation(bool interactiveMode, QProgressBar* progressBar);

    void createRecursiveScript();
    void createRecursiveScriptsRecursively();
    void createTest();
    void deleteOldSnapshots();

    bool compareImageLists(bool isInteractiveMode, QProgressBar* progressBar);

    QStringList createListOfAll_IMAGE_FORMAT_imagesInDirectory(QString pathToImageDirectory);

    bool isInSnapshotFilenameFormat(QString filename);

    void importTest(QTextStream& textStream, const QString& testPathname);

    void appendTestResultsToFile(QString testResultsFolderPath, TestFailure testFailure, QPixmap comparisonImage);

    bool createTestResultsFolderPathIfNeeded(QString directory);
    void zipAndDeleteTestResultsFolder();

    bool isAValidDirectory(QString pathname);

    QString getExpectedImageDestinationDirectory(QString filename);
    QString getExpectedImagePartialSourceDirectory(QString filename);

private:
    const QString TEST_FILENAME { "test.js" };
    const QString TEST_RESULTS_FOLDER { "TestResults" };
    const QString TEST_RESULTS_FILENAME { "TestResults.txt" };

    QMessageBox messageBox;

    QDir imageDirectory;

    QRegularExpression expectedImageFilenameFormat;

    MismatchWindow mismatchWindow;

    ImageComparer imageComparer;

    QString testResultsFolderPath { "" };
    int index { 1 };

    // Expected images are in the format ExpectedImage_dddd.jpg (d == decimal digit)
    const int NUM_DIGITS { 5 };
    const QString EXPECTED_IMAGE_PREFIX { "ExpectedImage_" };
    const QString IMAGE_FORMAT { ".png" };

    QString pathToTestResultsDirectory;
    QStringList expectedImagesFilenames;
    QStringList expectedImagesFullFilenames;
    QStringList resultImagesFullFilenames;
};

#endif // hifi_test_h