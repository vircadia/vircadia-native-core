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
#include <QHostInfo>
#include <QImageReader>
#include <QImageWriter>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

#include <math.h>

Test::Test(QProgressBar* progressBar, QCheckBox* checkBoxInteractiveMode) {
    _progressBar = progressBar;
    _checkBoxInteractiveMode = checkBoxInteractiveMode;

    _mismatchWindow.setModal(true);

    if (autoTester) {
        autoTester->setUserText(GIT_HUB_DEFAULT_USER);
        autoTester->setBranchText(GIT_HUB_DEFAULT_BRANCH);
    }
}

bool Test::createTestResultsFolderPath(const QString& directory) {
    QDateTime now = QDateTime::currentDateTime();
    _testResultsFolderPath =  directory + "/" + TEST_RESULTS_FOLDER + "--" + now.toString(DATETIME_FORMAT) + "(local)[" + QHostInfo::localHostName() + "]";
    QDir testResultsFolder(_testResultsFolderPath);

    // Create a new test results folder
    return QDir().mkdir(_testResultsFolderPath);
}

QString Test::zipAndDeleteTestResultsFolder() {
    QString zippedResultsFileName { _testResultsFolderPath + ".zip" };
    QFileInfo fileInfo(zippedResultsFileName);
    if (fileInfo.exists()) {
        QFile::remove(zippedResultsFileName);
    }

    QDir testResultsFolder(_testResultsFolderPath);
    JlCompress::compressDir(_testResultsFolderPath + ".zip", _testResultsFolderPath);

    testResultsFolder.removeRecursively();

    //In all cases, for the next evaluation
    _testResultsFolderPath = "";
    _failureIndex = 1;
    _successIndex = 1;

    return zippedResultsFileName;
}

