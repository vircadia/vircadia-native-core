//
//  TestRailInterface.cpp
//
//  Created by Nissim Hadar on 6 Jul 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestRailInterface.h"
#include "Test.h"

#include "ui/TestRailSelectorWindow.h"

#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

TestRailInterface::TestRailInterface() {
    _testRailSelectorWindow.setModal(true);

    ////_testRailSelectorWindow.setURL("https://highfidelity.testrail.net");
    _testRailSelectorWindow.setURL("https://nissimhadar.testrail.io");
    ////_testRailSelectorWindow.setUser("@highfidelity.io");
    _testRailSelectorWindow.setUser("nissim.hadar@gmail.com");

    // 24 is the HighFidelity Interface project id in TestRail
    ////_testRailSelectorWindow.setProject(24);
    _testRailSelectorWindow.setProject(1);
}

// Creates the testrail.py script
// This is the file linked to from http://docs.gurock.com/testrail-api2/bindings-python
void TestRailInterface::createTestRailDotPyScript(const QString& outputDirectory) {
    QFile file(outputDirectory + "/testrail.py");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create \'testrail.py\'");
        exit(-1);
    }

    QTextStream stream(&file);

    stream << "#\n";
    stream << "# TestRail API binding for Python 3.x (API v2, available since \n";
    stream << "# TestRail 3.0)\n";
    stream << "#\n";
    stream << "# Learn more:\n";
    stream << "#\n";
    stream << "# http://docs.gurock.com/testrail-api2/start\n";
    stream << "# http://docs.gurock.com/testrail-api2/accessing\n";
    stream << "#\n";
    stream << "# Copyright Gurock Software GmbH. See license.md for details.\n";
    stream << "#\n";
    stream << "\n";
    stream << "import urllib.request, urllib.error\n";
    stream << "import json, base64\n";
    stream << "\n";
    stream << "class APIClient:\n";
    stream << "\tdef __init__(self, base_url):\n";
    stream << "\t\tself.user = ''\n";
    stream << "\t\tself.password = ''\n";
    stream << "\t\tif not base_url.endswith('/'):\n";
    stream << "\t\t\tbase_url += '/'\n";
    stream << "\t\tself.__url = base_url + 'index.php?/api/v2/'\n";
    stream << "\n";
    stream << "\t#\n";
    stream << "\t# Send Get\n";
    stream << "\t#\n";
    stream << "\t# Issues a GET request (read) against the API and returns the result\n";
    stream << "\t# (as Python dict).\n";
    stream << "\t#\n";
    stream << "\t# Arguments:\n";
    stream << "\t#\n";
    stream << "\t# uri                 The API method to call including parameters\n";
    stream << "\t#                     (e.g. get_case/1)\n";
    stream << "\t#\n";
    stream << "\tdef send_get(self, uri):\n";
    stream << "\t\treturn self.__send_request('GET', uri, None)\n";
    stream << "\n";
    stream << "\t#\n";
    stream << "\t# Send POST\n";
    stream << "\t#\n";
    stream << "\t# Issues a POST request (write) against the API and returns the result\n";
    stream << "\t# (as Python dict).\n";
    stream << "\t#\n";
    stream << "\t# Arguments:\n";
    stream << "\t#\n";
    stream << "\t# uri                 The API method to call including parameters\n";
    stream << "\t#                     (e.g. add_case/1)\n";
    stream << "\t# data                The data to submit as part of the request (as\n";
    stream << "\t#                     Python dict, strings must be UTF-8 encoded)\n";
    stream << "\t#\n";
    stream << "\tdef send_post(self, uri, data):\n";
    stream << "\t\treturn self.__send_request('POST', uri, data)\n";
    stream << "\n";
    stream << "\tdef __send_request(self, method, uri, data):\n";
    stream << "\t\turl = self.__url + uri\n";
    stream << "\t\trequest = urllib.request.Request(url)\n";
    stream << "\t\tif (method == 'POST'):\n";
    stream << "\t\t\trequest.data = bytes(json.dumps(data), 'utf-8')\n";
    stream << "\t\tauth = str(\n";
    stream << "\t\t\tbase64.b64encode(\n";
    stream << "\t\t\t\tbytes('%s:%s' % (self.user, self.password), 'utf-8')\n";
    stream << "\t\t\t),\n";
    stream << "\t\t\t'ascii'\n";
    stream << "\t\t).strip()\n";
    stream << "\t\trequest.add_header('Authorization', 'Basic %s' % auth)\n";
    stream << "\t\trequest.add_header('Content-Type', 'application/json')\n";
    stream << "\n";
    stream << "\t\te = None\n";
    stream << "\t\ttry:\n";
    stream << "\t\t\tresponse = urllib.request.urlopen(request).read()\n";
    stream << "\t\texcept urllib.error.HTTPError as ex:\n";
    stream << "\t\t\tresponse = ex.read()\n";
    stream << "\t\t\te = ex\n";
    stream << "\n";
    stream << "\t\tif response:\n";
    stream << "\t\t\tresult = json.loads(response.decode())\n";
    stream << "\t\telse:\n";
    stream << "\t\t\tresult = {}\n";
    stream << "\n";
    stream << "\t\tif e != None:\n";
    stream << "\t\t\tif result and 'error' in result:\n";
    stream << "\t\t\t\terror = \'\"\' + result[\'error\'] + \'\"\'\n";
    stream << "\t\t\telse:\n";
    stream << "\t\t\t\terror = \'No additional error message received\'\n";
    stream << "\t\t\traise APIError(\'TestRail API returned HTTP %s (%s)\' % \n";
    stream << "\t\t\t\t(e.code, error))\n";
    stream << "\n";
    stream << "\t\treturn result\n";
    stream << "\n";
    stream << "class APIError(Exception):\n";
    stream << "\tpass\n";

    file.close();
}

