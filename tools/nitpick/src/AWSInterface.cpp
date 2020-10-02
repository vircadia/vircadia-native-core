//
//  AWSInterface.cpp
//
//  Created by Nissim Hadar on 3 Oct 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  ##### NOTICE: THIS FILE NEEDS TO BE UPDATED WITH THE CORRECT LATEST BUCKET.
//
#include "AWSInterface.h"
#include "common.h"

#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

AWSInterface::AWSInterface(QObject* parent) : QObject(parent) {
    _pythonInterface = new PythonInterface();
    _pythonCommand = _pythonInterface->getPythonCommand();
}

void AWSInterface::createWebPageFromResults(const QString& testResults,
                                            const QString& workingDirectory,
                                            QCheckBox* updateAWSCheckBox,
                                            QRadioButton* diffImageRadioButton,
                                            QRadioButton* ssimImageRadionButton,
                                            QLineEdit* urlLineEdit,
                                            const QString& branch,
                                            const QString& user
) {
    _workingDirectory = workingDirectory;

    // Verify filename is in correct format 
    // For example `D:/tt/TestResults--2019-02-10_17-30-57(local)[DESKTOP-6BO62Q9].zip`
    QStringList parts = testResults.split('/');
    QString zipFilename = parts[parts.length() - 1];

    QStringList zipFolderNameParts = zipFilename.split(QRegExp("[\\(\\)\\[\\]]"), QString::SkipEmptyParts);

    if (!QRegularExpression("TestResults--\\d{4}(-\\d\\d){2}_\\d\\d(-\\d\\d){2}").match(zipFolderNameParts[0]).hasMatch() ||
        !QRegularExpression("\\w").match(zipFolderNameParts[1]).hasMatch() ||                                                 // build (local, build number or PR number)
        !QRegularExpression("\\w").match(zipFolderNameParts[2]).hasMatch()                                                  // machine name
    ) {
        QMessageBox::critical(0, "Filename is in wrong format", "'" + zipFilename + "' is not in nitpick format");
        return;
    }

    _testResults = testResults;

    _urlLineEdit = urlLineEdit;
    _urlLineEdit->setEnabled(false);

    _branch = branch;
    _user = user;

    QString zipFilenameWithoutExtension = zipFilename.split('.')[0];
    extractTestFailuresFromZippedFolder(_workingDirectory + "/" + zipFilenameWithoutExtension);

    if (diffImageRadioButton->isChecked()) {
        _comparisonImageFilename = "Difference Image.png";
    } else {
        _comparisonImageFilename = "SSIM Image.png";
    }
        
    createHTMLFile();

    if (updateAWSCheckBox->isChecked()) {
        updateAWS();
        QMessageBox::information(0, "Success", "HTML file has been created and copied to AWS");
    } else {
        QMessageBox::information(0, "Success", "HTML file has been created");
    }
}

void AWSInterface::extractTestFailuresFromZippedFolder(const QString& folderName) {
    // For a test results zip file called `D:/tt/TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ].zip`
    //   the folder will be called `TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]`
    //   and, this folder will be in the working directory
    if (QDir(folderName).exists()) {
        QDir dir = folderName;
        dir.removeRecursively();
    }

    JlCompress::extractDir(_testResults, _workingDirectory);
}

void AWSInterface::createHTMLFile() {
    // For file named `D:/tt/snapshots/TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ].zip` 
    //    - the HTML will be in a folder named `TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ]`
    QStringList pathComponents = _testResults.split('/');
    QString filename = pathComponents[pathComponents.length() - 1];
    _resultsFolder = filename.left(filename.length() - 4);

    QString resultsPath = _workingDirectory + "/" + _resultsFolder + "/";
    QDir().mkdir(resultsPath);
    _htmlFilename = resultsPath + HTML_FILENAME;

    QFile file(_htmlFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create '" + _htmlFilename + "'");
        exit(-1);
    }

    QTextStream stream(&file);

    startHTMLpage(stream);
    writeHead(stream);
    writeBody(stream);
    finishHTMLpage(stream);

    file.close();
}

void AWSInterface::startHTMLpage(QTextStream& stream) {
    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n";
}

void AWSInterface::writeHead(QTextStream& stream) {
    stream << "\t" << "<head>\n";
    stream << "\t" << "\t" << "<style>\n";
    stream << "\t" << "\t" << "\t" << "table, th, td {\n";
    stream << "\t" << "\t" << "\t" << "\t" << "border: 1px solid blue;\n";
    stream << "\t" << "\t" << "\t" << "}\n";
    stream << "\t" << "\t" << "</style>\n";
    stream << "\t" << "</head>\n";
}

