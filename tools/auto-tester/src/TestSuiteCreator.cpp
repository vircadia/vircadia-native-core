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

    // We create a single section, within sections
    QDomElement root = document.createElement("sections");
    document.appendChild(root);

    QDomElement topLevelSection = document.createElement("section");

    QDomElement suiteName = document.createElement("name");
    suiteName.appendChild(document.createTextNode("Test Suite - " + QDateTime::currentDateTime().toString()));
    topLevelSection.appendChild(suiteName);

    QDomElement secondLevelSections = document.createElement("sections");
    topLevelSection.appendChild(processDirectory(testDirectory, secondLevelSections));

    topLevelSection.appendChild(secondLevelSections);
    root.appendChild(topLevelSection);

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
         QString nextDirectory = it.next();

       // The object name appears after the last slash (we are assured there is at least 1).
        QString objectName = nextDirectory.right(nextDirectory.length() - nextDirectory.lastIndexOf("/") - 1);

        // Only process directories
        if (Test::isAValidDirectory(nextDirectory)) {
            // Ignore the utils and preformance directories
            if (nextDirectory.right(QString("utils").length()) == "utils" || nextDirectory.right(QString("performance").length()) == "performance") {
                continue;
            }

            // Create a section and process it

            QDomElement sectionElement = document.createElement("section");

            QDomElement sectionElementName = document.createElement("name");
            sectionElementName.appendChild(document.createTextNode(objectName));
            sectionElement.appendChild(sectionElementName);

            QDomElement testsElement = document.createElement("sections");
            sectionElement.appendChild(processDirectory(nextDirectory, testsElement));

            result.appendChild(sectionElement);
        } else {
            if (objectName == "test.js") {
                QDomElement sectionElement = document.createElement("section");
                QDomElement sectionElementName = document.createElement("name");
                sectionElementName.appendChild(document.createTextNode("all"));
                sectionElement.appendChild(sectionElementName);
                sectionElement.appendChild(processTest(nextDirectory, objectName, document.createElement("cases")));
                result.appendChild(sectionElement);
            }
        }
    }

    return result;
}

QDomElement TestSuiteCreator::processTest(const QString& fullDirectory, const QString& test, const QDomElement& element) {
    QDomElement result = element;
   
    QDomElement caseElement = document.createElement("case");

    caseElement.appendChild(document.createElement("id"));

    // The name of the test is derived from the full path.
    // The first term is the first word after "tests"
    // The last word is the penultimate word
    QStringList words = fullDirectory.split('/');
    int i = 0;
    while (words[i] != "tests") {
        ++i;
        if (i >= words.length() - 1) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Folder \"tests\" not found  in " + fullDirectory);
            exit(-1);
        }
    }

    ++i;
    QString title{ words[i] };
    for (++i; i < words.length() - 1; ++i) {
        title += " / " + words[i];
    }

    QDomElement titleElement = document.createElement("title");
    titleElement.appendChild(document.createTextNode(title));
    caseElement.appendChild(titleElement);

    QDomElement templateElement = document.createElement("template");
    templateElement.appendChild(document.createTextNode("Test Case (Steps)"));
    caseElement.appendChild(templateElement);

    QDomElement typeElement = document.createElement("type");
    typeElement.appendChild(document.createTextNode("3 - Regression"));
    caseElement.appendChild(typeElement);

    QDomElement priorityElement = document.createElement("priority");
    priorityElement.appendChild(document.createTextNode("Medium"));
    caseElement.appendChild(priorityElement);

    QDomElement estimateElementName = document.createElement("estimate");
    estimateElementName.appendChild(document.createTextNode("60"));
    caseElement.appendChild(estimateElementName);

    caseElement.appendChild(document.createElement("references"));

    QDomElement customElement = document.createElement("custom");

    QDomElement tester_countElement = document.createElement("tester_count");
    tester_countElement.appendChild(document.createTextNode("1"));
    customElement.appendChild(tester_countElement);

    QDomElement domain_bot_loadElement = document.createElement("domain_bot_load");
    domain_bot_loadElement.appendChild(document.createElement("id"));
    QDomElement domain_bot_loadElementValue = document.createElement("value");
    domain_bot_loadElementValue.appendChild(document.createTextNode(" With Bots (hifiqa-rc-bots / hifi-qa-stable-bots / hifiqa-master-bots)"));
    domain_bot_loadElement.appendChild(domain_bot_loadElementValue);
    customElement.appendChild(domain_bot_loadElement);

    QDomElement automation_typeElement = document.createElement("automation_type");
    automation_typeElement.appendChild(document.createElement("id"));
    QDomElement automation_typeElementValue = document.createElement("value");
    automation_typeElementValue.appendChild(document.createTextNode("None"));
    automation_typeElement.appendChild(automation_typeElementValue);
    customElement.appendChild(automation_typeElement);

    QDomElement added_to_releaseElement = document.createElement("added_to_release");
    QDomElement added_to_releaseElementId = document.createElement("id");
    added_to_releaseElement.appendChild(document.createElement("id"));
    QDomElement added_to_releaseElementValue = document.createElement("value");
    added_to_releaseElementValue.appendChild(document.createTextNode("RC68"));
    added_to_releaseElement.appendChild(added_to_releaseElementValue);
    customElement.appendChild(added_to_releaseElement);

    QDomElement precondsElement = document.createElement("preconds");
    precondsElement.appendChild(document.createTextNode("Tester is in an empty region of a domain in which they have edit rights\n\n*Note: Press 'n' to advance test script"));
    customElement.appendChild(precondsElement);

    QDomElement steps_seperatedElement = document.createElement("steps_separated");
    QDomElement stepElement = document.createElement("step");
    QDomElement stepIndexElement = document.createElement("index");
    stepIndexElement.appendChild(document.createTextNode("1"));
    stepElement.appendChild(stepIndexElement);
    QDomElement stepContentElement = document.createElement("content");
    stepContentElement.appendChild(document.createTextNode("Execute instructions in [THIS TEST](https://github.com/highfidelity/hifi_tests/blob/RC70/tests/content/entity/light/point/create/test.md)"));
    stepElement.appendChild(stepContentElement);
    QDomElement stepExpectedElement = document.createElement("expected");
    stepExpectedElement.appendChild(document.createTextNode("Refer to the expected result in the linked description."));
    stepElement.appendChild(stepExpectedElement);
    steps_seperatedElement.appendChild(stepElement);
    customElement.appendChild(steps_seperatedElement);

    QDomElement notesElement = document.createElement("notes");
    notesElement.appendChild(document.createTextNode("https://github.com/highfidelity/hifi_tests/blob/RC70/tests/content/entity/light/point/create/test.md"));
    customElement.appendChild(notesElement);

    caseElement.appendChild(customElement);

    result.appendChild(caseElement);
 
    return result;
}