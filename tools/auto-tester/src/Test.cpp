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
#include <QImageReader>
#include <QImageWriter>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

#include <math.h>

Test::Test() {
    mismatchWindow.setModal(true);
}

bool Test::createTestResultsFolderPath(QString directory) {
    QDateTime now = QDateTime::currentDateTime();
    testResultsFolderPath =  directory + "/" + TEST_RESULTS_FOLDER + "--" + now.toString(DATETIME_FORMAT);
    QDir testResultsFolder(testResultsFolderPath);

    // Create a new test results folder
    return QDir().mkdir(testResultsFolderPath);
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

bool Test::compareImageLists(bool isInteractiveMode, QProgressBar* progressBar) {
    progressBar->setMinimum(0);
    progressBar->setMaximum(expectedImagesFullFilenames.length() - 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    // Loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    const double THRESHOLD { 0.999 };
    bool success{ true };
    bool keepOn{ true };
    for (int i = 0; keepOn && i < expectedImagesFullFilenames.length(); ++i) {
        // First check that images are the same size
        QImage resultImage(resultImagesFullFilenames[i]);
        QImage expectedImage(expectedImagesFullFilenames[i]);

        if (resultImage.width() != expectedImage.width() || resultImage.height() != expectedImage.height()) {
            messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Images are not the same size");
            exit(-1);
        }

        double similarityIndex;  // in [-1.0 .. 1.0], where 1.0 means images are identical
        try {
            similarityIndex = imageComparer.compareImages(resultImage, expectedImage);
        } catch (...) {
            messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Image not in expected format");
            exit(-1);
        }

        if (similarityIndex < THRESHOLD) {
            TestFailure testFailure = TestFailure{
                (float)similarityIndex,
                expectedImagesFullFilenames[i].left(expectedImagesFullFilenames[i].lastIndexOf("/") + 1), // path to the test (including trailing /)
                QFileInfo(expectedImagesFullFilenames[i].toStdString().c_str()).fileName(),  // filename of expected image
                QFileInfo(resultImagesFullFilenames[i].toStdString().c_str()).fileName()     // filename of result image
            };

            mismatchWindow.setTestFailure(testFailure);

            if (!isInteractiveMode) {
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
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Folder " + testResultsFolderPath + " not found");
        exit(-1);
    }

    QString failureFolderPath { testResultsFolderPath + "/" + "Failure_" + QString::number(index) };
    if (!QDir().mkdir(failureFolderPath)) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create folder " + failureFolderPath);
        exit(-1);
    }
    ++index;

    QFile descriptionFile(failureFolderPath + "/" + TEST_RESULTS_FILENAME);
    if (!descriptionFile.open(QIODevice::ReadWrite)) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + TEST_RESULTS_FILENAME);
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
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    sourceFile = testFailure._pathname + testFailure._actualImageFilename;
    destinationFile = failureFolderPath + "/" + "Actual Image.jpg";
    if (!QFile::copy(sourceFile, destinationFile)) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    comparisonImage.save(failureFolderPath + "/" + "Difference Image.jpg");
}

void Test::startTestsEvaluation() {
    // Get list of JPEG images in folder, sorted by name
    pathToTestResultsDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (pathToTestResultsDirectory == "") {
        return;
    }

    // Quit if test results folder could not be created
    if (!createTestResultsFolderPath(pathToTestResultsDirectory)) {
        return;
    }

    // Before any processing - all images are converted to PNGs, as this is the format stored on GitHub
    QStringList sortedSnapshotFilenames = createListOfAll_imagesInDirectory("jpg", pathToTestResultsDirectory);
    foreach(QString filename, sortedSnapshotFilenames) {
        QStringList stringParts = filename.split(".");
        copyJPGtoPNG(
            pathToTestResultsDirectory + "/" + stringParts[0] + ".jpg", 
            pathToTestResultsDirectory + "/" + stringParts[0] + ".png"
        );

        QFile::remove(pathToTestResultsDirectory + "/" + stringParts[0] + ".jpg");
    }

    // Create two lists.  The first is the test results,  the second is the expected images
    // The expected images are represented as a URL to enable download from GitHub
    // Images that are in the wrong format are ignored.

    QStringList sortedTestResultsFilenames = createListOfAll_imagesInDirectory("png", pathToTestResultsDirectory);
    QStringList expectedImagesURLs;

    resultImagesFullFilenames.clear();
    expectedImagesFilenames.clear();
    expectedImagesFullFilenames.clear();

    foreach(QString currentFilename, sortedTestResultsFilenames) {
        QString fullCurrentFilename = pathToTestResultsDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat("png", currentFilename)) {
            resultImagesFullFilenames << fullCurrentFilename;

            QString expectedImagePartialSourceDirectory = getExpectedImagePartialSourceDirectory(currentFilename);

            // Images are stored on GitHub as ExpectedImage_ddddd.png
            // Extract the digits at the end of the filename (excluding the file extension)
            QString expectedImageFilenameTail = currentFilename.left(currentFilename.length() - 4).right(NUM_DIGITS);
            QString expectedImageStoredFilename = EXPECTED_IMAGE_PREFIX + expectedImageFilenameTail + ".png";

            QString imageURLString("https://github.com/" + githubUser + "/hifi_tests/blob/" + gitHubBranch + "/" + 
                expectedImagePartialSourceDirectory + "/" + expectedImageStoredFilename + "?raw=true");

            expectedImagesURLs << imageURLString;

            // The image retrieved from GitHub needs a unique name
            QString expectedImageFilename = currentFilename.replace("/", "_").replace(".", "_EI.");

            expectedImagesFilenames << expectedImageFilename;
            expectedImagesFullFilenames << pathToTestResultsDirectory + "/" + expectedImageFilename;
        }
    }

    autoTester->downloadImages(expectedImagesURLs, pathToTestResultsDirectory, expectedImagesFilenames);
}

void Test::finishTestsEvaluation(bool interactiveMode, QProgressBar* progressBar) {
    bool success = compareImageLists(interactiveMode, progressBar);
    
    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }

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

QString Test::extractPathFromTestsDown(QString fullPath) {
    // `fullPath` includes the full path to the test.  We need the portion below (and including) `tests`
    QStringList pathParts = fullPath.split('/');
    int i{ 0 };
    while (i < pathParts.length() && pathParts[i] != "tests") {
        ++i;
    }

    if (i == pathParts.length()) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Bad testPathname");
        exit(-1);
    }

    QString partialPath;
    for (int j = i; j < pathParts.length(); ++j) {
        partialPath += "/" + pathParts[j];
    }

    return partialPath;
}