void AWSInterface::writeBody(QTextStream& stream) {
    stream << "\t" << "<body>\n";

    // The results are read here as they are used both in the title (for the summary) and for table
    QStringList originalNamesFailures;
    QStringList originalNamesSuccesses;
    QDirIterator it1(_workingDirectory);
    while (it1.hasNext()) {
        QString nextDirectory = it1.next();

        // Skip `.` and `..` directories
        if (nextDirectory.right(1) == ".") {
            continue;
        }

        // Only process result folders
        if (!nextDirectory.contains("--tests.")) {
            continue;
        }

        // Look at the filename at the end of the path
        QStringList parts = nextDirectory.split('/');
        QString name = parts[parts.length() - 1];
        if (name.left(7) == "Failure") {
            originalNamesFailures.append(nextDirectory);
        } else {
            originalNamesSuccesses.append(nextDirectory);
        }
    }

    writeTitle(stream, originalNamesFailures, originalNamesSuccesses);
    writeTable(stream, originalNamesFailures, originalNamesSuccesses);
    stream << "\t" << "</body>\n";
}

void AWSInterface::finishHTMLpage(QTextStream& stream) {
    stream << "</html>\n";
}

void AWSInterface::writeTitle(QTextStream& stream, const QStringList& originalNamesFailures, const QStringList& originalNamesSuccesses) {
    // Separate relevant components from the results name
    // The expected format is as follows: `D:/tt/snapshots/TestResults--2018-10-04_11-09-41(PR14128)[DESKTOP-PMKNLSQ].zip`
    QStringList tokens = _testResults.split('/');

    // date_buildorPR_hostName will be 2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ]
    QString date_buildorPR_hostName = tokens[tokens.length() - 1].split("--")[1].split(".")[0];

    QString buildorPR = date_buildorPR_hostName.split('(')[1].split(')')[0];
    QString hostName = date_buildorPR_hostName.split('[')[1].split(']')[0];

    QStringList dateList = date_buildorPR_hostName.split('(')[0].split('_')[0].split('-');
    QString year = dateList[0];
    QString month = dateList[1];
    QString day = dateList[2];

    QStringList timeList = date_buildorPR_hostName.split('(')[0].split('_')[1].split('-');
    QString hour = timeList[0];
    QString minute = timeList[1];
    QString second = timeList[2];

    const QString months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    stream << "\t" << "\t" << "<font color=\"Green\">\n";
    stream << "\t" << "\t" << "<h1>Results for ";
    stream << months[month.toInt() - 1] << " " << day << ", " << year << ", ";
    stream << hour << ":" << minute << ":" << second << ", ";

    if (buildorPR.left(2) == "PR") {
        stream << "PR " << buildorPR.right(buildorPR.length() - 2) << ", ";
    } else {
        stream << "build " << buildorPR << ", ";
    }

    stream << "run on " << hostName << "</h1>\n";

    stream << "<h2>";
    stream << "nitpick  " << nitpickVersion;
    stream << ", tests from GitHub: " << _user << "/" << _branch;
    stream << "</h2>";

    _numberOfFailures = originalNamesFailures.length();
    _numberOfSuccesses = originalNamesSuccesses.length();

    stream << "<h2>" << QString::number(_numberOfFailures) << " failed, out of a total of " << QString::number(_numberOfFailures + _numberOfSuccesses) << " tests</h2>\n";

    stream << "\t" << "\t" << "<font color=\"red\">\n";
 
    if (_numberOfFailures > 0) {
        stream << "\t" << "\t" << "<h1>The following tests failed:</h1>";
    }
}