int Test::compareImageLists() {
    _progressBar->setMinimum(0);
    _progressBar->setMaximum(_expectedImagesFullFilenames.length() - 1);
    _progressBar->setValue(0);
    _progressBar->setVisible(true);

    // Loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    bool keepOn{ true };
    int numberOfFailures{ 0 };
    for (int i = 0; keepOn && i < _expectedImagesFullFilenames.length(); ++i) {
        // First check that images are the same size
        QImage resultImage(_resultImagesFullFilenames[i]);
        QImage expectedImage(_expectedImagesFullFilenames[i]);

        double similarityIndex;  // in [-1.0 .. 1.0], where 1.0 means images are identical

        bool isInteractiveMode = (!_isRunningFromCommandLine && _checkBoxInteractiveMode->isChecked() && !_isRunningInAutomaticTestRun);
                                 
        // similarityIndex is set to -100.0 to indicate images are not the same size
        if (isInteractiveMode && (resultImage.width() != expectedImage.width() || resultImage.height() != expectedImage.height())) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Images are not the same size");
            similarityIndex = -100.0;
        } else {
            similarityIndex = _imageComparer.compareImages(resultImage, expectedImage);
        }

        TestResult testResult = TestResult{
            (float)similarityIndex,
            _expectedImagesFullFilenames[i].left(_expectedImagesFullFilenames[i].lastIndexOf("/") + 1),  // path to the test (including trailing /)
            QFileInfo(_expectedImagesFullFilenames[i].toStdString().c_str()).fileName(),                 // filename of expected image
            QFileInfo(_resultImagesFullFilenames[i].toStdString().c_str()).fileName()                    // filename of result image
        };

        _mismatchWindow.setTestResult(testResult);

        if (similarityIndex < THRESHOLD) {
            ++numberOfFailures;

            if (!isInteractiveMode) {
                appendTestResultsToFile(_testResultsFolderPath, testResult, _mismatchWindow.getComparisonImage(), true);
            } else {
                _mismatchWindow.exec();

                switch (_mismatchWindow.getUserResponse()) {
                    case USER_RESPONSE_PASS:
                        break;
                    case USE_RESPONSE_FAIL:
                        appendTestResultsToFile(_testResultsFolderPath, testResult, _mismatchWindow.getComparisonImage(), true);
                        break;
                    case USER_RESPONSE_ABORT:
                        keepOn = false;
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        } else {
            appendTestResultsToFile(_testResultsFolderPath, testResult, _mismatchWindow.getComparisonImage(), false);
        }

        _progressBar->setValue(i);
    }

    _progressBar->setVisible(false);
    return numberOfFailures;
}

void Test::appendTestResultsToFile(const QString& _testResultsFolderPath, TestResult testResult, QPixmap comparisonImage, bool hasFailed) {
    if (!QDir().exists(_testResultsFolderPath)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Folder " + _testResultsFolderPath + " not found");
        exit(-1);
    }

    QString resultFolderPath;
    if (hasFailed) {
        resultFolderPath = _testResultsFolderPath + "/Failure_" + QString::number(_failureIndex) + "--" +
                           testResult._actualImageFilename.left(testResult._actualImageFilename.length() - 4);

        ++_failureIndex;
    } else {
        resultFolderPath = _testResultsFolderPath + "/Success_" + QString::number(_successIndex) + "--" +
                           testResult._actualImageFilename.left(testResult._actualImageFilename.length() - 4);

        ++_successIndex;
    }
    
    if (!QDir().mkdir(resultFolderPath)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Failed to create folder " + resultFolderPath);
        exit(-1);
    }

    QFile descriptionFile(resultFolderPath + "/" + TEST_RESULTS_FILENAME);
    if (!descriptionFile.open(QIODevice::ReadWrite)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + TEST_RESULTS_FILENAME);
        exit(-1);
    }

    // Create text file describing the failure
    QTextStream stream(&descriptionFile);
    stream << "Test failed in folder " << testResult._pathname.left(testResult._pathname.length() - 1) << endl; // remove trailing '/'
    stream << "Expected image was    " << testResult._expectedImageFilename << endl;
    stream << "Actual image was      " << testResult._actualImageFilename << endl;
    stream << "Similarity index was  " << testResult._error << endl;

    descriptionFile.close();

    // Copy expected and actual images, and save the difference image
    QString sourceFile;
    QString destinationFile;

    sourceFile = testResult._pathname + testResult._expectedImageFilename;
    destinationFile = resultFolderPath + "/" + "Expected Image.png";
    if (!QFile::copy(sourceFile, destinationFile)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    sourceFile = testResult._pathname + testResult._actualImageFilename;
    destinationFile = resultFolderPath + "/" + "Actual Image.png";
    if (!QFile::copy(sourceFile, destinationFile)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    comparisonImage.save(resultFolderPath + "/" + "Difference Image.png");
}

void Test::startTestsEvaluation(const bool isRunningFromCommandLine,
                                const bool isRunningInAutomaticTestRun, 
                                const QString& snapshotDirectory,
                                const QString& branchFromCommandLine,
                                const QString& userFromCommandLine
) {
    _isRunningFromCommandLine = isRunningFromCommandLine;
    _isRunningInAutomaticTestRun = isRunningInAutomaticTestRun;

    if (snapshotDirectory.isNull()) {
        // Get list of PNG images in folder, sorted by name
        QString previousSelection = _snapshotDirectory;
        QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
        if (!parent.isNull() && parent.right(1) != "/") {
            parent += "/";
        }
        _snapshotDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", parent,
            QFileDialog::ShowDirsOnly);

        // If user canceled then restore previous selection and return
        if (_snapshotDirectory == "") {
            _snapshotDirectory = previousSelection;
            return;
        }
    } else {
        _snapshotDirectory = snapshotDirectory;
        _exitWhenComplete = (isRunningFromCommandLine && !isRunningInAutomaticTestRun);
    }

    // Quit if test results folder could not be created
    if (!createTestResultsFolderPath(_snapshotDirectory)) {
        return;
    }

    // Create two lists.  The first is the test results,  the second is the expected images
    // The expected images are represented as a URL to enable download from GitHub
    // Images that are in the wrong format are ignored.

    QStringList sortedTestResultsFilenames = createListOfAll_imagesInDirectory("png", _snapshotDirectory);
    QStringList expectedImagesURLs;

    _resultImagesFullFilenames.clear();
    _expectedImagesFilenames.clear();
    _expectedImagesFullFilenames.clear();

    QString branch = (branchFromCommandLine.isNull()) ? autoTester->getSelectedBranch() : branchFromCommandLine;
    QString user = (userFromCommandLine.isNull()) ? autoTester->getSelectedUser() : userFromCommandLine;

    foreach(QString currentFilename, sortedTestResultsFilenames) {
        QString fullCurrentFilename = _snapshotDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat("png", currentFilename)) {
            _resultImagesFullFilenames << fullCurrentFilename;

            QString expectedImagePartialSourceDirectory = getExpectedImagePartialSourceDirectory(currentFilename);

            // Images are stored on GitHub as ExpectedImage_ddddd.png
            // Extract the digits at the end of the filename (excluding the file extension)
            QString expectedImageFilenameTail = currentFilename.left(currentFilename.length() - 4).right(NUM_DIGITS);
            QString expectedImageStoredFilename = EXPECTED_IMAGE_PREFIX + expectedImageFilenameTail + ".png";

            QString imageURLString("https://raw.githubusercontent.com/" + user + "/" + GIT_HUB_REPOSITORY  + "/" + branch + "/" +
                expectedImagePartialSourceDirectory + "/" + expectedImageStoredFilename);

            expectedImagesURLs << imageURLString;

            // The image retrieved from GitHub needs a unique name
            QString expectedImageFilename = currentFilename.replace("/", "_").replace(".png", "_EI.png");

            _expectedImagesFilenames << expectedImageFilename;
            _expectedImagesFullFilenames << _snapshotDirectory + "/" + expectedImageFilename;
        }
    }

    autoTester->downloadFiles(expectedImagesURLs, _snapshotDirectory, _expectedImagesFilenames, (void *)this);
}
void Test::finishTestsEvaluation() {
    int numberOfFailures = compareImageLists();
    
    if (!_isRunningFromCommandLine && !_isRunningInAutomaticTestRun) {
        if (numberOfFailures == 0) {
            QMessageBox::information(0, "Success", "All images are as expected");
        } else {
            QMessageBox::information(0, "Failure", "One or more images are not as expected");
        }
    }

    QString zippedFolderName = zipAndDeleteTestResultsFolder();

    if (_exitWhenComplete) {
        exit(0);
    }

    if (_isRunningInAutomaticTestRun) {
        autoTester->automaticTestRunEvaluationComplete(zippedFolderName, numberOfFailures);
    }
}

