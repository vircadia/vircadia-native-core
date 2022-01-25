//
//  TestCreator.cpp
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TestCreator.h"

#include <assert.h>
#include <QtCore/QTextStream>
#include <QDirIterator>
#include <QHostInfo>
#include <QImageReader>
#include <QImageWriter>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "Nitpick.h"
extern Nitpick* nitpick;

#include <math.h>

TestCreator::TestCreator(QProgressBar* progressBar, QCheckBox* checkBoxInteractiveMode) : _awsInterface(NULL) {
    _downloader = new Downloader();

    _progressBar = progressBar;
    _checkBoxInteractiveMode = checkBoxInteractiveMode;

    _mismatchWindow.setModal(true);

    if (nitpick) {
        nitpick->setUserText(GIT_HUB_DEFAULT_USER);
        nitpick->setBranchText(GIT_HUB_DEFAULT_BRANCH);
    }
}

bool TestCreator::createTestResultsFolderPath(const QString& directory) {
    QDateTime now = QDateTime::currentDateTime();
    _testResultsFolderPath =  directory + "/" + TEST_RESULTS_FOLDER + "--" + now.toString(DATETIME_FORMAT) + "(local)[" + QHostInfo::localHostName() + "]";
    QDir testResultsFolder(_testResultsFolderPath);

    // Create a new test results folder
    return QDir().mkdir(_testResultsFolderPath);
}