// Creates a Stack class
void TestRailInterface::createStackDotPyScript(const QString& outputDirectory) {
    QFile file(outputDirectory + "/stack.py");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create \'stack.py\'");
        exit(-1);
    }

    QTextStream stream(&file);

    stream << "class Stack:\n";

    stream << "\tdef __init__(self):\n";
    stream << "\t\tself.items = []\n";
    stream << "\n";

    stream << "\tdef is_empty(self):\n";
    stream << "\t\treturn self.items == []\n";
    stream << "\n";

    stream << "\tdef push(self, item):\n";
    stream << "\t\tself.items.append(item)\n";
    stream << "\n";

    stream << "\tdef pop(self):\n";
    stream << "\t\treturn self.items.pop()\n";
    stream << "\n";

    stream << "\tdef peek(self):\n";
    stream << "\t\treturn self.items[len(self.items)-1]\n";
    stream << "\n";

    stream << "\tdef size(self):\n";
    stream << "\t\treturn len(self.items)\n";
    stream << "\n";

    file.close();
}

void TestRailInterface::requestDataFromUser() {
    _testRailSelectorWindow.exec();

    if (_testRailSelectorWindow.getUserCancelled()) {
        return;
    }

    _url = _testRailSelectorWindow.getURL() + "/";
    _user = _testRailSelectorWindow.getUser();
    ////_password = _testRailSelectorWindow.getPassword();
    _password = "tutKA76";
    _project = QString::number(_testRailSelectorWindow.getProject());
}

bool TestRailInterface::isAValidTestDirectory(const QString& directory) {
    if (Test::isAValidDirectory(directory)) {
        // Ignore the utils and preformance directories
        if (directory.right(QString("utils").length()) == "utils" ||
            directory.right(QString("performance").length()) == "performance") {
            return false;
        }
        return true;
    }

    return false;
}

void TestRailInterface::processDirecoryPython(const QString& directory, QTextStream& stream) {
    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
        QString nextDirectory = it.next();

        // Only process directories
        if (!isAValidTestDirectory(nextDirectory)) {
            continue;
        }

        // The name of the section is the directory at the end of the path
        stream << "parent_id = parent_ids.peek()\n";
        QString name = nextDirectory.right(nextDirectory.length() - nextDirectory.lastIndexOf("/") - 1);
        stream << "data = { \'name\': \'" << name << "\', \'parent_id\': parent_id }\n";

        stream << "section = client.send_post(\'add_section/\' + str(" << _project << "), data)\n";

        // Now we push the parent_id, and recursively process each directory
        stream << "parent_ids.push(section[\'id\'])\n\n";
        processDirecoryPython(nextDirectory, stream);
    }

    // pop the parent directory before leaving
    stream << "parent_ids.pop()\n\n";
}

    // A suite of TestRail test cases contains trees.