void AWSInterface::writeTable(QTextStream& stream, const QStringList& originalNamesFailures, const QStringList& originalNamesSuccesses) {
    QString previousTestName{ "" };

    // Loop over all entries in directory.  This is done in stages, as the names are not in the order of the tests
    //      The first stage reads the directory names into a list 
    //      The second stage renames the tests by removing everything up to "--tests."
    //      The third stage renames the directories
    //      The fourth and lasts stage creates the HTML entries
    //
    // Note that failures are processed first, then successes

    QStringList newNamesFailures;
    for (int i = 0; i < originalNamesFailures.length(); ++i) {
        newNamesFailures.append(originalNamesFailures[i].split("--tests.")[1]);
    }

    QStringList newNamesSuccesses;
    for (int i = 0; i < originalNamesSuccesses.length(); ++i) {
        newNamesSuccesses.append(originalNamesSuccesses[i].split("--tests.")[1]);
    }

    _htmlFailuresFolder = _workingDirectory + "/" + _resultsFolder + "/" + FAILURES_FOLDER;
    QDir().mkdir(_htmlFailuresFolder);

    _htmlSuccessesFolder = _workingDirectory + "/" + _resultsFolder + "/" + SUCCESSES_FOLDER;
    QDir().mkdir(_htmlSuccessesFolder);

    for (int i = 0; i < newNamesFailures.length(); ++i) {
        QDir().rename(originalNamesFailures[i], _htmlFailuresFolder + "/" + newNamesFailures[i]);
    }

    for (int i = 0; i < newNamesSuccesses.length(); ++i) {
        QDir().rename(originalNamesSuccesses[i], _htmlSuccessesFolder + "/" + newNamesSuccesses[i]);
    }

    // Mac does not read folders in lexicographic order, so this step is divided into 2
    // Each test consists of the test name and its index.
    QStringList folderNames;

    QDirIterator it2(_htmlFailuresFolder);
    while (it2.hasNext()) {
        QString nextDirectory = it2.next();

        // Skip `.` and `..` directories, as well as the HTML directory
        if (nextDirectory.right(1) == "." || nextDirectory.contains(QString("/") + _resultsFolder + "/TestResults--")) {
            continue;
        }

        QStringList pathComponents = nextDirectory.split('/');
        QString folderName = pathComponents[pathComponents.length() - 1];

        folderNames << folderName;
    }

    folderNames.sort();
    for (const auto& folderName : folderNames) {
        int splitIndex = folderName.lastIndexOf(".");
        QString testName = folderName.left(splitIndex).replace('.', " / ");

        int testNumber = folderName.right(folderName.length() - (splitIndex + 1)).toInt();

        // The failures are ordered lexicographically, so we know that we can rely on the testName changing to create a new table
        if (testName != previousTestName) {
            if (!previousTestName.isEmpty()) {
                closeTable(stream);
            }

            previousTestName = testName;

            stream << "\t\t<h2>" << testName << "</h2>\n";
            openTable(stream, folderName, true);
        }

        createEntry(testNumber, folderName, stream, true);
    }

    closeTable(stream);
    stream << "\t" << "\t" << "<font color=\"blue\">\n";

    if (_numberOfSuccesses > 0) {
        stream << "\t" << "\t" << "<h1>The following tests passed:</h1>";
    }

    // Now do the same for passes
    folderNames.clear();

    QDirIterator it3(_htmlSuccessesFolder);
    while (it3.hasNext()) {
        QString nextDirectory = it3.next();

        // Skip `.` and `..` directories, as well as the HTML directory
        if (nextDirectory.right(1) == "." || nextDirectory.contains(QString("/") + _resultsFolder + "/TestResults--")) {
            continue;
        }

        QStringList pathComponents = nextDirectory.split('/');
        QString folderName = pathComponents[pathComponents.length() - 1];

        folderNames << folderName;
    }

    folderNames.sort();
    for (const auto& folderName : folderNames) {
        int splitIndex = folderName.lastIndexOf(".");
        QString testName = folderName.left(splitIndex).replace('.', " / ");

        int testNumber = folderName.right(folderName.length() - (splitIndex + 1)).toInt();

        // The failures are ordered lexicographically, so we know that we can rely on the testName changing to create a new table
        if (testName != previousTestName) {
            if (!previousTestName.isEmpty()) {
                closeTable(stream);
            }

            previousTestName = testName;

            stream << "\t\t<h2>" << testName << "</h2>\n";
            openTable(stream, folderName, false);
        }

        createEntry(testNumber, folderName, stream, false);
    }

    closeTable(stream);
}

