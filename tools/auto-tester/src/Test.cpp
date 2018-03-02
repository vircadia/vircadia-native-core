//
//  Test.cpp
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Test.h"

#include <assert.h>
#include <QtCore/QTextStream>
#include <QDirIterator>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

Test::Test() {
    QString regex(EXPECTED_IMAGE_PREFIX + QString("\\\\d").repeated(NUM_DIGITS) + EXPECTED_IMAGE_TYPE);
    
    expectedImageFilenameFormat = QRegularExpression(regex);

    mismatchWindow.setModal(true);
}

bool Test::createTestResultsFolderPathIfNeeded(QString directory) {
    // The test results folder is located in the root of the tests (i.e. for recursive test evaluation)
    if (testResultsFolderPath == "") {
        testResultsFolderPath =  directory + "/" + TEST_RESULTS_FOLDER;
        QDir testResultsFolder(testResultsFolderPath);

        if (testResultsFolder.exists()) {
            testResultsFolder.removeRecursively();
        }

        // Create a new test results folder
        return QDir().mkdir(testResultsFolderPath);
    } else {
        return true;
    }
}

void Test::zipAndDeleteTestResultsFolder() {
    QString zippedResultsFileName { testResultsFolderPath + ".zip" };
    QFileInfo fileInfo(zippedResultsFileName);
    if (!fileInfo.exists()) {
        QFile::remove(zippedResultsFileName);
    }

    QDir testResultsFolder(testResultsFolderPath);
    if (!testResultsFolder.isEmpty()) {
        JlCompress::compressDir(testResultsFolderPath + ".zip", testResultsFolderPath);
    }

    testResultsFolder.removeRecursively();

    //In all cases, for the next evaluation
    testResultsFolderPath = "";
    index = 1;
}