void Test::importTest(QTextStream& textStream, const QString& testPathname) {
    QString partialPath = extractPathFromTestsDown(testPathname);
    textStream << "Script.include(\"" << "https://github.com/" << githubUser
               << "/hifi_tests/blob/" << gitHubBranch << partialPath + "?raw=true\");" << endl;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    createRecursiveScript(topLevelDirectory, true);
}

// This method creates a `testRecursive.js` script in every sub-folder.
void Test::createAllRecursiveScripts() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select the root folder for the recursive scripts", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    createRecursiveScript(topLevelDirectory, false);

    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir;
        if (!isAValidDirectory(directory)) {
            continue;
        }

        // Only process directories that have sub-directories
        bool hasNoSubDirectories{ true };
        QDirIterator it2(directory.toStdString().c_str(), QDirIterator::Subdirectories);
        while (it2.hasNext()) {
            QString directory2 = it2.next();

            // Only process directories
            QDir dir;
            if (isAValidDirectory(directory2)) {
                hasNoSubDirectories = false;
                break;
            }
        }

        if (!hasNoSubDirectories) {
            createRecursiveScript(directory, false);
        }
    }

    messageBox.information(0, "Success", "Scripts have been created");
}

void Test::createRecursiveScript(QString topLevelDirectory, bool interactiveMode) {
    const QString recursiveTestsFilename("testRecursive.js");
    QFile allTestsFilename(topLevelDirectory + "/" + recursiveTestsFilename);
    if (!allTestsFilename.open(QIODevice::WriteOnly | QIODevice::Text)) {
        messageBox.critical(0,
            "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Failed to create \"" + recursiveTestsFilename + "\" in directory \"" + topLevelDirectory + "\""
        );

        exit(-1);
    }

    QTextStream textStream(&allTestsFilename);
    textStream << "// This is an automatically generated file, created by auto-tester" << endl << endl;

    textStream << "var autoTester = Script.require(\"https://github.com/" + githubUser + "/hifi_tests/blob/" 
        + gitHubBranch + "/tests/utils/autoTester.js?raw=true\");" << endl;

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

        const QString testPathname { directory + "/" + TEST_FILENAME };
        QFileInfo fileInfo(testPathname);
        if (fileInfo.exists()) {
            // Current folder contains a test
            importTest(textStream, testPathname);

            testPathnames << testPathname;
        }
    }

    if (interactiveMode && testPathnames.length() <= 0) {
        messageBox.information(0, "Failure", "No \"" + TEST_FILENAME + "\" files found");
        allTestsFilename.close();
        return;
    }

    textStream << endl;
    textStream << "autoTester.runRecursive();" << endl;

    allTestsFilename.close();
    
    if (interactiveMode) {
        messageBox.information(0, "Success", "Script has been created");
    }
}

