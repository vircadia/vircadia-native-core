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

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

AWSInterface::AWSInterface(QObject* parent) :
    QObject(parent) {
}

void AWSInterface::createWebPageFromResults(const QString& testResults, const QString& tempDirectory) {
    // Extract test failures from zipped folder
    _tempDirectory = tempDirectory;

    QDir dir = _tempDirectory;
    dir.mkdir(_tempDirectory);
    JlCompress::extractDir(testResults, _tempDirectory);

    createHTMLFile(testResults, tempDirectory);
}

void AWSInterface::createHTMLFile(const QString& testResults, const QString& tempDirectory) {
    // For file named `D:/tt/snapshots/TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ].zip` 
    //    - the HTML will be named `TestResults--2018-10-03_15-35-28(9433)[DESKTOP-PMKNLSQ]`
    QString resultsPath = tempDirectory + "/" + resultsFolder + "/";
    QDir().mkdir(resultsPath);
    QStringList tokens = testResults.split('/');
    QString htmlFilename = resultsPath + tokens[tokens.length() - 1].split('.')[0] + ".html";

    QFile file(htmlFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create '" + htmlFilename + "'");
        exit(-1);
    }

    QTextStream stream(&file);

    startHTMLpage(stream);
    writeHead(stream);
    writeBody(testResults, stream);
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

void AWSInterface::writeBody(const QString& testResults, QTextStream& stream) {
    stream << "\t" << "<body>\n";
    writeTitle(testResults, stream);
    writeTable(stream);
    stream << "\t" << "</body>\n";
}

void AWSInterface::finishHTMLpage(QTextStream& stream) {
    stream << "</html>\n";
}

void AWSInterface::writeTitle(const QString& testResults, QTextStream& stream) {
    // Separate relevant components from the results name
    // The expected format is as follows: `D:/tt/snapshots/TestResults--2018-10-04_11-09-41(PR14128)[DESKTOP-PMKNLSQ].zip`
    QStringList tokens = testResults.split('/');

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

    QStringList originalNames;
    QDirIterator it1(_tempDirectory.toStdString().c_str());
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

        originalNames.append(nextDirectory);
    }

    QStringList newNames;
    for (int i = 0; i < originalNames.length(); ++i) {
        newNames.append(originalNames[i].split("--tests.")[1]);
    }
    
    for (int i = 0; i < newNames.length(); ++i) {
        QDir dir(originalNames[i]);
        dir.rename(originalNames[i], _tempDirectory + "/" + resultsFolder + "/" + newNames[i]);
    }

    QDirIterator it2((_tempDirectory + "/" + resultsFolder).toStdString().c_str());
    while (it2.hasNext()) {
        QString nextDirectory = it2.next();

        // Skip `.` and `..` directories, as well as the HTML directory
        if (nextDirectory.right(1) == "." || nextDirectory.contains(QString("/") + resultsFolder + "/TestResults--")) {
            continue;
        }

        int splitIndex = nextDirectory.lastIndexOf(".");
        QString testName = nextDirectory.left(splitIndex).replace(".", " / ");
        QString testNumber = nextDirectory.right(nextDirectory.length() - (splitIndex + 1));

        // The failures are ordered lexicographically, so we know that we can rely on the testName changing to create a new table
        if (testName != previousTestName) {
            if (!previousTestName.isEmpty()) {
                closeTable(stream);
            }

            previousTestName = testName;

            stream << "\t\t<h2>" << testName << "</h2>\n";

            openTable(stream);
        }

        createEntry(testNumber.toInt(), nextDirectory, stream);
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

void AWSInterface::createEntry(int index, const QString& testFailure, QTextStream& stream) {
    stream << "\t\t\t<tr>\n";
    stream << "\t\t\t\t\<td><h1>" << QString::number(index) << "</h1></td>\n";
    
    // For a test named `D:/t/fgadhcUDHSFaidsfh3478JJJFSDFIUSOEIrf/Failure_1--tests.engine.interaction.pick.collision.many.00000`
    // we need `Failure_1--tests.engine.interaction.pick.collision.many.00000`
    QStringList failureNameComponents = testFailure.split('/');
    QString failureName = failureNameComponents[failureNameComponents.length() - 1];

    stream << "\t\t\t\t<td><img src=\"" << failureName << "/Actual Image.png\" width = \"576\" height = \"324\" ></td>\n";
    stream << "\t\t\t\t<td><img src=\"" << failureName << "/Expected Image.png\" width = \"576\" height = \"324\" ></td>\n";
    stream << "\t\t\t\t<td><img src=\"" << failureName << "/Difference Image.png\" width = \"576\" height = \"324\" ></td>\n";
    stream << "\t\t\t</tr>\n";
}