//    The nodes of the trees are sections
//    The leaves are the test cases
//
// Each node and leaf have an ID and a parent ID.
// Therefore, the tree is built top-down, using a stack to store the IDs of each node
//
void TestRailInterface::createAddSectionsPythonScript(const QString& testDirectory, const QString& outputDirectory) {
    QFile file(outputDirectory + "/addSections.py");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create \'addSections.py\'");
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient(\'" << _url.toStdString().c_str() << "\')\n";
    stream << "client.user = \'" << _user << "\'\n";
    stream << "client.password = \'" << _password << "\'\n\n";

    stream << "from stack import *\n";
    stream << "parent_ids = Stack()\n\n";

    // top-level section
    stream << "data = { \'name\': \'"
           << "Test Suite - " << QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm") << "\'}\n";

    stream << "section = client.send_post(\'add_section/\' + str(" << _project << "), data)\n";

    // Now we push the parent_id, and recursively process each directory
    stream << "parent_ids.push(section[\'id\'])\n\n";
    processDirecoryPython(testDirectory, stream);

    file.close();
}

void TestRailInterface::createTestSuitePython(const QString& testDirectory,
                                              const QString& outputDirectory,
                                              const QString& user,
                                              const QString& branch) {
    
    createTestRailDotPyScript(outputDirectory);
    createStackDotPyScript(outputDirectory);
    requestDataFromUser();
    createAddSectionsPythonScript(testDirectory, outputDirectory);
 }

 void TestRailInterface::createTestSuiteXML(const QString& testDirectory,
                                            const QString& outputDirectory,
                                            const QString& user,
                                            const QString& branch) {

    QDomProcessingInstruction instruction = _document.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
    _document.appendChild(instruction);

    // We create a single section, within sections
    QDomElement root = _document.createElement("sections");
    _document.appendChild(root);

    QDomElement topLevelSection = _document.createElement("section");

    QDomElement suiteName = _document.createElement("name");
    suiteName.appendChild(_document.createTextNode("Test Suite - " + QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm")));
    topLevelSection.appendChild(suiteName);

    // This is the first call to 'process'.  This is then called recursively to build the full XML tree
    QDomElement secondLevelSections = _document.createElement("sections");
    topLevelSection.appendChild(processDirectoryXML(testDirectory, user, branch, secondLevelSections));

    topLevelSection.appendChild(secondLevelSections);
    root.appendChild(topLevelSection);

    // Write to file
    const QString testRailsFilename{ outputDirectory  + "/TestRailSuite.xml" };
    QFile file(testRailsFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__), "Could not create XML file");
        exit(-1);
    }

    QTextStream stream(&file);
    stream << _document.toString();

    file.close();

    QMessageBox::information(0, "Success", "TestRail XML file has been created");
}

QDomElement TestRailInterface::processDirectoryXML(const QString& directory, const QString& user, const QString& branch, const QDomElement& element) {
    QDomElement result = element;

    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
         QString nextDirectory = it.next();

       // The object name appears after the last slash (we are assured there is at least 1).
        QString objectName = nextDirectory.right(nextDirectory.length() - nextDirectory.lastIndexOf("/") - 1);

        // Only process directories
        if (isAValidTestDirectory(nextDirectory)) {
            // Create a section and process it
            QDomElement sectionElement = _document.createElement("section");

            QDomElement sectionElementName = _document.createElement("name");
            sectionElementName.appendChild(_document.createTextNode(objectName));
            sectionElement.appendChild(sectionElementName);

            QDomElement testsElement = _document.createElement("sections");
            sectionElement.appendChild(processDirectoryXML(nextDirectory, user, branch, testsElement));

            result.appendChild(sectionElement);
        } else if (objectName == "test.js" || objectName == "testStory.js") {
            QDomElement sectionElement = _document.createElement("section");
            QDomElement sectionElementName = _document.createElement("name");
            sectionElementName.appendChild(_document.createTextNode("all"));
            sectionElement.appendChild(sectionElementName);
            sectionElement.appendChild(processTest(nextDirectory, objectName, user, branch, _document.createElement("cases")));
            result.appendChild(sectionElement);
        }
    }

    return result;
}