QString TestCreator::zipAndDeleteTestResultsFolder() {
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

int TestCreator::compareImageLists(const QString& gpuVendor) {
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
        double worstTileValue;   // in [-1.0 .. 1.0], where 1.0 means images are identical

        bool isInteractiveMode = (!_isRunningFromCommandLine && _checkBoxInteractiveMode->isChecked() && !_isRunningInAutomaticTestRun);
                                 
        // similarityIndex is set to -100.0 to indicate images are not the same size
        if (isInteractiveMode && (resultImage.width() != expectedImage.width() || resultImage.height() != expectedImage.height())) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Images are not the same size");
            similarityIndex = -100.0;
            worstTileValue = 0.0;
        } else {
            _imageComparer.compareImages(resultImage, expectedImage);
            similarityIndex = _imageComparer.getSSIMValue();
            worstTileValue = _imageComparer.getWorstTileValue();
        }

        TestResult testResult = TestResult{
            similarityIndex,
            worstTileValue,
            _expectedImagesFullFilenames[i].left(_expectedImagesFullFilenames[i].lastIndexOf("/") + 1),  // path to the test (including trailing /)
            QFileInfo(_expectedImagesFullFilenames[i].toStdString().c_str()).fileName(),                 // filename of expected image
            QFileInfo(_resultImagesFullFilenames[i].toStdString().c_str()).fileName(),                   // filename of result image
            _imageComparer.getSSIMResults()                                                              // results of SSIM algoritm
        };

        _mismatchWindow.setTestResult(testResult);

        // Lower threshold for non-Nvidia GPUs
        double thresholdGlobal{ 0.9995 };
        double thresholdLocal{ 0.6 };
        if (gpuVendor != "Nvidia") {
            thresholdGlobal =  0.97;
            thresholdLocal = 0.2;
        }

        if (similarityIndex < thresholdGlobal || worstTileValue < thresholdLocal) {
            if (!isInteractiveMode) {
                ++numberOfFailures;
                appendTestResultsToFile(testResult, _mismatchWindow.getComparisonImage(), _mismatchWindow.getSSIMResultsImage(testResult._ssimResults), true);
            } else {
                _mismatchWindow.exec();

                switch (_mismatchWindow.getUserResponse()) {
                    case USER_RESPONSE_PASS:
                        break;
                    case USE_RESPONSE_FAIL:
                        ++numberOfFailures;
                        appendTestResultsToFile(testResult, _mismatchWindow.getComparisonImage(), _mismatchWindow.getSSIMResultsImage(testResult._ssimResults), true);
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
            appendTestResultsToFile(testResult, _mismatchWindow.getComparisonImage(), _mismatchWindow.getSSIMResultsImage(testResult._ssimResults), false);
        }

        _progressBar->setValue(i);
    }

    _progressBar->setVisible(false);
    return numberOfFailures;
}

int TestCreator::checkTextResults() {
    // Create lists of failed and passed tests
    QStringList nameFilterFailed;
    nameFilterFailed << "*.failed.txt";
    QStringList testsFailed = QDir(_snapshotDirectory).entryList(nameFilterFailed, QDir::Files, QDir::Name);

    QStringList nameFilterPassed;
    nameFilterPassed << "*.passed.txt";
    QStringList testsPassed = QDir(_snapshotDirectory).entryList(nameFilterPassed, QDir::Files, QDir::Name);

    // Add results to TestCreator Results folder
    foreach(QString currentFilename, testsFailed) {
        appendTestResultsToFile(currentFilename, true);
    }

    foreach(QString currentFilename, testsPassed) {
        appendTestResultsToFile(currentFilename, false);
    }

    return testsFailed.length();
}

void TestCreator::appendTestResultsToFile(const TestResult& testResult, const QPixmap& comparisonImage, const QPixmap& ssimResultsImage, bool hasFailed) {
    // Critical error if Test Results folder does not exist
    if (!QDir().exists(_testResultsFolderPath)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Folder " + _testResultsFolderPath + " not found");
        exit(-1);
    }

    // There are separate subfolders for failures and passes
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
    stream << "TestCreator in folder " << testResult._pathname.left(testResult._pathname.length() - 1) << Qt::endl; // remove trailing '/'
    stream << "Expected image was    " << testResult._expectedImageFilename << Qt::endl;
    stream << "Actual image was      " << testResult._actualImageFilename << Qt::endl;
    stream << "Similarity index was  " << testResult._errorGlobal << Qt::endl;
    stream << "Worst tile was  " << testResult._errorLocal << Qt::endl;

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

    // Save the SSIM results image
    ssimResultsImage.save(resultFolderPath + "/" + "SSIM Image.png");
}

void::TestCreator::appendTestResultsToFile(QString testResultFilename, bool hasFailed) {
    // The test name includes everything until the penultimate period
    QString testNameTemp = testResultFilename.left(testResultFilename.lastIndexOf('.'));
    QString testName = testResultFilename.left(testNameTemp.lastIndexOf('.'));
    QString resultFolderPath;
    if (hasFailed) {
        resultFolderPath = _testResultsFolderPath + "/Failure_" + QString::number(_failureIndex) + "--" + testName;
        ++_failureIndex;
    } else {
        resultFolderPath = _testResultsFolderPath + "/Success_" + QString::number(_successIndex) + "--" + testName;
        ++_successIndex;
    }

    if (!QDir().mkdir(resultFolderPath)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Failed to create folder " + resultFolderPath);
        exit(-1);
    }

    QString source = _snapshotDirectory + "/" + testResultFilename;
    QString destination = resultFolderPath + "/Result.txt";
    if (!QFile::copy(source, destination)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to copy " + testResultFilename + " to " + resultFolderPath);
        exit(-1);
    }
}

void TestCreator::startTestsEvaluation(
    QComboBox *gpuVendor, 
    const bool isRunningFromCommandLine,
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
        _snapshotDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the snapshots", parent,
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

    QString branch = (branchFromCommandLine.isNull()) ? nitpick->getSelectedBranch() : branchFromCommandLine;
    QString user = (userFromCommandLine.isNull()) ? nitpick->getSelectedUser() : userFromCommandLine;

    foreach(QString currentFilename, sortedTestResultsFilenames) {
        QString fullCurrentFilename = _snapshotDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat("png", currentFilename)) {
            _resultImagesFullFilenames << fullCurrentFilename;

            QString expectedImagePartialSourceDirectory = getExpectedImagePartialSourceDirectory(currentFilename);

            // Images are stored on GitHub as ExpectedImage_ddddd.png or ExpectedImage_some_metadata_ddddd.png
            // Extract the part of the filename after "ExpectedImage_" and excluding the file extension
            QString expectedImageFilenameTail = currentFilename.left(currentFilename.lastIndexOf("."));
            int expectedImageStart = expectedImageFilenameTail.lastIndexOf(".") + 1;
            expectedImageFilenameTail.remove(0, expectedImageStart);
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

    _downloader->downloadFiles(expectedImagesURLs, _snapshotDirectory, _expectedImagesFilenames, (void *)this);
    finishTestsEvaluation(gpuVendor->currentText());
}

void TestCreator::finishTestsEvaluation(const QString& gpuVendor) {
    // First - compare the pairs of images
    int numberOfFailures = compareImageLists(gpuVendor);
 
    // Next - check text results
    numberOfFailures += checkTextResults();

    if (!_isRunningFromCommandLine && !_isRunningInAutomaticTestRun) {
        if (numberOfFailures == 0) {
            QMessageBox::information(0, "Success", "All images are as expected");
        } else if (numberOfFailures == 1) {
            QMessageBox::information(0, "Failure", "One image is not as expected");
        } else {
            QMessageBox::information(0, "Failure", QString::number(numberOfFailures) + " images are not as expected");
        }
    }

    QString zippedFolderName = zipAndDeleteTestResultsFolder();

    if (_exitWhenComplete) {
        exit(0);
    }

    if (_isRunningInAutomaticTestRun) {
        nitpick->automaticTestRunEvaluationComplete(zippedFolderName, numberOfFailures);
    }
}

bool TestCreator::isAValidDirectory(const QString& pathname) {
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

QString TestCreator::extractPathFromTestsDown(const QString& fullPath) {
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

void TestCreator::includeTest(QTextStream& textStream, const QString& testPathname) {
    QString partialPath = extractPathFromTestsDown(testPathname);
    QString partialPathWithoutTests = partialPath.right(partialPath.length() - 7);

    textStream << "Script.include(testsRootPath + \"" << partialPathWithoutTests + "\");" << Qt::endl;
}

void TestCreator::createTests(const QString& clientProfile) {
    // Rename files sequentially, as ExpectedResult_00000.png, ExpectedResult_00001.png and so on
    // Any existing expected result images will be deleted
    QString previousSelection = _snapshotDirectory;
    QString parent = previousSelection.left(previousSelection.lastIndexOf('/'));
    if (!parent.isNull() && parent.right(1) != "/") {
        parent += "/";
    }

    _snapshotDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the snapshots", parent,
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

    if (!createAllFilesSetup()) {
        return;
    }

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

namespace TestProfile {
    std::vector<QString> tiers = [](){
        std::vector<QString> toReturn;
        for (int tier = (int)platform::Profiler::Tier::LOW; tier < (int)platform::Profiler::Tier::NumTiers; ++tier) {
            QString tierStringUpper = platform::Profiler::TierNames[tier];
            toReturn.push_back(tierStringUpper.toLower());
        }
        return toReturn;
    }();

    std::vector<QString> operatingSystems = { "windows", "mac", "linux", "android" };

    std::vector<QString> gpus = { "amd", "nvidia", "intel" };
};

enum class ProfileCategory {
    TIER,
    OS,
    GPU
};
const std::map<QString, ProfileCategory> propertyToProfileCategory = [](){
    std::map<QString, ProfileCategory> toReturn;
    for (const auto& tier : TestProfile::tiers) {
        toReturn[tier] = ProfileCategory::TIER;
    }
    for (const auto& os : TestProfile::operatingSystems) {
        toReturn[os] = ProfileCategory::OS;
    }
    for (const auto& gpu : TestProfile::gpus) {
        toReturn[gpu] = ProfileCategory::GPU;
    }
    return toReturn;
}();

TestFilter::TestFilter(const QString& filterString) {
    auto filterParts = filterString.split(".", QString::SkipEmptyParts);
    for (const auto& filterPart : filterParts) {
        QList<QString> allowedVariants = filterPart.split(",", QString::SkipEmptyParts);
        if (allowedVariants.empty()) {
            continue;
        }

        auto& referenceVariant = allowedVariants[0];
        auto foundCategoryIt = propertyToProfileCategory.find(referenceVariant);
        if (foundCategoryIt == propertyToProfileCategory.cend()) {
            error = "Invalid test filter property '" + referenceVariant + "'";
            return;
        }

        ProfileCategory selectedFilterCategory = foundCategoryIt->second;
        for (auto allowedVariantIt = ++(allowedVariants.cbegin()); allowedVariantIt != allowedVariants.cend(); ++allowedVariantIt) {
            auto& currentVariant = *allowedVariantIt;
            auto nextCategoryIt = propertyToProfileCategory.find(currentVariant);
            if (nextCategoryIt == propertyToProfileCategory.cend()) {
                error = "Invalid test filter property '" + referenceVariant + "'";
                return;
            }
            auto& currentCategory = nextCategoryIt->second;
            if (currentCategory != selectedFilterCategory) {
                error = "Mismatched comma-separated test filter properties '" + referenceVariant + "' and '" + currentVariant + "'";
                return;
            }
            // List of comma-separated test property variants is consistent so far
        }

        switch (selectedFilterCategory) {
        case ProfileCategory::TIER:
            allowedTiers.insert(allowedTiers.cend(), allowedVariants.cbegin(), allowedVariants.cend());
            break;
        case ProfileCategory::OS:
            allowedOperatingSystems.insert(allowedOperatingSystems.cend(), allowedVariants.cbegin(), allowedVariants.cend());
            break;
        case ProfileCategory::GPU:
            allowedGPUs.insert(allowedGPUs.cend(), allowedVariants.cbegin(), allowedVariants.cend());
            break;
        }
    }
}

bool TestFilter::isValid() const {
    return error.isEmpty();
}

QString TestFilter::getError() const {
    return error;
}

ExtractedText TestCreator::getTestScriptLines(QString testFileName) {
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
    //        nitpick.perform("Apply Material Entities to Avatars", Script.resolvePath("."), "secondary", undefined, function(testType) {...
    const QString ws("\\h*");    //white-space character
    const QString functionPerformName(ws + "nitpick" + ws + "\\." + ws + "perform");
    const QString quotedString("\\\".+\\\"");
    QString regexTestTitle(ws + functionPerformName + "\\(" + quotedString);
    QRegularExpression lineContainingTitle = QRegularExpression(regexTestTitle);


    // Each step is either of the following forms:
    //        nitpick.addStepSnapshot("Take snapshot"...
    //        nitpick.addStep("Clean up after test"...
    const QString functionAddStepSnapshotName(ws + "nitpick" + ws + "\\." + ws + "addStepSnapshot");
    const QString regexStepSnapshot(ws + functionAddStepSnapshotName + ws + "\\(" + ws + quotedString + ".*");
    const QRegularExpression lineStepSnapshot = QRegularExpression(regexStepSnapshot);

    const QString functionAddStepName(ws + "nitpick" + ws + "\\." + ws + "addStep");
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

bool TestCreator::createFileSetup() {
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

bool TestCreator::createAllFilesSetup() {
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
void TestCreator::createMDFile() {
    if (!createFileSetup()) {
        return;
    } 
    
    if (createMDFile(_testDirectory)) {
        QMessageBox::information(0, "Success", "MD file has been created");
    }
}

void TestCreator::createAllMDFiles() {
    if (!createAllFilesSetup()) {
        return;
    }

    // First test if top-level folder has a test.js file
    const QString testPathname{ _testsRootDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        createMDFile(_testsRootDirectory);
    }

    QDirIterator it(_testsRootDirectory, QDirIterator::Subdirectories);
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

QString joinVector(const std::vector<QString>& qStringVector, const char* separator) {
    if (qStringVector.empty()) {
        return QString("");
    }
    QString joined = qStringVector[0];
    for (std::size_t i = 1; i < qStringVector.size(); ++i) {
        joined += separator + qStringVector[i];
    }
    return joined;
}

bool TestCreator::createMDFile(const QString& directory) {
    // Verify folder contains test.js file
    QString testFileName(directory + "/" + TEST_FILENAME);
    QFileInfo testFileInfo(testFileName);
    if (!testFileInfo.exists()) {
        QMessageBox::critical(0, "Error", "Could not find file: " + TEST_FILENAME);
        return false;
    }

    ExtractedText testScriptLines = getTestScriptLines(testFileName);

    QDir qDirectory(directory);

    QString mdFilename(directory + "/" + "test.md");
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        // TODO: Don't just exit
        exit(-1);
    }

    QTextStream stream(&mdFile);

    QString testName = testScriptLines.title;
    stream << "# " << testName << "\n";

    stream << "## Run this script URL: [Manual](./test.js?raw=true)   [Auto](./testAuto.js?raw=true)(from menu/Edit/Open and Run scripts from URL...)."  << "\n\n";

    stream << "## Preconditions" << "\n";
    stream << "- In an empty region of a domain with editing rights." << "\n";
    stream << "\n";

    // ExpectedImage_00000.png OR ExpectedImage_some_stu-ff_00000.png
    const QRegularExpression firstExpectedImage("^ExpectedImage(_[-_\\w]*)?_00000\\.png$");
    std::vector<QString> testDescriptors;
    std::vector<TestFilter> testFilters;

    for (const auto& potentialImageFile : qDirectory.entryInfoList()) {
        if (potentialImageFile.isDir()) {
            continue;
        }

        auto firstExpectedImageMatch = firstExpectedImage.match(potentialImageFile.fileName());
        if (!firstExpectedImageMatch.hasMatch()) {
            continue;
        }

        QString testDescriptor = firstExpectedImageMatch.captured(1);
        auto filterString = QString(testDescriptor).replace("_", ".").replace("-", ",");
        TestFilter descriptorAsFilter(filterString);

        testDescriptors.push_back(testDescriptor);
        testFilters.push_back(descriptorAsFilter);
    }

    stream << "## Steps\n";
    stream << "Press '" + ADVANCE_KEY + "' key to advance step by step\n\n"; // note apostrophes surrounding 'ADVANCE_KEY'
    int snapShotIndex { 0 };
    for (size_t i = 0; i < testScriptLines.stepList.size(); ++i) {
        stream << "### Step " << QString::number(i + 1) << "\n";
        stream << "- " << testScriptLines.stepList[i]->text << "\n";
        if ((i + 1 < testScriptLines.stepList.size()) && testScriptLines.stepList[i]->takeSnapshot) {
            for (int i = 0; i < (int)testDescriptors.size(); ++i) {
                const auto& testDescriptor = testDescriptors[i];
                const auto& descriptorAsFilter = testFilters[i];
                if (descriptorAsFilter.isValid()) {
                    stream << "- Expected image on ";
                    stream << (descriptorAsFilter.allowedTiers.empty() ? "any" : joinVector(descriptorAsFilter.allowedTiers, "/")) << " tier, ";
                    stream << (descriptorAsFilter.allowedOperatingSystems.empty() ? "any" : joinVector(descriptorAsFilter.allowedOperatingSystems, "/")) << " OS, ";
                    stream << (descriptorAsFilter.allowedGPUs.empty() ? "any" : joinVector(descriptorAsFilter.allowedGPUs, "/")) << " GPU";
                    stream << ":";
                } else {
                    // Fall back to displaying file name
                    stream << "- ExpectedImage" << testDescriptor << "_" << QString::number(snapShotIndex).rightJustified(5, '0') << ".png";
                }
                stream << "\n";
                stream << "- ![](./ExpectedImage" << testDescriptor << "_" << QString::number(snapShotIndex).rightJustified(5, '0') << ".png)\n";
            }
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

void TestCreator::createTestAutoScript() {
    if (!createFileSetup()) {
        return;
    } 
    
    if (createTestAutoScript(_testDirectory)) {
        QMessageBox::information(0, "Success", "'testAuto.js` script has been created");
    }
}

void TestCreator::createAllTestAutoScripts() {
    if (!createAllFilesSetup()) {
        return;
    }

    // First test if top-level folder has a test.js file
    const QString testPathname{ _testsRootDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        createTestAutoScript(_testsRootDirectory);
    }

    QDirIterator it(_testsRootDirectory, QDirIterator::Subdirectories);
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

    QMessageBox::information(0, "Success", "All 'testAuto.js' scripts have been created");
}

bool TestCreator::createTestAutoScript(const QString& directory) {
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
    stream << "var nitpick = createNitpick(Script.resolvePath('.'));\n\n";
    stream << "nitpick.enableAuto();\n\n";
    stream << "Script.include('./test.js?raw=true');\n";

    testAutoScriptFile.close();
    return true;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void TestCreator::createRecursiveScript() {
    if (!createFileSetup()) {
        return;
    }

    createRecursiveScript(_testDirectory, true);
    QMessageBox::information(0, "Success", "'testRecursive.js` script has been created");
}

// This method creates a `testRecursive.js` script in every sub-folder.
void TestCreator::createAllRecursiveScripts() {
    if (!createAllFilesSetup()) {
        return;
    }

    createAllRecursiveScripts(_testsRootDirectory);
    createRecursiveScript(_testsRootDirectory, false);
    QMessageBox::information(0, "Success", "Scripts have been created");
}

void TestCreator::createAllRecursiveScripts(const QString& directory) {
    QDirIterator it(directory, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString nextDirectory = it.next();
        if (isAValidDirectory(nextDirectory)) {
            createAllRecursiveScripts(nextDirectory);
            createRecursiveScript(nextDirectory, false);
        }
    }
}

void TestCreator::createRecursiveScript(const QString& directory, bool interactiveMode) {
    // If folder contains a test, then we are at a leaf
    const QString testPathname{ directory + "/" + TEST_FILENAME };
    if (QFileInfo(testPathname).exists()) {
        return;
    }

    // Directories are included in reverse order.  The nitpick scripts use a stack mechanism,
    // so this ensures that the tests run in alphabetical order (a convenience when debugging)
    QStringList directories;
    QDirIterator it(directory);
    while (it.hasNext()) {
        QString subDirectory = it.next();

        // Only process directories
        if (!isAValidDirectory(subDirectory)) {
            continue;
        }

        const QString testPathname{ subDirectory + "/" + TEST_FILENAME };
        if (QFileInfo(testPathname).exists()) {
            // Current folder contains a test script
            directories.push_front(testPathname);
        }

        const QString testRecursivePathname{ subDirectory + "/" + TEST_RECURSIVE_FILENAME };
        if (QFileInfo(testRecursivePathname).exists()) {
            // Current folder contains a recursive script
            directories.push_front(testRecursivePathname);
        }
    }

    // If 'directories' is empty, this means that this recursive script has no tests to call, so it is redundant
    if (directories.length() == 0) {
        QString testRecursivePathname = directory + "/" + TEST_RECURSIVE_FILENAME;
        if (QFile::exists(testRecursivePathname)) {
            QFile::remove(testRecursivePathname);
        }
        return;
    }

    // Open the recursive script file
    const QString recursiveTestsFilename(directory + "/" + TEST_RECURSIVE_FILENAME);
    QFile recursiveTestsFile(recursiveTestsFilename);
    if (!recursiveTestsFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Failed to create \"" + TEST_RECURSIVE_FILENAME + "\" in directory \"" + directory + "\"");

        exit(-1);
    }

    QTextStream textStream(&recursiveTestsFile);

    textStream << "// This is an automatically generated file, created by nitpick" << Qt::endl;

    // Include 'nitpick.js'
    QString branch = nitpick->getSelectedBranch();
    QString user = nitpick->getSelectedUser();

    textStream << "PATH_TO_THE_REPO_PATH_UTILS_FILE = \"https://raw.githubusercontent.com/" + user + "/hifi_tests/" + branch +
        "/tests/utils/branchUtils.js\";"
        << Qt::endl;
    textStream << "Script.include(PATH_TO_THE_REPO_PATH_UTILS_FILE);" << Qt::endl << Qt::endl;

    // The 'depth' variable is used to signal when to start running the recursive scripts
    textStream << "if (typeof depth === 'undefined') {" << Qt::endl;
    textStream << "   depth = 0;" << Qt::endl;
    textStream << "   nitpick = createNitpick(Script.resolvePath(\".\"));" << Qt::endl;
    textStream << "   testsRootPath = nitpick.getTestsRootPath();" << Qt::endl << Qt::endl;
    textStream << "   nitpick.enableRecursive();" << Qt::endl;
    textStream << "   nitpick.enableAuto();" << Qt::endl;
    textStream << "} else {" << Qt::endl;
    textStream << "   depth++" << Qt::endl;
    textStream << "}" << Qt::endl << Qt::endl;

    // Now include the test scripts
    for (int i = 0; i < directories.length(); ++i) {
        includeTest(textStream, directories.at(i));
    }

    textStream << Qt::endl;
    textStream << "if (depth > 0) {" << Qt::endl;
    textStream << "   depth--;" << Qt::endl;
    textStream << "} else {" << Qt::endl;
    textStream << "   nitpick.runRecursive();" << Qt::endl;
    textStream << "}" << Qt::endl << Qt::endl;

    recursiveTestsFile.close();
}

void TestCreator::createTestsOutline() {
    if (!createAllFilesSetup()) {
        return;
    }

    const QString testsOutlineFilename { "testsOutline.md" };
    QString mdFilename(_testsRootDirectory + "/" + testsOutlineFilename);
    QFile mdFile(mdFilename);
    if (!mdFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Failed to create file " + mdFilename);
        exit(-1);
    }

    QTextStream stream(&mdFile);

    //TestCreator title
    stream << "# Outline of all tests\n";
    stream << "Directories with an appended (*) have an automatic test\n\n";

    // We need to know our current depth, as this isn't given by QDirIterator
    int rootDepth { _testsRootDirectory.count('/') };

    // Each test is shown as the folder name linking to the matching GitHub URL, and the path to the associated test.md file
    QDirIterator it(_testsRootDirectory, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
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

        // nitpick is run on a clone of the repository.  We use relative paths, so we can use both local disk and GitHub
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

    QMessageBox::information(0, "Success", "TestCreator outline file " + testsOutlineFilename + " has been created");
}

void TestCreator::createTestRailTestCases() {
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

    if (!_testRailInterface) {
        _testRailInterface = new TestRailInterface;
    }

    if (_testRailCreateMode == PYTHON) {
        _testRailInterface->createTestSuitePython(_testDirectory, outputDirectory, nitpick->getSelectedUser(),
                                              nitpick->getSelectedBranch());
    } else {
        _testRailInterface->createTestSuiteXML(_testDirectory, outputDirectory, nitpick->getSelectedUser(),
                                           nitpick->getSelectedBranch());
    }
}

void TestCreator::createTestRailRun() {
    QString outputDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store generated files in",
                                                                nullptr, QFileDialog::ShowDirsOnly);

    if (outputDirectory.isNull()) {
        return;
    }


    if (!_testRailInterface) {
        _testRailInterface = new TestRailInterface;
    }

    _testRailInterface->createTestRailRun(outputDirectory);
}

void TestCreator::updateTestRailRunResult() {
    QString testResults = QFileDialog::getOpenFileName(nullptr, "Please select the zipped test results to update from", nullptr,
                                                       "Zipped TestCreator Results (*.zip)");   
    if (testResults.isNull()) {
        return;
    }

    QString tempDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store temporary files in",
                                                                nullptr, QFileDialog::ShowDirsOnly);
    if (tempDirectory.isNull()) {
        return;
    }


    if (!_testRailInterface) {
        _testRailInterface = new TestRailInterface;
    }

    _testRailInterface->updateTestRailRunResults(testResults, tempDirectory);
}

QStringList TestCreator::createListOfAll_imagesInDirectory(const QString& imageFormat, const QString& pathToImageDirectory) {
    _imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*." + imageFormat;

    return _imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

// Snapshots are files in the following format:
//      Filename (i.e. without extension) contains tests (this is based on all test scripts being within the tests folder)
//      Last 5 characters in filename are digits (after removing the extension)
//      Extension is 'imageFormat'
bool TestCreator::isInSnapshotFilenameFormat(const QString& imageFormat, const QString& filename) {
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
QString TestCreator::getExpectedImageDestinationDirectory(const QString& filename) {
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
QString TestCreator::getExpectedImagePartialSourceDirectory(const QString& filename) {
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

void TestCreator::setTestRailCreateMode(TestRailCreateMode testRailCreateMode) {
    _testRailCreateMode = testRailCreateMode;
}

void TestCreator::createWebPage(
    QCheckBox* updateAWSCheckBox, 
    QRadioButton* diffImageRadioButton,
    QRadioButton* ssimImageRadionButton,
    QLineEdit* urlLineEdit,
    const QString& branch,
    const QString& user

) {
    QString testResults = QFileDialog::getOpenFileName(nullptr, "Please select the zipped test results to update from", nullptr,
                                                       "Zipped TestCreator Results (TestResults--*.zip)");
    if (testResults.isNull()) {
        return;
    }

    QString workingDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select a folder to store temporary files in",
                                                              nullptr, QFileDialog::ShowDirsOnly);
    if (workingDirectory.isNull()) {
        return;
    }

    if (!_awsInterface) {
        _awsInterface = new AWSInterface;
    }

    _awsInterface->createWebPageFromResults(
        testResults, 
        workingDirectory, 
        updateAWSCheckBox, 
        diffImageRadioButton,
        ssimImageRadionButton,
        urlLineEdit,
        branch,
        user
    );
}