bool Test::isAValidDirectory(const QString& pathname) {
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

QString Test::extractPathFromTestsDown(const QString& fullPath) {
    // `fullPath` includes the full path to the test.  We need the portion below (and including) `tests`
    QStringList pathParts = fullPath.split('/');
    int i{ 0 };
    while (i < pathParts.length() && pathParts[i] != "tests") {
        ++i;
    }

    if (i == pathParts.length()) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Bad testPathname");
        exit(-1);
    }

    QString partialPath;
    for (int j = i; j < pathParts.length(); ++j) {
        partialPath += "/" + pathParts[j];
    }

    return partialPath;
}

void Test::includeTest(QTextStream& textStream, const QString& testPathname) {
    QString partialPath = extractPathFromTestsDown(testPathname);
    QString partialPathWithoutTests = partialPath.right(partialPath.length() - 7);

    textStream << "Script.include(testsRootPath + \"" << partialPathWithoutTests + "\");" << endl;
}

void Test::createTests() {
    // Rename files sequentially, as ExpectedResult_00000.png, ExpectedResult_00001.png and so on
    // Any existing expected result images will be deleted
    QString previousSelection = _snapshotDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _snapshotDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", parent,
                                                          QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_snapshotDirectory == "") {
        _snapshotDirectory = previousSelection;
        return;
    }

    previousSelection = _testsRootDirectory;
    parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _testsRootDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select test root folder", parent,
                                                              QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_testsRootDirectory == "") {
        _testsRootDirectory = previousSelection;
        return;
    }

    QStringList sortedImageFilenames = createListOfAll_imagesInDirectory("png", _snapshotDirectory);

    int i = 1; 
    const int maxImages = pow(10, NUM_DIGITS);
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = _snapshotDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat("png", currentFilename)) {
            if (i >= maxImages) {
                QMessageBox::critical(0, "Error", "More than " + QString::number(maxImages) + " images not supported");
                exit(-1);
            }

            // Path to test is extracted from the file name
            // Example:
            //      filename is tests.engine.interaction.pointer.laser.distanceScaleEnd.00000.jpg
            //      path is <_testDirectory>/engine/interaction/pointer/laser/distanceScaleEnd
            //
            //      Note: we don't use the first part and the last 2 parts of the filename at this stage
            //
            QStringList pathParts = currentFilename.split(".");
            QString fullNewFileName = _testsRootDirectory;
            for (int j = 1; j < pathParts.size() - 2; ++j) {
                fullNewFileName += "/" + pathParts[j];
            }

            // The image _index is the penultimate component of the path parts (the last being the file extension)
            QString newFilename = "ExpectedImage_" + pathParts[pathParts.size() - 2].rightJustified(5, '0') + ".png";
            fullNewFileName += "/" + newFilename;

            try {
                if (QFile::exists(fullNewFileName)) {
                    QFile::remove(fullNewFileName);
                }               
                QFile::copy(fullCurrentFilename, fullNewFileName);
            } catch (...) {
                QMessageBox::critical(0, "Error", "Could not copy file: " + fullCurrentFilename + " to " + fullNewFileName + "\n");
                exit(-1);
            }
            ++i;
        }
    }

    QMessageBox::information(0, "Success", "Test images have been created");
}