void Test::createTest() {
    // Rename files sequentially, as ExpectedResult_00000.jpeg, ExpectedResult_00001.jpg and so on
    // Any existing expected result images will be deleted
    QString imageSourceDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (imageSourceDirectory == "") {
        return;
    }

    QString imageDestinationDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder to save the test images", ".", QFileDialog::ShowDirsOnly);
    if (imageDestinationDirectory == "") {
        return;
    }

    QStringList sortedImageFilenames = createListOfAll_imagesInDirectory("jpg", imageSourceDirectory);

    int i = 1; 
    const int maxImages = pow(10, NUM_DIGITS);
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = imageSourceDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat("jpg", currentFilename)) {
            if (i >= maxImages) {
                messageBox.critical(0, "Error", "More than " + QString::number(maxImages) + " images not supported");
                exit(-1);
            }
            QString newFilename = "ExpectedImage_" + QString::number(i - 1).rightJustified(5, '0') + ".png";
            QString fullNewFileName = imageDestinationDirectory + "/" + newFilename;

            try {
                copyJPGtoPNG(fullCurrentFilename, fullNewFileName);
            } catch (...) {
                messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nTest creation aborted");
                exit(-1);
            }
            ++i;
        }
    }

    messageBox.information(0, "Success", "Test images have been created");
}