void AWSInterface::openTable(QTextStream& stream, const QString& testResult, const bool isFailure) {
    QStringList resultNameComponents = testResult.split('/');
    QString resultName = resultNameComponents[resultNameComponents.length() - 1];

    bool textResultsFileFound;
    if (isFailure) {
        textResultsFileFound = QFile::exists(_htmlFailuresFolder + "/" + resultName + "/Result.txt");
    } else {
        textResultsFileFound = QFile::exists(_htmlSuccessesFolder + "/" + resultName + "/Result.txt");
    }

    if (textResultsFileFound) {
        if (isFailure) {
            stream << "\t\t<table>\n";
            stream << "\t\t\t<tr>\n";
            stream << "\t\t\t\t<th><h1>Element</h1></th>\n";
            stream << "\t\t\t\t<th><h1>Actual Value</h1></th>\n";
            stream << "\t\t\t\t<th><h1>Expected Value</h1></th>\n";
            stream << "\t\t\t</tr>\n";
        } else {
            stream << "\t\t<h3>No errors found</h3>\n\n";
            stream << "\t\t<h3>===============</h3>\n\n";
        }
    } else {
        stream << "\t\t<table>\n";
        stream << "\t\t\t<tr>\n";
        stream << "\t\t\t\t<th><h1>Test</h1></th>\n";
        stream << "\t\t\t\t<th><h1>Actual Image</h1></th>\n";
        stream << "\t\t\t\t<th><h1>Expected Image</h1></th>\n";
        stream << "\t\t\t\t<th><h1>Comparison Image</h1></th>\n";
        stream << "\t\t\t</tr>\n";
    }
}

void AWSInterface::closeTable(QTextStream& stream) {
    stream << "\t\t</table>\n";
}

void AWSInterface::createEntry(const int index, const QString& testResult, QTextStream& stream, const bool isFailure) {
    // For a test named `D:/t/fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf/Failure_1--tests.engine.interaction.pick.collision.many.00000`
    // we need `Failure_1--tests.engine.interaction.pick.collision.many.00000`
    QStringList resultNameComponents = testResult.split('/');
    QString resultName = resultNameComponents[resultNameComponents.length() - 1];

    QString textResultFilename;
    if (isFailure) {
        textResultFilename = _htmlFailuresFolder + "/" + resultName + "/Result.txt";
    } else {
        textResultFilename = _htmlSuccessesFolder + "/" + resultName + "/Result.txt";
    }
    bool textResultsFileFound{ QFile::exists(textResultFilename) };

    QString folder;
    bool differenceFileFound;
    
    if (isFailure) {
        folder = FAILURES_FOLDER;
        differenceFileFound = QFile::exists(_htmlFailuresFolder + "/" + resultName + "/" + _comparisonImageFilename);
    } else {
        folder = SUCCESSES_FOLDER;
        differenceFileFound = QFile::exists(_htmlSuccessesFolder + "/" + resultName + "/" + _comparisonImageFilename);
    }

    if (textResultsFileFound) {
        // Parse the JSON file
        QFile file;
        file.setFileName(textResultFilename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                "Failed to open file " + textResultFilename);
        }

        QString value = file.readAll();
        file.close();

        // The Result.txt file is an object containing elements such as the following:
        //    "angularDamping": {
        //        "actual": 0.3938899040222168,
        //        "expected" : 0.3938899,
        //        "result" : "pass"
        //    },
        //
        // Failures are thos element that have "fail for the result

        QJsonDocument document = QJsonDocument::fromJson(value.toUtf8());
        QJsonObject json = document.object();
        foreach(const QString& key, json.keys()) {
            QJsonValue value = json.value(key);
            QJsonObject object = value.toObject();

            QJsonValue actualValue = object.value("actual");
            QString actualValueString;
            if (actualValue.isString()) {
                actualValueString = actualValue.toString();
            } else if (actualValue.isBool()) {
                actualValueString = actualValue.toBool() ? "true" : "false";
            } else if (actualValue.isDouble()) {
                actualValueString = QString::number(actualValue.toDouble());
            }

            QJsonValue expectedValue = object.value("expected");
            QString expectedValueString;
            if (expectedValue.isString()) {
                expectedValueString = expectedValue.toString();
            } else if (expectedValue.isBool()) {
                expectedValueString = expectedValue.toBool() ? "true" : "false";
            } else if (expectedValue.isDouble()) {
                expectedValueString = QString::number(expectedValue.toDouble());
            }
            QString result = object.value("result").toString();
           
            if (result == "fail") {
                stream << "\t\t\t<tr>\n";
                stream << "\t\t\t\t<td><font size=\"6\">" + key + "</td>\n";
                stream << "\t\t\t\t<td><font size=\"6\">" + actualValueString + "</td>\n";
                stream << "\t\t\t\t<td><font size=\"6\">" + expectedValueString + "</td>\n";
                stream << "\t\t\t</tr>\n";
            }
        }
    } else {
        stream << "\t\t\t<tr>\n";
        stream << "\t\t\t\t<td><h1>" << QString::number(index) << "</h1></td>\n";

        stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/Actual Image.png\" width = \"576\" height = \"324\" ></td>\n";
        stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/Expected Image.png\" width = \"576\" height = \"324\" ></td>\n";

        if (differenceFileFound) {
            stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/" << _comparisonImageFilename << "\" width = \"576\" height = \"324\" ></td>\n";
        } else {
            stream << "\t\t\t\t<td><h2>Image size mismatch</h2>\n";
        }

        stream << "\t\t\t</tr>\n";
    }

}

