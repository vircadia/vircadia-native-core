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

Test::Test() {
    snapshotFilenameFormat = QRegularExpression("hifi-snap-by-.+-on-\\d\\d\\d\\d-\\d\\d-\\d\\d_\\d\\d-\\d\\d-\\d\\d.jpg");

    expectedImageFilenameFormat = QRegularExpression("ExpectedImage_\\d+.jpg");

    mismatchWindow.setModal(true);
}

bool Test::compareImageLists(QStringList expectedImages, QStringList resultImages) {
    // Loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    const double THRESHOLD{ 0.999 };
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
            mismatchWindow.setTestFailure(TestFailure{
                (float)similarityIndex,
                expectedImages[i].left(expectedImages[i].lastIndexOf("/") + 1), // path to the test (including trailing /)
                QFileInfo(expectedImages[i].toStdString().c_str()).fileName(),  // filename of expected image
                QFileInfo(resultImages[i].toStdString().c_str()).fileName()     // filename of result image
            });

            mismatchWindow.exec();

            switch (mismatchWindow.getUserResponse()) {
                case USER_RESPONSE_PASS:
                    break;
                case USE_RESPONSE_FAIL:
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

    return success;
}

void Test::evaluateTests() {
    // Get list of JPEG images in folder, sorted by name
    QString pathToImageDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (pathToImageDirectory == "") {
        return;
    }

    QStringList sortedImageFilenames = createListOfAllJPEGimagesInDirectory(pathToImageDirectory);

    // Separate images into two lists.  The first is the expected images, the second is the test results
    // Images that are in the wrong format are ignored.
    QStringList expectedImages;
    QStringList resultImages;
    foreach(QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = pathToImageDirectory + "/" + currentFilename;
        if (isInExpectedImageFilenameFormat(currentFilename)) {
            expectedImages << fullCurrentFilename;
        } else if (isInSnapshotFilenameFormat(currentFilename)) {
            resultImages << fullCurrentFilename;
        }
    }

    // The number of images in each list should be identical
    if (expectedImages.length() != resultImages.length()) {
        messageBox.critical(0, 
            "Test failed", 
            "Found " + QString::number(resultImages.length()) + " images in directory" +
            "\nExpected to find " + QString::number(expectedImages.length()) + " images"
        );

        exit(-1);
    }

    bool success = compareImageLists(expectedImages, resultImages);

    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }
}

// Two criteria are used to decide if a folder contains valid test results.
//      1) a 'test'js' file exists in the folder
//      2) the folder has the same number of actual and expected images
void Test::evaluateTestsRecursively() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    bool success{ true };
    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();
        if (directory[directory.length() - 1] == '.') {
            // ignore '.', '..' directories
            continue;
        }

        // 
        const QString testPathname{ directory + "/" + testFilename };
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
        success &= compareImageLists(expectedImages, resultImages);
    }

    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }
}

