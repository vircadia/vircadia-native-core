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

void Test::evaluateTests() {
    createListOfAllJPEGimagesInDirectory();

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

    // Now loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    const double THRESHOLD{ 0.99999 };
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
                expectedImages[i].left(expectedImages[i].lastIndexOf("/") + 1), // path to the test (including trailing /
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
            }
        }
    }

    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);

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
    textStream << "var testNumber = 1;" << endl << endl;

    // Components of the test path are stored as these are used to call each specific test
    // Each test has a unique "run test" function.  The name of this function depends on the path to the test
    // For example:
    //      Test:           D:\GitHub\hifi-tests\tests\content\entity\zone\ambientLightInheritance\test.js
    //      Function name:  tests_content_entity_zone_ambientLightInheritance()
    // The last occurance of "tests" is assumed to be the top of the test hierarchy
    //
    QVector<QStringList> testPathComponents;

    const QString testFilename{ "test.js" };

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + testFilename };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        // Current folder contains a test
        textStream << "Script.include(\"" << testPathname + "\");" << endl;

        testPathComponents << testPathname.split('/');
    }

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
            textStream << "Script.include(\"" << testPathname + "\");" << endl;

            testPathComponents << testPathname.split('/');
        }
    }

    if (testPathComponents.length() <= 0) {
        messageBox.information(0, "Failure", "No \"test.js\" files found");
        allTestsFilename.close();
        return;
    }

    // Find last occurance of tests in the first testPathname
    int firstComponent{ 0 };
    for (int i = testPathComponents[0].length() - 1; i >= 0; --i) {
        if (testPathComponents[0][i] == "tests") {
            firstComponent = i;
            break;
        }
    }

    // Leave a blank line in the main
    textStream << endl;

    const int TEST_PERIOD = 1000; // in milliseconds
    QString tab = "    ";

    textStream << "var testTimer = Script.setInterval(" << endl;
    textStream << tab << "function() {" << endl;

    int stepNumber = 1;
    for (QStringList testPathComponent : testPathComponents) {
        QString testName = "tests";
        for (int i = firstComponent + 1; i < testPathComponent.length() - 1; ++i) {
            testName += "_" + testPathComponent[i];
        }
        
        textStream << tab << tab << "if (testNumber == " << stepNumber << ") {" << endl;
        textStream << tab << tab << tab << testName << "();" << endl;

        // Set stepNumber to 0 between tests to stop the same test being called twice
        textStream << tab << tab << tab << "testNumber = 0;" << endl;

        textStream << tab << tab << "}" << endl << endl;

        ++stepNumber;
    }

    // Add extra step to stop the script
    textStream << tab << tab << "if (testNumber == " << stepNumber << ") {" << endl;
    textStream << tab << tab << tab << "Script.stop();" << endl;
    textStream << tab << tab << "}" << endl << endl;

    textStream << tab << "}," << endl;
    textStream << endl;
    textStream << tab << TEST_PERIOD << endl;
    textStream << ");" << endl << endl;

    textStream << "Script.scriptEnding.connect(" << endl;
    textStream << tab << "function() {" << endl;
    textStream << tab << tab << "Script.clearInterval(testTimer);" << endl;
    textStream << tab << "}" << endl;
    textStream << ");" << endl;

    allTestsFilename.close();

    messageBox.information(0, "Success", "Script has been created");
}

void Test::createTest() {
    // Rename files sequentially, as ExpectedResult_1.jpeg, ExpectedResult_2.jpg and so on
    // Any existing expected result images will be deleted
    createListOfAllJPEGimagesInDirectory();

    int i = 1;
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = pathToImageDirectory + "/" + currentFilename;
        if (isInExpectedImageFilenameFormat(currentFilename)) {
            if (!QFile::remove(fullCurrentFilename)) {
                messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nTest creation aborted");
                exit(-1);
            }
        } else if (isInSnapshotFilenameFormat(currentFilename)) {
            QString newFilename = "ExpectedImage_" + QString::number(i) + ".jpg";
            QString fullNewFileName = pathToImageDirectory + "/" + newFilename;

            imageDirectory.rename(fullCurrentFilename, newFilename);
            ++i;
        }
    }

    messageBox.information(0, "Success", "Test images have been created");
}

void Test::createListOfAllJPEGimagesInDirectory() {
    // Get list of JPEG images in folder, sorted by name
    pathToImageDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);

    imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*.jpg";

    sortedImageFilenames = imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

bool Test::isInSnapshotFilenameFormat(QString filename) {
    return (snapshotFilenameFormat.match(filename).hasMatch());
}

bool Test::isInExpectedImageFilenameFormat(QString filename) {
    return (expectedImageFilenameFormat.match(filename).hasMatch());
}