ExtractedText Test::getTestScriptLines(QString testFileName) {
    ExtractedText relevantTextFromTest;

    QFile inputFile(testFileName);
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
        messageBox.critical(0,
            "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Failed to open \"" + testFileName
        );
    }

    QTextStream stream(&inputFile);
    QString line = stream.readLine();

    // Name of test is the string in the following line:
    //        autoTester.perform("Apply Material Entities to Avatars", Script.resolvePath("."), function(testType) {...
    const QString ws("\\h*");    //white-space character
    const QString functionPerformName(ws + "autoTester" + ws + "\\." + ws + "perform");
    const QString quotedString("\\\".+\\\"");
    const QString ownPath("Script" + ws + "\\." + ws + "resolvePath" + ws + "\\(" + ws + "\\\"\\.\\\"" + ws + "\\)");
    const QString functionParameter("function" + ws + "\\(testType" + ws + "\\)");
    QString regexTestTitle(ws + functionPerformName + "\\(" + quotedString + "\\," + ws + ownPath + "\\," + ws + functionParameter + ws + "{" + ".*");
    QRegularExpression lineContainingTitle = QRegularExpression(regexTestTitle);

    // Assert platform checks that test is running on the correct OS
    const QString functionAssertPlatform(ws + "autoTester" + ws + "\\." + ws + "assertPlatform");
    const QString regexAssertPlatform(ws + functionAssertPlatform + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineAssertPlatform = QRegularExpression(regexAssertPlatform);

    // Assert display checks that test is running on the correct display
    const QString functionAssertDisplay(ws + "autoTester" + ws + "\\." + ws + "assertDisplay");
    const QString regexAssertDisplay(ws + functionAssertDisplay + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineAssertDisplay = QRegularExpression(regexAssertDisplay);

    // Assert CPU checks that test is running on the correct type of CPU 
    const QString functionAssertCPU(ws + "autoTester" + ws + "\\." + ws + "assertCPU");
    const QString regexAssertCPU(ws + functionAssertCPU + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineAssertCPU = QRegularExpression(regexAssertCPU);

    // Assert GPU checks that test is running on the correct type of GPU 
    const QString functionAssertGPU(ws + "autoTester" + ws + "\\." + ws + "assertGPU");
    const QString regexAssertGPU(ws + functionAssertGPU + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineAssertGPU = QRegularExpression(regexAssertGPU);

    // Each step is either of the following forms:
    //        autoTester.addStepSnapshot("Take snapshot"...
    //        autoTester.addStep("Clean up after test"...
    const QString functionAddStepSnapshotName(ws + "autoTester" + ws + "\\." + ws + "addStepSnapshot");
    const QString regexStepSnapshot(ws + functionAddStepSnapshotName + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineStepSnapshot = QRegularExpression(regexStepSnapshot);

    const QString functionAddStepName(ws + "autoTester" + ws + "\\." + ws + "addStep");
    const QString regexStep(ws + functionAddStepName + ws + "\\(" + ws + quotedString + ws + "\\)" + ".*");
    const QRegularExpression lineStep = QRegularExpression(regexStep);

    while (!line.isNull()) {
        line = stream.readLine();
        if (lineContainingTitle.match(line).hasMatch()) {
            QStringList tokens = line.split('"');
            relevantTextFromTest.title = tokens[1];
        } else if (lineAssertPlatform.match(line).hasMatch()) {
            QStringList platforms = line.split('"');
            relevantTextFromTest.platform = platforms[1];
        } else if (lineAssertDisplay.match(line).hasMatch()) {
            QStringList displays = line.split('"');
            relevantTextFromTest.display = displays[1];
        } else if (lineAssertCPU.match(line).hasMatch()) {
            QStringList cpus = line.split('"');
            relevantTextFromTest.cpu = cpus[1];
        } else if (lineAssertGPU.match(line).hasMatch()) {
            QStringList gpus = line.split('"');
            relevantTextFromTest.gpu = gpus[1];
        } else if (lineStepSnapshot.match(line).hasMatch()) {
            QStringList tokens = line.split('"');
            QString nameOfStep = tokens[1];

            Step *step = new Step();
            step->text = nameOfStep;
            step->takeSnapshot = true;
            relevantTextFromTest.stepList.emplace_back(step);
        } else if (lineStep.match(line).hasMatch()) {
            QStringList tokens = line.split('"');
            QString nameOfStep = tokens[1];

            Step *step = new Step();
            step->text = nameOfStep;
            step->takeSnapshot = false;
            relevantTextFromTest.stepList.emplace_back(step);
        }
    }

    inputFile.close();

    return relevantTextFromTest;
}

// Create an MD file for a user-selected test.
// The folder selected must contain a script named "test.js", the file produced is named "test.md"
void Test::createMDFile() {
    // Folder selection
    QString testDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test", ".", QFileDialog::ShowDirsOnly);
    if (testDirectory == "") {
        return;
    }

    createMDFile(testDirectory);
}

void Test::createAllMDFiles() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select the root folder for the MD files", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        createMDFile(topLevelDirectory);
    }

    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir;
        if (!isAValidDirectory(directory)) {
            continue;
        }

        const QString testPathname{ directory + "/" + TEST_FILENAME };
        QFileInfo fileInfo(testPathname);
        if (fileInfo.exists()) {
            createMDFile(directory);
        }
    }

    messageBox.information(0, "Success", "MD files have been created");
}

void Test::createMDFile(QString testDirectory) {
    // Verify folder contains test.js file
    QString testFileName(testDirectory + "/" + TEST_FILENAME);
    QFileInfo testFileInfo(testFileName);
    if (!testFileInfo.exists()) {
        messageBox.critical(0, "Error", "Could not find file: " + TEST_FILENAME);
        return;
    }

    ExtractedText testScriptLines = getTestScriptLines(testFileName);

    QString mdFilename(testDirectory + "/" + "test.md");
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        exit(-1);
    }

    QTextStream stream(&mdFile);

    //Test title
    QString testName = testScriptLines.title;
    stream << "# " << testName << "\n";

    // Find the relevant part of the path to the test (i.e. from "tests" down
    QString partialPath = extractPathFromTestsDown(testDirectory);

    stream << "## Run this script URL: [Manual](./test.js?raw=true)   [Auto](./testAuto.js?raw=true)(from menu/Edit/Open and Run scripts from URL...)."  << "\n\n";

    stream << "## Preconditions" << "\n";
    stream << "- In an empty region of a domain with editing rights." << "\n\n";

    // Platform
    QStringList  platforms = testScriptLines.platform.split(" ");;
    stream << "## Platforms\n";
    stream << "Run the test on each of the following platforms\n";
    for (int i = 0; i < platforms.size(); ++i) {
        // Note that the platforms parameter may include extra spaces, these appear as empty strings in the list
        if (platforms[i] != QString()) {
            stream << " - " << platforms[i] << "\n";
        }
    }

    // Display
    QStringList  displays = testScriptLines.display.split(" ");
    stream << "## Displays\n";
    stream << "Run the test on each of the following displays\n";
    for (int i = 0; i < displays.size(); ++i) {
        // Note that the displays parameter may include extra spaces, these appear as empty strings in the list
        if (displays[i] != QString()) {
            stream << " - " << displays[i] << "\n";
        }
    }

    // CPU
    QStringList  cpus = testScriptLines.cpu.split(" ");
    stream << "## Processors\n";
    stream << "Run the test on each of the following processors\n";
    for (int i = 0; i < cpus.size(); ++i) {
        // Note that the cpus parameter may include extra spaces, these appear as empty strings in the list
        if (cpus[i] != QString()) {
            stream << " - " << cpus[i] << "\n";
        }
    }

    // GPU
    QStringList  gpus = testScriptLines.gpu.split(" ");
    stream << "## Graphics Cards\n";
    stream << "Run the test on graphics cards from each of the following vendors\n";
    for (int i = 0; i < gpus.size(); ++i) {
        // Note that the gpus parameter may include extra spaces, these appear as empty strings in the list
        if (gpus[i] != QString()) {
            stream << " - " << gpus[i] << "\n";
        }
    }

    stream << "## Steps\n";
    stream << "Press space bar to advance step by step\n\n";

    int snapShotIndex { 0 };
    for (size_t i = 0; i < testScriptLines.stepList.size(); ++i) {
        stream << "### Step " << QString::number(i + 1) << "\n";
        stream << "- " << testScriptLines.stepList[i]->text << "\n";
        if (testScriptLines.stepList[i]->takeSnapshot) {
            stream << "- ![](./ExpectedImage_" << QString::number(snapShotIndex).rightJustified(5, '0') << ".png)\n";
            ++snapShotIndex;
        }
    }

    mdFile.close();
}

