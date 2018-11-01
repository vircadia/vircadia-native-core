//
//  AWSInterface.cpp
//
//  Created by Nissim Hadar on 3 Oct 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AWSInterface.h"

#include <QDirIterator>
#include <QMessageBox>
#include <QProcess>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

AWSInterface::AWSInterface(QObject* parent) : QObject(parent) {
    _pythonInterface = new PythonInterface();
    _pythonCommand = _pythonInterface->getPythonCommand();
}

void AWSInterface::createWebPageFromResults(const QString& testResults,
                                            const QString& snapshotDirectory,
                                            QCheckBox* updateAWSCheckBox,
                                            QLineEdit* urlLineEdit) {
    _testResults = testResults;
    _snapshotDirectory = snapshotDirectory;
    
    _urlLineEdit = urlLineEdit;
    _urlLineEdit->setEnabled(false);

    extractTestFailuresFromZippedFolder();
    createHTMLFile();

    if (updateAWSCheckBox->isChecked()) {
        updateAWS();
    }
}

void AWSInterface::extractTestFailuresFromZippedFolder() {
    // For a test results zip file called `D:/tt/TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ].zip`
    //   the folder will be called `TestResults--2018-10-02_16-54-11(9426)[DESKTOP-PMKNLSQ]`
    //   and, this folder will be in the working directory
    QStringList parts =_testResults.split('/');
    QString zipFolderName = _snapshotDirectory + "/" + parts[parts.length() - 1].split('.')[0];
    if (QDir(zipFolderName).exists()) {
        QDir dir = zipFolderName;
        dir.removeRecursively();
    }

    JlCompress::extractDir(_testResults, _snapshotDirectory);
}

void AWSInterface::createHTMLFile() {
    // For file named `D:/tt/snapshots/TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ].zip` 
    //    - the HTML will be in a folder named `TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ]`
    QStringList pathComponents = _testResults.split('/');
    QString filename = pathComponents[pathComponents.length() - 1];
    _resultsFolder = filename.left(filename.length() - 4);

    QString resultsPath = _snapshotDirectory + "/" + _resultsFolder + "/";
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
    writeTitle(stream);
    writeTable(stream);
    stream << "\t" << "</body>\n";
}

void AWSInterface::finishHTMLpage(QTextStream& stream) {
    stream << "</html>\n";
}

void AWSInterface::writeTitle(QTextStream& stream) {
    // Separate relevant components from the results name
    // The expected format is as follows: `D:/tt/snapshots/TestResults--2018-10-04_11-09-41(PR14128)[DESKTOP-PMKNLSQ].zip`
    QStringList tokens = _testResults.split('/');

    // date_buildorPR_hostName will be 2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ]
    QString date_buildorPR_hostName = tokens[tokens.length() - 1].split("--")[1].split(".")[0];

    QString buildorPR = date_buildorPR_hostName.split('(')[1].split(')')[0];
    QString hostName  = date_buildorPR_hostName.split('[')[1].split(']')[0];

    QStringList dateList = date_buildorPR_hostName.split('(')[0].split('_')[0].split('-');
    QString year = dateList[0];
    QString month = dateList[1];
    QString day = dateList[2];

    QStringList timeList = date_buildorPR_hostName.split('(')[0].split('_')[1].split('-');
    QString hour = timeList[0];
    QString minute = timeList[1];
    QString second = timeList[2];

    const QString months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    stream << "\t" << "\t" << "<font color=\"red\">\n";
    stream << "\t" << "\t" << "<h1>Failures for ";
    stream << months[month.toInt() - 1] << " " << day << ", " << year << ", ";
    stream << hour << ":" << minute << ":" << second << ", ";

    if (buildorPR.left(2) == "PR") {
        stream << "PR " << buildorPR.right(buildorPR.length() - 2) << ", ";
    } else {
        stream << "build " << buildorPR << ", ";
    }

    stream << "run on " << hostName << "</h1>\n";
}