void AWSInterface::updateAWS() {
    QString filename = _workingDirectory + "/updateAWS.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'updateAWS.py'");
        exit(-1);
    }

    QTextStream stream(&file);

    stream << "import boto3\n";
    stream << "s3 = boto3.resource('s3')\n\n";

    QDirIterator it1(_htmlFailuresFolder);
    while (it1.hasNext()) {
        QString nextDirectory = it1.next();

        // Skip `.` and `..` directories
        if (nextDirectory.right(1) == ".") {
            continue;
        }

        // nextDirectory looks like `D:/t/TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]/failures/engine.render.effect.bloom.00000`
        // We need to concatenate the last 3 components, to get `TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]/failures/engine.render.effect.bloom.00000`
        QStringList parts = nextDirectory.split('/');
        QString filename = parts[parts.length() - 3] + "/" + parts[parts.length() - 2] + "/" + parts[parts.length() - 1];

        // The directory may contain either 'Result.txt', or 3 images (and a text file named 'TestResults.txt' that is not used)
        if (QFile::exists(_htmlFailuresFolder + "/" + parts[parts.length() - 1] + "/Result.txt")) {
            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Result.txt"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Result.txt" << "', Body=data)\n\n";
        } else {
            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Actual Image.png"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Actual Image.png" << "', Body=data)\n\n";

            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Expected Image.png"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Expected Image.png" << "', Body=data)\n\n";

            if (QFile::exists(_htmlFailuresFolder + "/" + parts[parts.length() - 1] + "/" + _comparisonImageFilename)) {
                stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                    << _comparisonImageFilename
                    << "', 'rb')\n";

                stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << _comparisonImageFilename << "', Body=data)\n\n";
            }
        }
    }

    QDirIterator it2(_htmlSuccessesFolder);
    while (it2.hasNext()) {
        QString nextDirectory = it2.next();

        // Skip `.` and `..` directories
        if (nextDirectory.right(1) == ".") {
            continue;
        }

        // nextDirectory looks like `D:/t/TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]/successes/engine.render.effect.bloom.00000`
        // We need to concatenate the last 3 components, to get `TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]/successes/engine.render.effect.bloom.00000`
        QStringList parts = nextDirectory.split('/');
        QString filename = parts[parts.length() - 3] + "/" + parts[parts.length() - 2] + "/" + parts[parts.length() - 1];
        // The directory may contain either 'Result.txt', or 3 images (and a text file named 'TestResults.txt' that is not used)
        if (QFile::exists(_htmlSuccessesFolder + "/" + parts[parts.length() - 1] + "/Result.txt")) {
            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Result.txt"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Result.txt" << "', Body=data)\n\n";
        } else {
            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Actual Image.png"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Actual Image.png" << "', Body=data)\n\n";

            stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                << "Expected Image.png"
                << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Expected Image.png" << "', Body=data)\n\n";

            if (QFile::exists(_htmlSuccessesFolder + "/" + parts[parts.length() - 1] + "/" + _comparisonImageFilename)) {
                stream << "data = open('" << _workingDirectory << "/" << filename << "/"
                    << _comparisonImageFilename
                    << "', 'rb')\n";

                stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << _comparisonImageFilename << "', Body=data)\n\n";
            }
        }
    }

    stream << "data = open('" << _workingDirectory << "/" << _resultsFolder << "/" << HTML_FILENAME << "', 'rb')\n";
    stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << _resultsFolder << "/"
        << HTML_FILENAME << "', Body=data, ContentType='text/html')\n";

    file.close();

    // Show user the URL
    _urlLineEdit->setEnabled(true);
    _urlLineEdit->setText(QString("https://") + AWS_BUCKET + ".s3.amazonaws.com/" + _resultsFolder + "/" + HTML_FILENAME);
    _urlLineEdit->setCursorPosition(0);

    QProcess* process = new QProcess();

    _busyWindow.setWindowTitle("Updating AWS");
    connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });
    connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
        [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

#ifdef Q_OS_WIN
    QStringList parameters = QStringList() << filename;
    process->start(_pythonCommand, parameters);
#elif defined Q_OS_MAC
    QStringList parameters = QStringList() << "-c" << _pythonCommand + " " + filename;
    process->start("sh", parameters);
#endif
}