QDomElement TestRailInterface::processTest(const QString& fullDirectory, const QString& test, const QString& user, const QString& branch, const QDomElement& element) {
    QDomElement result = element;
   
    QDomElement caseElement = _document.createElement("case");

    caseElement.appendChild(_document.createElement("id"));

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

    QDomElement titleElement = _document.createElement("title");
    titleElement.appendChild(_document.createTextNode(title));
    caseElement.appendChild(titleElement);

    QDomElement templateElement = _document.createElement("template");
    templateElement.appendChild(_document.createTextNode("Test Case (Steps)"));
    caseElement.appendChild(templateElement);

    QDomElement typeElement = _document.createElement("type");
    typeElement.appendChild(_document.createTextNode("3 - Regression"));
    caseElement.appendChild(typeElement);

    QDomElement priorityElement = _document.createElement("priority");
    priorityElement.appendChild(_document.createTextNode("Medium"));
    caseElement.appendChild(priorityElement);

    QDomElement estimateElementName = _document.createElement("estimate");
    estimateElementName.appendChild(_document.createTextNode("60"));
    caseElement.appendChild(estimateElementName);

    caseElement.appendChild(_document.createElement("references"));

    QDomElement customElement = _document.createElement("custom");

    QDomElement tester_countElement = _document.createElement("tester_count");
    tester_countElement.appendChild(_document.createTextNode("1"));
    customElement.appendChild(tester_countElement);

    QDomElement domain_bot_loadElement = _document.createElement("domain_bot_load");
    QDomElement domain_bot_loadElementId = _document.createElement("id");
    domain_bot_loadElementId.appendChild(_document.createTextNode("1"));
    domain_bot_loadElement.appendChild(domain_bot_loadElementId);
    QDomElement domain_bot_loadElementValue = _document.createElement("value");
    domain_bot_loadElementValue.appendChild(_document.createTextNode(" Without Bots (hifiqa-rc / hifi-qa-stable / hifiqa-master)"));
    domain_bot_loadElement.appendChild(domain_bot_loadElementValue);
    customElement.appendChild(domain_bot_loadElement);

    QDomElement automation_typeElement = _document.createElement("automation_type");
    QDomElement automation_typeElementId = _document.createElement("id");
    automation_typeElementId.appendChild(_document.createTextNode("0"));
    automation_typeElement.appendChild(automation_typeElementId);
    QDomElement automation_typeElementValue = _document.createElement("value");
    automation_typeElementValue.appendChild(_document.createTextNode("None"));
    automation_typeElement.appendChild(automation_typeElementValue);
    customElement.appendChild(automation_typeElement);

    QDomElement added_to_releaseElement = _document.createElement("added_to_release");
    QDomElement added_to_releaseElementId = _document.createElement("id");
    added_to_releaseElementId.appendChild(_document.createTextNode("4"));
    added_to_releaseElement.appendChild(added_to_releaseElementId);
    QDomElement added_to_releaseElementValue = _document.createElement("value");
    added_to_releaseElementValue.appendChild(_document.createTextNode(branch));
    added_to_releaseElement.appendChild(added_to_releaseElementValue);
    customElement.appendChild(added_to_releaseElement);

    QDomElement precondsElement = _document.createElement("preconds");
    precondsElement.appendChild(_document.createTextNode("Tester is in an empty region of a domain in which they have edit rights\n\n*Note: Press 'n' to advance test script"));
    customElement.appendChild(precondsElement);

    QString testMDName = QString("https://github.com/") + user + "/hifi_tests/blob/" + branch + "/tests/content/entity/light/point/create/test.md";

    QDomElement steps_seperatedElement = _document.createElement("steps_separated");
    QDomElement stepElement = _document.createElement("step");
    QDomElement stepIndexElement = _document.createElement("index");
    stepIndexElement.appendChild(_document.createTextNode("1"));
    stepElement.appendChild(stepIndexElement);
    QDomElement stepContentElement = _document.createElement("content");
    stepContentElement.appendChild(_document.createTextNode(QString("Execute instructions in [THIS TEST](") + testMDName + ")"));
    stepElement.appendChild(stepContentElement);
    QDomElement stepExpectedElement = _document.createElement("expected");
    stepExpectedElement.appendChild(_document.createTextNode("Refer to the expected result in the linked description."));
    stepElement.appendChild(stepExpectedElement);
    steps_seperatedElement.appendChild(stepElement);
    customElement.appendChild(steps_seperatedElement);

    QDomElement notesElement = _document.createElement("notes");
    notesElement.appendChild(_document.createTextNode(testMDName));
    customElement.appendChild(notesElement);

    caseElement.appendChild(customElement);

    result.appendChild(caseElement);
 
    return result;
}