void Test::createTestsOutline() {
    QString testsRootDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select the tests root folder", ".", QFileDialog::ShowDirsOnly);
    if (testsRootDirectory == "") {
        return;
    }

    const QString testsOutlineFilename { "testsOutline.md" };
    QString mdFilename(testsRootDirectory + "/" + testsOutlineFilename);
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        exit(-1);
    }

    QTextStream stream(&mdFile);

    //Test title
    stream << "# Outline of all tests\n";
    stream << "Directories with an appended (*) have an automatic test\n\n";

    // We need to know our current depth, as this isn't given by QDirIterator
    int rootDepth { testsRootDirectory.count('/') };

    // Each test is shown as the folder name linking to the matching GitHub URL, and the path to the associated test.md file
    QDirIterator it(testsRootDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir;
        if (!isAValidDirectory(directory)) {
            continue;
        }

        // Ignore the utils directory
        if (directory.right(5) == "utils") {
            continue;
        }

        // The prefix is the MarkDown prefix needed for correct indentation
        // It consists of 2 spaces for each level of indentation, folled by a dash sign
        int currentDepth = directory.count('/') - rootDepth;
        QString prefix = QString(" ").repeated(2 * currentDepth - 1) + " - ";

        // The directory name appears after the last slash (we are assured there is at least 1).
        QString directoryName = directory.right(directory.length() - directory.lastIndexOf("/") - 1);

        // autoTester is run on a clone of the repository.  We use relative paths, so we can use both local disk and GitHub
        // For a test in "D:/GitHub/hifi_tests/tests/content/entity/zone/ambientLightInheritance" the
        // GitHub URL  is "./content/entity/zone/ambientLightInheritance?raw=true"
        QString partialPath = directory.right(directory.length() - (directory.lastIndexOf("/tests/") + QString("/tests").length() + 1));
        QString url = "./" + partialPath;

        stream << prefix << "[" << directoryName << "](" << url << "?raw=true" << ")";
        QFileInfo fileInfo1(directory + "/test.md");
        if (fileInfo1.exists()) {
            stream << "  [(test description)](" << url << "/test.md)";
        }

        QFileInfo fileInfo2(directory + "/" + TEST_FILENAME);
        if (fileInfo2.exists()) {
            stream << " (*)";
        }
        stream << "\n";
    }

    mdFile.close();

    messageBox.information(0, "Success", "Test outline file " + testsOutlineFilename + " has been created");
}