bool Test::compareImageLists(QStringList expectedImages, QStringList resultImages, QString testDirectory, bool interactiveMode, QProgressBar* progressBar) {
    progressBar->setMinimum(0);
    progressBar->setMaximum(expectedImages.length() - 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    // Loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    const double THRESHOLD { 0.999 };
    bool success{ true };
    bool keepOn{ true };
    for (int i = 0; keepOn && i < expectedImages.length(); ++i) {
        // First check that images are the same size
        QImage resultImage(resultImages[i]);
        QImage expectedImage(expectedImages[i]);
        if (resultImage.width() != expectedImage.width() || resultImage.height() != expectedImage.height()) {
            messageBox.critical(0, "Internal error", "Images are not the same size");
            exit(-1);
        }

        double similarityIndex;  // in [-1.0 .. 1.0], where 1.0 means images are identical
        try {
            similarityIndex = imageComparer.compareImages(resultImage, expectedImage);
        } catch (...) {
            messageBox.critical(0, "Internal error", "Image not in expected format");
            exit(-1);
        }

        if (similarityIndex < THRESHOLD) {
            TestFailure testFailure = TestFailure{
                (float)similarityIndex,
                expectedImages[i].left(expectedImages[i].lastIndexOf("/") + 1), // path to the test (including trailing /)
                QFileInfo(expectedImages[i].toStdString().c_str()).fileName(),  // filename of expected image
                QFileInfo(resultImages[i].toStdString().c_str()).fileName()     // filename of result image
            };

            mismatchWindow.setTestFailure(testFailure);

            if (!interactiveMode) {
                appendTestResultsToFile(testResultsFolderPath, testFailure, mismatchWindow.getComparisonImage());
                success = false;
            } else {
                mismatchWindow.exec();

                switch (mismatchWindow.getUserResponse()) {
                    case USER_RESPONSE_PASS:
                        break;
                    case USE_RESPONSE_FAIL:
                        appendTestResultsToFile(testResultsFolderPath, testFailure, mismatchWindow.getComparisonImage());
                        success = false;
                        break;
                    case USER_RESPONSE_ABORT:
                        keepOn = false;
                        success = false;
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }

        progressBar->setValue(i);
    }

    progressBar->setVisible(false);
    return success;
}

void Test::appendTestResultsToFile(QString testResultsFolderPath, TestFailure testFailure, QPixmap comparisonImage) {
    if (!QDir().exists(testResultsFolderPath)) {
        messageBox.critical(0, "Internal error", "Folder " + testResultsFolderPath + " not found");
        exit(-1);
    }

    QString failureFolderPath { testResultsFolderPath + "/" + "Failure_" + QString::number(index) };
    if (!QDir().mkdir(failureFolderPath)) {
        messageBox.critical(0, "Internal error", "Failed to create folder " + failureFolderPath);
        exit(-1);
    }
    ++index;

    QFile descriptionFile(failureFolderPath + "/" + TEST_RESULTS_FILENAME);
    if (!descriptionFile.open(QIODevice::ReadWrite)) {
        messageBox.critical(0, "Internal error", "Failed to create file " + TEST_RESULTS_FILENAME);
        exit(-1);
    }

    // Create text file describing the failure
    QTextStream stream(&descriptionFile);
    stream << "Test failed in folder " << testFailure._pathname.left(testFailure._pathname.length() - 1) << endl; // remove trailing '/'
    stream << "Expected image was    " << testFailure._expectedImageFilename << endl;
    stream << "Actual image was      " << testFailure._actualImageFilename << endl;
    stream << "Similarity index was  " << testFailure._error << endl;

    descriptionFile.close();

    // Copy expected and actual images, and save the difference image
    QString sourceFile;
    QString destinationFile;

    sourceFile = testFailure._pathname + testFailure._expectedImageFilename;
    destinationFile = failureFolderPath + "/" + "Expected Image.jpg";
    if (!QFile::copy(sourceFile, destinationFile)) {
        messageBox.critical(0, "Internal error", "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    sourceFile = testFailure._pathname + testFailure._actualImageFilename;
    destinationFile = failureFolderPath + "/" + "Actual Image.jpg";
    if (!QFile::copy(sourceFile, destinationFile)) {
        messageBox.critical(0, "Internal error", "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    comparisonImage.save(failureFolderPath + "/" + "Difference Image.jpg");
}

void Test::evaluateTests(bool interactiveMode, QProgressBar* progressBar) {
    // Get list of JPEG images in folder, sorted by name
    QString pathToTestResultsDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (pathToTestResultsDirectory == "") {
        return;
    }

    // Leave if test results folder could not be created
    if (!createTestResultsFolderPathIfNeeded(pathToTestResultsDirectory)) {
        return;
    }

    // Create two lists.  The first is the test results,  the second is the expected images
    // Images that are in the wrong format are ignored.

    QStringList sortedTestResultsFilenames = createListOfAllJPEGimagesInDirectory(pathToTestResultsDirectory);
    QStringList expectedImages;
    QStringList resultImages;
    foreach(QString currentFilename, sortedTestResultsFilenames) {
        QString fullCurrentFilename = pathToTestResultsDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat(currentFilename)) {
            resultImages << fullCurrentFilename;

            QString expectedImageDirectory = getExpectedImageDestinationDirectory(currentFilename);
            
            // extract the digits at the end of the filename (exluding the file extension)
            QString expectedImageFilenameTail = currentFilename.left(currentFilename.length() - 4).right(NUM_DIGITS);

            QString expectedImageFilename = EXPECTED_IMAGE_PREFIX + expectedImageFilenameTail + EXPECTED_IMAGE_TYPE;
            expectedImages << (expectedImageDirectory + "/" + expectedImageFilename);
        }
    }

    autoTester->downloadImage(QUrl("http://ribafreixo.com/wp-content/uploads/2017/03/Order-Now-Button-300x113.png"));
    ////bool success = compareImageLists(expectedImages, resultImages, pathToImageDirectory, interactiveMode, progressBar);

    ////if (success) {
    ////    messageBox.information(0, "Success", "All images are as expected");
    ////} else {
    ////    messageBox.information(0, "Failure", "One or more images are not as expected");
    ////}

    zipAndDeleteTestResultsFolder();
}

bool Test::isAValidDirectory(QString pathname) {
    // Only process directories
    QDir dir(pathname);
    if (!dir.exists()) {
        return false;
    }

    // Ignore '.', '..' directories
    if (pathname[pathname.length() - 1] == '.') {
        return false;
    }

    return true;
}

// Two criteria are used to decide if a folder contains valid test results.
//      1) a 'test'js' file exists in the folder
//      2) the folder has the same number of actual and expected images
void Test::evaluateTestsRecursively(bool interactiveMode, QProgressBar* progressBar) {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    // Leave if test results folder could not be created
    if (!createTestResultsFolderPathIfNeeded(topLevelDirectory)) {
        return;
    }

    bool success{ true };
    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        if (!isAValidDirectory(directory)) {
            continue;
        }

        const QString testPathname{ directory + "/" + TEST_FILENAME };
        QFileInfo fileInfo(testPathname);
        if (!fileInfo.exists()) {
            // Folder does not contain 'test.js'
            continue;
        }

        QStringList sortedImageFilenames = createListOfAllJPEGimagesInDirectory(directory);

        // Separate images into two lists.  The first is the expected images, the second is the test results
        // Images that are in the wrong format are ignored.
        QStringList expectedImages;
        QStringList resultImages;
        foreach(QString currentFilename, sortedImageFilenames) {
            QString fullCurrentFilename = directory + "/" + currentFilename;
            if (isInExpectedImageFilenameFormat(currentFilename)) {
                expectedImages << fullCurrentFilename;
            } else if (isInSnapshotFilenameFormat(currentFilename)) {
                resultImages << fullCurrentFilename;
            }
        }

        if (expectedImages.length() != resultImages.length()) {
            // Number of images doesn't match
            continue;
        }

        // Set success to false if any test has failed
        success &= compareImageLists(expectedImages, resultImages, directory, interactiveMode, progressBar);
    }

    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }

    zipAndDeleteTestResultsFolder();
}

void Test::importTest(QTextStream& textStream, const QString& testPathname) {
    textStream << "Script.include(\"" << "file:///" << testPathname + "?raw=true\");" << endl;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    const QString recursiveTestsFilename("testRecursive.js");
    QFile allTestsFilename(topLevelDirectory + "/" + recursiveTestsFilename);
    if (!allTestsFilename.open(QIODevice::WriteOnly | QIODevice::Text)) {
        messageBox.critical(0,
            "Internal Error",
            "Failed to create \"" + recursiveTestsFilename + "\" in directory \"" + topLevelDirectory + "\""
        );

        exit(-1);
    }

    QTextStream textStream(&allTestsFilename);
    textStream << "// This is an automatically generated file, created by auto-tester" << endl << endl;

    textStream << "var autoTester = Script.require(\"https://github.com/highfidelity/hifi_tests/blob/master/tests/utils/autoTester.js?raw=true\");" << endl;
    textStream << "autoTester.enableRecursive();" << endl << endl;

    QVector<QString> testPathnames;

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        // Current folder contains a test
        importTest(textStream, testPathname);

        testPathnames << testPathname;
    }

    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir(directory);
        if (!isAValidDirectory(directory)) {
            continue;
        }

        const QString testPathname{ directory + "/" + TEST_FILENAME };
        QFileInfo fileInfo(testPathname);
        if (fileInfo.exists()) {
            // Current folder contains a test
            importTest(textStream, testPathname);

            testPathnames << testPathname;
        }
    }

    if (testPathnames.length() <= 0) {
        messageBox.information(0, "Failure", "No \"" + TEST_FILENAME + "\" files found");
        allTestsFilename.close();
        return;
    }

    textStream << endl;
    textStream << "autoTester.runRecursive();" << endl;

    allTestsFilename.close();
    messageBox.information(0, "Success", "Script has been created");
}

void Test::createRecursiveScriptsRecursively() {
}

void Test::createTest() {
    // Rename files sequentially, as ExpectedResult_1.jpeg, ExpectedResult_2.jpg and so on
    // Any existing expected result images will be deleted
    QString imageSourceDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (imageSourceDirectory == "") {
        return;
    }

    QStringList sortedImageFilenames = createListOfAllJPEGimagesInDirectory(imageSourceDirectory);

    int i = 1;
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = imageSourceDirectory + "/" + currentFilename;
        if (isInExpectedImageFilenameFormat(currentFilename)) {
            if (!QFile::remove(fullCurrentFilename)) {
                messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nTest creation aborted");
                exit(-1);
            }
        } else if (isInSnapshotFilenameFormat(currentFilename)) {
            const int MAX_IMAGES = 100000;
            if (i >= MAX_IMAGES) {
                messageBox.critical(0, "Error", "More than 100,000 images not supported");
                exit(-1);
            }
            QString newFilename = "ExpectedImage_" + QString::number(i - 1).rightJustified(5, '0') + ".jpg";
            QString imageDestinationDirectory = getExpectedImageDestinationDirectory(currentFilename);
            QString fullNewFileName = imageDestinationDirectory + "/" + newFilename;

            try {
                QFile::copy(fullCurrentFilename, fullNewFileName);
            } catch (...) {
                messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nTest creation aborted");
                exit(-1);
            }
            ++i;
        }
    }

    messageBox.information(0, "Success", "Test images have been created");
}

void Test::deleteOldSnapshots() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select root folder for snapshot deletion", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    // Recurse over folders
    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir(directory);
        if (!isAValidDirectory(directory)) {
            continue;
        }

        QStringList sortedImageFilenames = createListOfAllJPEGimagesInDirectory(directory);

        // Delete any file that is a snapshot (NOT the Expected Images)
        QStringList expectedImages;
        QStringList resultImages;
        foreach(QString currentFilename, sortedImageFilenames) {
            QString fullCurrentFilename = directory + "/" + currentFilename;
            if (isInSnapshotFilenameFormat(currentFilename)) {
                if (!QFile::remove(fullCurrentFilename)) {
                    messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nSnapshot deletion aborted");
                    exit(-1);
                }
            }
        }
    }
}

