//
//  TestSuiteCreator.cpp
//
//  Created by Nissim Hadar on 6 Jul 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestSuiteCreator.h"
#include "Test.h"

#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

void TestSuiteCreator::createTestSuite(const QString& testDirectory) {
    QDomProcessingInstruction instruction = document.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
    document.appendChild(instruction);

    // root element
    QDomElement root = document.createElement("suite");
    document.appendChild(root);

    // id (empty seems to be OK)
    QDomElement idElement = document.createElement("id");
    root.appendChild(idElement);

    // name - our tests are in "Rendering"
    QDomElement nameElement = document.createElement("name");
    QDomText nameElementText = document.createTextNode("Rendering");
    nameElement.appendChild(nameElementText);
    root.appendChild(nameElement);

    // We create a single section, within sections
    QDomElement topLevelSections = document.createElement("sections");
    
    QDomElement topLevelSection = document.createElement("section");

    QDomElement suiteName = document.createElement("name");
    QDomText suiteNameElementElementText = document.createTextNode("Test Suite - " + QDateTime::currentDateTime().toString());
    suiteName.appendChild(suiteNameElementElementText);
    topLevelSection.appendChild(suiteName);

    QDomElement secondLevelSections = document.createElement("sections");
    QDomElement tests = processDirectory(testDirectory, secondLevelSections);
    topLevelSection.appendChild(tests);


    topLevelSection.appendChild(secondLevelSections);
    topLevelSections.appendChild(topLevelSection);

    root.appendChild(topLevelSections);

    // Write to file
    const QString testRailsFilename{ "D:/t/TestRailSuite.xml" };
    QFile file(testRailsFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Could not create XML file");
        exit(-1);
    }

    QTextStream stream(&file);
    stream << document.toString();

    file.close();

    QMessageBox::information(0, "Success", "TestRail XML file has been created");
}

QDomElement TestSuiteCreator::processDirectory(const QString& directory, const QDomElement& element) {
    QDomElement result = element;

    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir;
        if (Test::isAValidDirectory(directory)) {
            // Ignore the utils directory
            if (directory.right(5) == "utils") {
                continue;
            }

            // Create a section and process it
            // The directory name appears after the last slash (we are assured there is at least 1).
            QString directoryName = directory.right(directory.length() - directory.lastIndexOf("/") - 1);

            QDomElement sectionElement = document.createElement("section");

            QDomElement sectionElementName = document.createElement("name");
            QDomText sectionElementNameText = document.createTextNode(directoryName);
            sectionElementName.appendChild(sectionElementNameText);
            sectionElement.appendChild(sectionElementName);

            result.appendChild(sectionElement);
        }
    }

    return result;
}