ExtractedText Test::getTestScriptLines(QString testFileName) {
    ExtractedText relevantTextFromTest;

    QFile inputFile(testFileName);
    inputFile.open(QIODevice::ReadOnly);
    if (!inputFile.isOpen()) {
        QMessageBox::critical(0,
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
    QString regexTestTitle(ws + functionPerformName + "\\(" + quotedString);
    QRegularExpression lineContainingTitle = QRegularExpression(regexTestTitle);


    // Each step is either of the following forms:
    //        autoTester.addStepSnapshot("Take snapshot"...
    //        autoTester.addStep("Clean up after test"...
    const QString functionAddStepSnapshotName(ws + "autoTester" + ws + "\\." + ws + "addStepSnapshot");
    const QString regexStepSnapshot(ws + functionAddStepSnapshotName + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineStepSnapshot = QRegularExpression(regexStepSnapshot);

    const QString functionAddStepName(ws + "autoTester" + ws + "\\." + ws + "addStep");
    const QString regexStep(ws + functionAddStepName + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineStep = QRegularExpression(regexStep);

    while (!line.isNull()) {
        line = stream.readLine();
        if (lineContainingTitle.match(line).hasMatch()) {
            QStringList tokens = line.split('"');
            relevantTextFromTest.title = tokens[1];
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

bool Test::createFileSetup() {
    // Folder selection
    QString previousSelection = _testDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _testDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test", parent,
                                                       QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_testDirectory == "") {
        _testDirectory = previousSelection;
        return false;
    }

    return true;
}

bool Test::createAllFilesSetup() {
    // Select folder to start recursing from
    QString previousSelection = _testsRootDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _testsRootDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select the tests root folder", parent,
                                                            QFileDialog::ShowDirsOnly);

    // If user cancelled then restore previous selection and return
    if (_testsRootDirectory == "") {
        _testsRootDirectory = previousSelection;
        return false;
    }

    return true;
}

// Create an MD file for a user-selected test.
// The folder selected must contain a script named "test.js", the file produced is named "test.md"
void Test::createMDFile() {
    if (!createFileSetup()) {
        return;
    } 
    
    if (createMDFile(_testDirectory)) {
        QMessageBox::information(0, "Success", "MD file has been created");
    }
}

void Test::createAllMDFiles() {
    if (!createAllFilesSetup()) {
        return;
    }

    // First test if top-level folder has a test.js file
    const QString testPathname{ _testsRootDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        createMDFile(_testsRootDirectory);
    }

    QDirIterator it(_testsRootDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
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

    QMessageBox::information(0, "Success", "MD files have been created");
}

bool Test::createMDFile(const QString& directory) {
    // Verify folder contains test.js file
    QString testFileName(directory + "/" + TEST_FILENAME);
    QFileInfo testFileInfo(testFileName);
    if (!testFileInfo.exists()) {
        QMessageBox::critical(0, "Error", "Could not find file: " + TEST_FILENAME);
        return false;
    }

    ExtractedText testScriptLines = getTestScriptLines(testFileName);

    QString mdFilename(directory + "/" + "test.md");
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        exit(-1);
    }

    QTextStream stream(&mdFile);

    //Test title
    QString testName = testScriptLines.title;
    stream << "# " << testName << "\n";

    stream << "## Run this script URL: [Manual](./test.js?raw=true)   [Auto](./testAuto.js?raw=true)(from menu/Edit/Open and Run scripts from URL...)."  << "\n\n";

    stream << "## Preconditions" << "\n";
    stream << "- In an empty region of a domain with editing rights." << "\n\n";

    stream << "## Steps\n";
    stream << "Press '" + ADVANCE_KEY + "' key to advance step by step\n\n"; // note apostrophes surrounding 'ADVANCE_KEY'

    int snapShotIndex { 0 };
    for (size_t i = 0; i < testScriptLines.stepList.size(); ++i) {
        stream << "### Step " << QString::number(i + 1) << "\n";
        stream << "- " << testScriptLines.stepList[i]->text << "\n";
        if ((i + 1 < testScriptLines.stepList.size()) && testScriptLines.stepList[i]->takeSnapshot) {
            stream << "- ![](./ExpectedImage_" << QString::number(snapShotIndex).rightJustified(5, '0') << ".png)\n";
            ++snapShotIndex;
        }
    }

    mdFile.close();

    foreach (auto test, testScriptLines.stepList) {
        delete test;
    }
    testScriptLines.stepList.clear();

    return true;
}

void Test::createTestAutoScript() {
    if (!createFileSetup()) {
        return;
    } 
    
    if (createTestAutoScript(_testDirectory)) {
        QMessageBox::information(0, "Success", "'autoTester.js` script has been created");
    }
}

void Test::createAllTestAutoScripts() {
    if (!createAllFilesSetup()) {
        return;
    }

    // First test if top-level folder has a test.js file
    const QString testPathname{ _testsRootDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        createTestAutoScript(_testsRootDirectory);
    }

    QDirIterator it(_testsRootDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
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
            createTestAutoScript(directory);
        }
    }

    QMessageBox::information(0, "Success", "'autoTester.js' scripts have been created");
}

bool Test::createTestAutoScript(const QString& directory) {
    // Verify folder contains test.js file
    QString testFileName(directory + "/" + TEST_FILENAME);
    QFileInfo testFileInfo(testFileName);
    if (!testFileInfo.exists()) {
        QMessageBox::critical(0, "Error", "Could not find file: " + TEST_FILENAME);
        return false;
    }

    QString testAutoScriptFilename(directory + "/" + "testAuto.js");
    QFile testAutoScriptFile(testAutoScriptFilename);
    if (!testAutoScriptFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Failed to create file " + testAutoScriptFilename);
        exit(-1);
    }

    QTextStream stream(&testAutoScriptFile);

    stream << "if (typeof PATH_TO_THE_REPO_PATH_UTILS_FILE === 'undefined') PATH_TO_THE_REPO_PATH_UTILS_FILE = 'https://raw.githubusercontent.com/highfidelity/hifi_tests/master/tests/utils/branchUtils.js';\n";
    stream << "Script.include(PATH_TO_THE_REPO_PATH_UTILS_FILE);\n";
    stream << "var autoTester = createAutoTester(Script.resolvePath('.'));\n\n";
    stream << "autoTester.enableAuto();\n\n";
    stream << "Script.include('./test.js?raw=true');\n";

    testAutoScriptFile.close();
    return true;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    if (!createFileSetup()) {
        return;
    }

    createRecursiveScript(_testDirectory, true);
    QMessageBox::information(0, "Success", "'testRecursive.js` script has been created");
}

// This method creates a `testRecursive.js` script in every sub-folder.
void Test::createAllRecursiveScripts() {
    if (!createAllFilesSetup()) {
        return;
    }

    createRecursiveScript(_testsRootDirectory, false);

    QDirIterator it(_testsRootDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
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

    QMessageBox::information(0, "Success", "Scripts have been created");
}

void Test::createRecursiveScript(const QString& topLevelDirectory, bool interactiveMode) {
    const QString recursiveTestsFilename("testRecursive.js");
    QFile allTestsFilename(topLevelDirectory + "/" + recursiveTestsFilename);
    if (!allTestsFilename.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Failed to create \"" + recursiveTestsFilename + "\" in directory \"" + topLevelDirectory + "\"");

        exit(-1);
    }

    QTextStream textStream(&allTestsFilename);

    textStream << "// This is an automatically generated file, created by auto-tester" << endl;

    // Include 'autoTest.js'
    QString branch = autoTester->getSelectedBranch();
    QString user = autoTester->getSelectedUser();

    textStream << "PATH_TO_THE_REPO_PATH_UTILS_FILE = \"https://raw.githubusercontent.com/" + user + "/hifi_tests/" + branch +
                      "/tests/utils/branchUtils.js\";"
               << endl;
    textStream << "Script.include(PATH_TO_THE_REPO_PATH_UTILS_FILE);" << endl;
    textStream << "var autoTester = createAutoTester(Script.resolvePath(\".\"));" << endl << endl;

    textStream << "var testsRootPath = autoTester.getTestsRootPath();" << endl << endl;

    // Wait 10 seconds before starting
    textStream << "if (typeof Test !== 'undefined') {" << endl;
    textStream << "    Test.wait(10000);" << endl;
    textStream << "};" << endl << endl;

    textStream << "autoTester.enableRecursive();" << endl;
    textStream << "autoTester.enableAuto();" << endl << endl;

    // This is used to verify that the recursive test contains at least one test
    bool testFound{ false };

    // Directories are included in reverse order.  The autoTester scripts use a stack mechanism,
    // so this ensures that the tests run in alphabetical order (a convenience when debugging)
    QStringList directories;

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        // Current folder contains a test
        directories.push_front(testPathname);

        testFound = true;
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
            directories.push_front(testPathname);

            testFound = true;
        }
    }

    if (interactiveMode && !testFound) {
        QMessageBox::information(0, "Failure", "No \"" + TEST_FILENAME + "\" files found");
        allTestsFilename.close();
        return;
    }

    // Now include the test scripts
    for (int i = 0; i < directories.length(); ++i) {
        includeTest(textStream, directories.at(i));
    }

    textStream << endl;
    textStream << "autoTester.runRecursive();" << endl;

    allTestsFilename.close();
}

void Test::createTestsOutline() {
    QString previousSelection = _testDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _testDirectory =
        QFileDialog::getExistingDirectory(nullptr, "Please select the tests root folder", parent, QFileDialog::ShowDirsOnly);

    // If user canceled then restore previous selection and return
    if (_testDirectory == "") {
        _testDirectory = previousSelection;
        return;
    }

    const QString testsOutlineFilename { "testsOutline.md" };
    QString mdFilename(_testDirectory + "/" + testsOutlineFilename);
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        exit(-1);
    }

    QTextStream stream(&mdFile);

    //Test title
    stream << "# Outline of all tests\n";
    stream << "Directories with an appended (*) have an automatic test\n\n";

    // We need to know our current depth, as this isn't given by QDirIterator
    int rootDepth { _testDirectory.count('/') };

    // Each test is shown as the folder name linking to the matching GitHub URL, and the path to the associated test.md file
    QDirIterator it(_testDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
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

        // note that md files may be named 'test.md' or 'testStory.md'
        QFileInfo fileInfo1(directory + "/test.md");
        if (fileInfo1.exists()) {
            stream << "  [(test description)](" << url << "/test.md)";
        }

        QFileInfo fileInfo2(directory + "/testStory.md");
        if (fileInfo2.exists()) {
            stream << "  [(test description)](" << url << "/testStory.md)";
        }

        QFileInfo fileInfo3(directory + "/" + TEST_FILENAME);
        if (fileInfo3.exists()) {
            stream << " (*)";
        }
        stream << "\n";
    }

    mdFile.close();

    QMessageBox::information(0, "Success", "Test outline file " + testsOutlineFilename + " has been created");
}

void Test::createTestRailTestCases() {
    QString previousSelection = _testDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _testDirectory =
        QFileDialog::getExistingDirectory(nullptr, "Please select the tests root folder", parent, QFileDialog::ShowDirsOnly);

    // If user cancelled then restore previous selection and return
    if (_testDirectory.isNull()) {
        _testDirectory = previousSelection;
        return;
    }

    QString outputDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store generated files in",
                                                                nullptr, QFileDialog::ShowDirsOnly);

    // If user cancelled then return
    if (outputDirectory.isNull()) {
        return;
    }

    if (_testRailCreateMode == PYTHON) {
        _testRailInterface.createTestSuitePython(_testDirectory, outputDirectory, autoTester->getSelectedUser(),
                                              autoTester->getSelectedBranch());
    } else {
        _testRailInterface.createTestSuiteXML(_testDirectory, outputDirectory, autoTester->getSelectedUser(),
                                           autoTester->getSelectedBranch());
    }
}