QStringList Test::createListOfAllJPEGimagesInDirectory(QString pathToImageDirectory) {
    imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*.jpg";

    return imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

// Snapshots are files in the following format:
//      Filename contains no periods (excluding period before exception
//      Filename (i.e. without extension) contains _tests_ (this is based on all test scripts being within the tests folder
//      Last 5 characters in filename are digits
//      Extension is jpg
bool Test::isInSnapshotFilenameFormat(QString filename) {
    QStringList filenameParts = filename.split(".");

    bool filnameHasNoPeriods = (filenameParts.size() == 2);
    bool contains_tests = filenameParts[0].contains("_tests_");

    bool last5CharactersAreDigits;
    filenameParts[0].right(5).toInt(&last5CharactersAreDigits, 10);

    bool extensionIsJPG = filenameParts[1] == "jpg";

    return (filnameHasNoPeriods && contains_tests && last5CharactersAreDigits && extensionIsJPG);
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the test directory is
// D:/GitHub/hifi-tests/tests/content/entity/zone/create
// This method assumes the filename is in the correct format
// The final part of the filename is the image number.  This is checked for sanity
QString Test::getExpectedImageDestinationDirectory(QString filename) {
    QString filenameWithoutExtension = filename.split(".")[0];
    QStringList filenameParts = filenameWithoutExtension.split("_");

    QString result = filenameParts[0] + ":";

    for (int i = 1; i < filenameParts.length() - 1; ++i) {
        result += "/" + filenameParts[i];
    }

    return result;
}

bool Test::isInExpectedImageFilenameFormat(QString filename) {
    return (expectedImageFilenameFormat.match(filename).hasMatch());
}