void Test::copyJPGtoPNG(QString sourceJPGFullFilename, QString destinationPNGFullFilename) {
    QFile::remove(destinationPNGFullFilename);

    QImageReader reader;
    reader.setFileName(sourceJPGFullFilename);

    QImage image = reader.read();

    QImageWriter writer;
    writer.setFileName(destinationPNGFullFilename);
    writer.write(image);
}

QStringList Test::createListOfAll_imagesInDirectory(QString imageFormat, QString pathToImageDirectory) {
    imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*." + imageFormat;

    return imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

// Snapshots are files in the following format:
//      Filename contains no periods (excluding period before exception
//      Filename (i.e. without extension) contains _tests_ (this is based on all test scripts being within the tests folder
//      Last 5 characters in filename are digits
//      Extension is jpg
bool Test::isInSnapshotFilenameFormat(QString imageFormat, QString filename) {
    QStringList filenameParts = filename.split(".");

    bool filnameHasNoPeriods = (filenameParts.size() == 2);
    bool contains_tests = filenameParts[0].contains("_tests_");

    bool last5CharactersAreDigits;
    filenameParts[0].right(5).toInt(&last5CharactersAreDigits, 10);

    bool extensionIsIMAGE_FORMAT = (filenameParts[1] == imageFormat);

    return (filnameHasNoPeriods && contains_tests && last5CharactersAreDigits && extensionIsIMAGE_FORMAT);
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the test directory is
// D:/GitHub/hifi-tests/tests/content/entity/zone/create
// This method assumes the filename is in the correct format
QString Test::getExpectedImageDestinationDirectory(QString filename) {
    QString filenameWithoutExtension = filename.split(".")[0];
    QStringList filenameParts = filenameWithoutExtension.split("_");

    QString result = filenameParts[0] + ":";

    for (int i = 1; i < filenameParts.length() - 1; ++i) {
        result += "/" + filenameParts[i];
    }

    return result;
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the source directory on GitHub
// is ...tests/content/entity/zone/create
// This is used to create the full URL
// This method assumes the filename is in the correct format
QString Test::getExpectedImagePartialSourceDirectory(QString filename) {
    QString filenameWithoutExtension = filename.split(".")[0];
    QStringList filenameParts = filenameWithoutExtension.split("_");

    // Note that the bottom-most "tests" folder is assumed to be the root
    // This is required because the tests folder is named hifi_tests
    int i { filenameParts.length() - 1 };
    while (i >= 0 && filenameParts[i] != "tests") {
        --i;
    }

    if (i < 0) {
        messageBox.critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Bad filename");
        exit(-1);
    }

    QString result = filenameParts[i];

    for (int j = i + 1; j < filenameParts.length() - 1; ++j) {
        result += "/" + filenameParts[j];
    }

    return result;
}