void Test::importTest(QTextStream& textStream, const QString& testPathname, int testNumber) {
    textStream << "var test" << testNumber << " = Script.require(\"" << "file:///" << testPathname + "\");" << endl;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    QFile allTestsFilename(topLevelDirectory + "/" + "allTests.js");
    if (!allTestsFilename.open(QIODevice::WriteOnly | QIODevice::Text)) {
        messageBox.critical(0,
            "Internal Error",
            "Failed to create \"allTests.js\" in directory \"" + topLevelDirectory + "\"");

        exit(-1);
    }

    QTextStream textStream(&allTestsFilename);
    textStream << "// This is an automatically generated file, created by auto-tester" << endl;

    // The main will call each test after the previous test is completed
    // This is implemented with an interval timer that periodically tests if a
    // running test has increment a testNumber variable that it received as an input.
    int testNumber = 1;
    QVector<QString> testPathnames;

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + testFilename };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        // Current folder contains a test
        importTest(textStream, testPathname, testNumber);
        ++testNumber;

        testPathnames << testPathname;
    }

    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();
        if (directory[directory.length() - 1] == '.') {
            // ignore '.', '..' directories
            continue;
        }

        const QString testPathname{ directory + "/" + testFilename };
        QFileInfo fileInfo(testPathname);
        if (fileInfo.exists()) {
            // Current folder contains a test
            importTest(textStream, testPathname, testNumber);
            ++testNumber;

            testPathnames << testPathname;
        }
    }

    if (testPathnames.length() <= 0) {
        messageBox.information(0, "Failure", "No \"test.js\" files found");
        allTestsFilename.close();
        return;
    }

    textStream << endl;

    // Define flags for each test
    for (int i = 1; i <= testPathnames.length(); ++i) {
        textStream << "var test" << i << "HasNotStarted = true;" << endl;
    }

    // Leave a blank line in the main
    textStream << endl;

    const int TEST_PERIOD = 1000; // in milliseconds
    const QString tab = "    ";

    textStream << "// Check every second if the current test is complete and the next test can be run" << endl;
    textStream << "var testTimer = Script.setInterval(" << endl;
    textStream << tab << "function() {" << endl;

    const QString testFunction = "test";
    for (int i = 1; i <= testPathnames.length(); ++i) {
        // First test starts immediately, all other tests wait for the previous test to complete.
        // The script produced will look as follows:
        //      if (test1HasNotStarted) {
        //          test1HasNotStarted = false;
        //          test1.test();
        //          print("******started test 1******");
        //      }
        //      |
        //      |
        //      if (test5.complete && test6HasNotStarted) {
        //          test6HasNotStarted = false;
        //          test7.test();
        //          print("******started test 6******");
        //      }
        //      |
        //      |
        //      if (test12.complete) {
        //          print("******stopping******");
        //          Script.stop();
        //      }
        //
        if (i == 1) {
            textStream << tab << tab << "if (test1HasNotStarted) {" << endl;
        } else {
            textStream << tab << tab << "if (test" << i - 1 << ".complete && test" << i << "HasNotStarted) {" << endl;
        }
        textStream << tab << tab << tab << "test" << i << "HasNotStarted = false;" << endl;
        textStream << tab << tab << tab << "test" << i << "." << testFunction << "();" << endl;
        textStream << tab << tab << tab << "print(\"******started test " << i << "******\");" << endl;

        textStream << tab << tab << "}" << endl << endl;

    }

    // Add extra step to stop the script
    textStream << tab << tab << "if (test" << testPathnames.length() << ".complete) {" << endl;
    textStream << tab << tab << tab << "print(\"******stopping******\");" << endl;
    textStream << tab << tab << tab << "Script.stop();" << endl;
    textStream << tab << tab << "}" << endl << endl;

    textStream << tab << "}," << endl;
    textStream << endl;
    textStream << tab << TEST_PERIOD << endl;
    textStream << ");" << endl << endl;

    textStream << "// Stop the timer and clear the module cache" << endl;
    textStream << "Script.scriptEnding.connect(" << endl;
    textStream << tab << "function() {" << endl;
    textStream << tab << tab << "Script.clearInterval(testTimer);" << endl;
    textStream << tab << tab << "Script.require.cache = {};" << endl;
    textStream << tab << "}" << endl;
    textStream << ");" << endl;

    allTestsFilename.close();

    messageBox.information(0, "Success", "Script has been created");
}

void Test::createTest() {
    // Rename files sequentially, as ExpectedResult_1.jpeg, ExpectedResult_2.jpg and so on
    // Any existing expected result images will be deleted
    QString pathToImageDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (pathToImageDirectory == "") {
        return;
    }

    QStringList sortedImageFilenames = createListOfAllJPEGimagesInDirectory(pathToImageDirectory);

    int i = 1;
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = pathToImageDirectory + "/" + currentFilename;
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
            QString newFilename = "ExpectedImage_" + QString::number(i-1).rightJustified(5, '0') + ".jpg";
            QString fullNewFileName = pathToImageDirectory + "/" + newFilename;

            if (!imageDirectory.rename(fullCurrentFilename, newFilename)) {
                if (!QFile::exists(fullCurrentFilename)) {
                    messageBox.critical(0, "Error", "Could not rename file: " + fullCurrentFilename + " to: " + newFilename + "\n"
                        + fullCurrentFilename + " not found"
                        + "\nTest creation aborted"
                    );
                    exit(-1);
                } else {
                    messageBox.critical(0, "Error", "Could not rename file: " + fullCurrentFilename + " to: " + newFilename + "\n"
                        + "unknown error" + "\nTest creation aborted"
                    );
                    exit(-1);
                }
            }
            ++i;
        }
    }

    messageBox.information(0, "Success", "Test images have been created");
}

QStringList Test::createListOfAllJPEGimagesInDirectory(QString pathToImageDirectory) {
    imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*.jpg";

    return imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

bool Test::isInSnapshotFilenameFormat(QString filename) {
    return (snapshotFilenameFormat.match(filename).hasMatch());
}

bool Test::isInExpectedImageFilenameFormat(QString filename) {
    return (expectedImageFilenameFormat.match(filename).hasMatch());
}