void Test::createTestRailRun() {
    QString outputDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store generated files in",
                                                                nullptr, QFileDialog::ShowDirsOnly);

    if (outputDirectory.isNull()) {
        return;
    }

    _testRailInterface.createTestRailRun(outputDirectory);
}

void Test::updateTestRailRunResult() {
    QString testResults = QFileDialog::getOpenFileName(nullptr, "Please select the zipped test results to update from", nullptr,
                                                       "Zipped Test Results (*.zip)");   
    if (testResults.isNull()) {
        return;
    }

    QString tempDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store temporary files in",
                                                                nullptr, QFileDialog::ShowDirsOnly);
    if (tempDirectory.isNull()) {
        return;
    }

    _testRailInterface.updateTestRailRunResults(testResults, tempDirectory);
}

QStringList Test::createListOfAll_imagesInDirectory(const QString& imageFormat, const QString& pathToImageDirectory) {
    _imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*." + imageFormat;

    return _imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

// Snapshots are files in the following format:
//      Filename (i.e. without extension) contains tests (this is based on all test scripts being within the tests folder)
//      Last 5 characters in filename are digits (after removing the extension)
//      Extension is 'imageFormat'
bool Test::isInSnapshotFilenameFormat(const QString& imageFormat, const QString& filename) {
    bool contains_tests = filename.contains("tests" + PATH_SEPARATOR);

    QString filenameWithoutExtension = filename.left(filename.lastIndexOf('.'));
    bool last5CharactersAreDigits;
    filenameWithoutExtension.right(5).toInt(&last5CharactersAreDigits, 10);

    bool extensionIsIMAGE_FORMAT = (filename.right(imageFormat.length()) == imageFormat);

    return (contains_tests && last5CharactersAreDigits && extensionIsIMAGE_FORMAT);
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the test directory is
// D:/GitHub/hifi-tests/tests/content/entity/zone/create
// This method assumes the filename is in the correct format
QString Test::getExpectedImageDestinationDirectory(const QString& filename) {
    QString filenameWithoutExtension = filename.left(filename.length() - 4);
    QStringList filenameParts = filenameWithoutExtension.split(PATH_SEPARATOR);

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
QString Test::getExpectedImagePartialSourceDirectory(const QString& filename) {
    QString filenameWithoutExtension = filename.left(filename.length() - 4);
    QStringList filenameParts = filenameWithoutExtension.split(PATH_SEPARATOR);

    // Note that the bottom-most "tests" folder is assumed to be the root
    // This is required because the tests folder is named hifi_tests
    int i { filenameParts.length() - 1 };
    while (i >= 0 && filenameParts[i] != "tests") {
        --i;
    }

    if (i < 0) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Bad filename");
        exit(-1);
    }

    QString result = filenameParts[i];

    for (int j = i + 1; j < filenameParts.length() - 1; ++j) {
        result += "/" + filenameParts[j];
    }

    return result;
}

void Test::setTestRailCreateMode(TestRailCreateMode testRailCreateMode) {
    _testRailCreateMode = testRailCreateMode;
}

void Test::createWebPage(QCheckBox* updateAWSCheckBox, QLineEdit* urlLineEdit) {
    QString testResults = QFileDialog::getOpenFileName(nullptr, "Please select the zipped test results to update from", nullptr,
                                                       "Zipped Test Results (*.zip)");
    if (testResults.isNull()) {
        return;
    }

    QString tempDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store temporary files in",
                                                              nullptr, QFileDialog::ShowDirsOnly);
    if (tempDirectory.isNull()) {
        return;
    }

    _awsInterface.createWebPageFromResults(testResults, tempDirectory, updateAWSCheckBox, urlLineEdit);
}