void AWSInterface::writeTable(QTextStream& stream) {
    QString previousTestName{ "" };

    // Loop over all entries in directory.  This is done in stages, as the names are not in the order of the tests
    //      The first stage reads the directory names into a list 
    //      The second stage renames the tests by removing everything up to "--tests."
    //      The third stage renames the directories
    //      The fourth and lasts stage creates the HTML entries
    //
    // Note that failures are processed first, then successes
    QStringList originalNamesFailures;
    QStringList originalNamesSuccesses;
    QDirIterator it1(_snapshotDirectory.toStdString().c_str());
    while (it1.hasNext()) {
        QString nextDirectory = it1.next();

        // Skip `.` and `..` directories
        if (nextDirectory.right(1) == ".") {
            continue;
        }

        // Only process failure folders
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

    QStringList newNamesFailures;
    for (int i = 0; i < originalNamesFailures.length(); ++i) {
        newNamesFailures.append(originalNamesFailures[i].split("--tests.")[1]);
    }

    QStringList newNamesSuccesses;
    for (int i = 0; i < originalNamesSuccesses.length(); ++i) {
        newNamesSuccesses.append(originalNamesSuccesses[i].split("--tests.")[1]);
    }
    
    _htmlFailuresFolder = _snapshotDirectory + "/" + _resultsFolder + "/" + FAILURES_FOLDER;
    QDir().mkdir(_htmlFailuresFolder);

    _htmlSuccessesFolder = _snapshotDirectory + "/" + _resultsFolder + "/" + SUCCESSES_FOLDER;
    QDir().mkdir(_htmlSuccessesFolder);

    for (int i = 0; i < newNamesFailures.length(); ++i) {
        QDir().rename(originalNamesFailures[i], _htmlFailuresFolder + "/" + newNamesFailures[i]);
    }

    for (int i = 0; i < newNamesSuccesses.length(); ++i) {
        QDir().rename(originalNamesSuccesses[i], _htmlSuccessesFolder + "/" + newNamesSuccesses[i]);
    }

    QDirIterator it2((_htmlFailuresFolder).toStdString().c_str());
    while (it2.hasNext()) {
        QString nextDirectory = it2.next();

        // Skip `.` and `..` directories, as well as the HTML directory
        if (nextDirectory.right(1) == "." || nextDirectory.contains(QString("/") + _resultsFolder + "/TestResults--")) {
            continue;
        }

        QStringList pathComponents = nextDirectory.split('/');
        QString filename = pathComponents[pathComponents.length() - 1];
        int splitIndex = filename.lastIndexOf(".");
        QString testName = filename.left(splitIndex).replace(".", " / ");
        QString testNumber = filename.right(filename.length() - (splitIndex + 1));

        // The failures are ordered lexicographically, so we know that we can rely on the testName changing to create a new table
        if (testName != previousTestName) {
            if (!previousTestName.isEmpty()) {
                closeTable(stream);
            }

            previousTestName = testName;

            stream << "\t\t<h2>" << testName << "</h2>\n";

            openTable(stream);
        }

        createEntry(testNumber.toInt(), filename, stream, true);
    }

    closeTable(stream);
    stream << "\t" << "\t" << "<font color=\"blue\">\n";
    stream << "\t" << "\t" << "<h1>The following tests passed:</h1>";

    QDirIterator it3((_htmlSuccessesFolder).toStdString().c_str());
    while (it3.hasNext()) {
        QString nextDirectory = it3.next();

        // Skip `.` and `..` directories, as well as the HTML directory
        if (nextDirectory.right(1) == "." || nextDirectory.contains(QString("/") + _resultsFolder + "/TestResults--")) {
            continue;
        }

        QStringList pathComponents = nextDirectory.split('/');
        QString filename = pathComponents[pathComponents.length() - 1];
        int splitIndex = filename.lastIndexOf(".");
        QString testName = filename.left(splitIndex).replace(".", " / ");
        QString testNumber = filename.right(filename.length() - (splitIndex + 1));

        // The failures are ordered lexicographically, so we know that we can rely on the testName changing to create a new table
        if (testName != previousTestName) {
            if (!previousTestName.isEmpty()) {
                closeTable(stream);
            }

            previousTestName = testName;

            stream << "\t\t<h2>" << testName << "</h2>\n";

            openTable(stream);
        }

        createEntry(testNumber.toInt(), filename, stream, false);
    }

    closeTable(stream);
}

void AWSInterface::openTable(QTextStream& stream) {
    stream << "\t\t<table>\n";
    stream << "\t\t\t<tr>\n";
    stream << "\t\t\t\t<th><h1>Test</h1></th>\n";
    stream << "\t\t\t\t<th><h1>Actual Image</h1></th>\n";
    stream << "\t\t\t\t<th><h1>Expected Image</h1></th>\n";
    stream << "\t\t\t\t<th><h1>Difference Image</h1></th>\n";
    stream << "\t\t\t</tr>\n";
}

void AWSInterface::closeTable(QTextStream& stream) {
    stream << "\t\t</table>\n";
}

void AWSInterface::createEntry(int index, const QString& testResult, QTextStream& stream, const bool isFailure) {
    stream << "\t\t\t<tr>\n";
    stream << "\t\t\t\t<td><h1>" << QString::number(index) << "</h1></td>\n";
    
    // For a test named `D:/t/fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf/Failure_1--tests.engine.interaction.pick.collision.many.00000`
    // we need `Failure_1--tests.engine.interaction.pick.collision.many.00000`
    QStringList resultNameComponents = testResult.split('/');
    QString resultName = resultNameComponents[resultNameComponents.length() - 1];

    QString folder;
    bool differenceFileFound;
    if (isFailure) {
        folder = FAILURES_FOLDER;
        differenceFileFound = QFile::exists(_htmlFailuresFolder + "/" + resultName + "/Difference Image.png");
    } else {
        folder = SUCCESSES_FOLDER;    
        differenceFileFound = QFile::exists(_htmlSuccessesFolder + "/" + resultName + "/Difference Image.png");
    }

  
    stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/Actual Image.png\" width = \"576\" height = \"324\" ></td>\n";
    stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/Expected Image.png\" width = \"576\" height = \"324\" ></td>\n";

    if (differenceFileFound) {
        stream << "\t\t\t\t<td><img src=\"./" << folder << "/" << resultName << "/Difference Image.png\" width = \"576\" height = \"324\" ></td>\n";
    } else {
        stream << "\t\t\t\t<td><h2>No Image Found</h2>\n";
    }

    stream << "\t\t\t</tr>\n";
}

void AWSInterface::updateAWS() {
    QString filename = _snapshotDirectory + "/updateAWS.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'addTestCases.py'");
        exit(-1);
    }

    QTextStream stream(&file);

    stream << "import boto3\n";
    stream << "s3 = boto3.resource('s3')\n\n";

    QDirIterator it1(_htmlFailuresFolder.toStdString().c_str());
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

        stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
               << "Actual Image.png"
               << "', 'rb')\n";

        stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Actual Image.png" << "', Body=data)\n\n";

        stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
               << "Expected Image.png"
               << "', 'rb')\n";

        stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Expected Image.png" << "', Body=data)\n\n";

        if (QFile::exists(_htmlFailuresFolder + "/" + parts[parts.length() - 1] + "/Difference Image.png")) {
            stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
                   << "Difference Image.png"
                   << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Difference Image.png" << "', Body=data)\n\n";
        }
    }

    QDirIterator it2(_htmlSuccessesFolder.toStdString().c_str());
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

        stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
               << "Actual Image.png"
               << "', 'rb')\n";

        stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Actual Image.png" << "', Body=data)\n\n";

        stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
               << "Expected Image.png"
               << "', 'rb')\n";

        stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Expected Image.png" << "', Body=data)\n\n";

        if (QFile::exists(_htmlSuccessesFolder + "/" + parts[parts.length() - 1] + "/Difference Image.png")) {
            stream << "data = open('" << _snapshotDirectory << "/" << filename << "/"
                   << "Difference Image.png"
                   << "', 'rb')\n";

            stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << filename << "/" << "Difference Image.png" << "', Body=data)\n\n";
        }
    }

    stream << "data = open('" << _snapshotDirectory << "/" << _resultsFolder << "/" << HTML_FILENAME << "', 'rb')\n";
    stream << "s3.Bucket('hifi-content').put_object(Bucket='" << AWS_BUCKET << "', Key='" << _resultsFolder << "/"
           << HTML_FILENAME << "', Body=data, ContentType='text/html')\n";

    file.close();

    // Show user the URL
    _urlLineEdit->setEnabled(true);
    _urlLineEdit->setText(QString("https://") + AWS_BUCKET + ".s3.amazonaws.com/" + _resultsFolder + "/" + HTML_FILENAME);
    _urlLineEdit->setCursorPosition(0);

    QProcess* process = new QProcess();

    connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });
    connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

#ifdef Q_OS_WIN
    QStringList parameters = QStringList() << filename ;
    process->start(_pythonCommand, parameters);
#elif defined Q_OS_MAC
    QStringList parameters = QStringList() << "-c" <<  _pythonCommand + " " + filename;
    process->start("sh", parameters);
#endif
}
