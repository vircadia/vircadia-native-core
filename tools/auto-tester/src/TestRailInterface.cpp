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

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include <QDateTime>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>

TestRailInterface::TestRailInterface() {
    _testRailTestCasesSelectorWindow.setURL("https://highfidelity.testrail.net");
    ////_testRailTestCasesSelectorWindow.setURL("https://nissimhadar.testrail.io");
    _testRailTestCasesSelectorWindow.setUser("@highfidelity.io");
    ////_testRailTestCasesSelectorWindow.setUser("nissim.hadar@gmail.com");

    _testRailTestCasesSelectorWindow.setProjectID(INTERFACE_PROJECT_ID);
    ////_testRailTestCasesSelectorWindow.setProjectID(2);

    _testRailTestCasesSelectorWindow.setSuiteID(INTERFACE_SUITE_ID);
    ////_testRailTestCasesSelectorWindow.setSuiteID(2);

    _testRailRunSelectorWindow.setURL("https://highfidelity.testrail.net");
    ////_testRailRunSelectorWindow.setURL("https://nissimhadar.testrail.io");
    _testRailRunSelectorWindow.setUser("@highfidelity.io");
    ////_testRailRunSelectorWindow.setUser("nissim.hadar@gmail.com");

    _testRailRunSelectorWindow.setProjectID(INTERFACE_PROJECT_ID);
    ////_testRailRunSelectorWindow.setProjectID(2);

    _testRailRunSelectorWindow.setSuiteID(INTERFACE_SUITE_ID);
    ////_testRailRunSelectorWindow.setSuiteID(2);

    _testRailResultsSelectorWindow.setURL("https://highfidelity.testrail.net");
    ////_testRailResultsSelectorWindow.setURL("https://nissimhadar.testrail.io");
    _testRailResultsSelectorWindow.setUser("@highfidelity.io");
    ////_testRailResultsSelectorWindow.setUser("nissim.hadar@gmail.com");

    _testRailResultsSelectorWindow.setProjectID(INTERFACE_PROJECT_ID);
    ////_testRailResultsSelectorWindow.setProjectID(2);

    _testRailResultsSelectorWindow.setSuiteID(INTERFACE_SUITE_ID);
    ////_testRailResultsSelectorWindow.setSuiteID(2);
}

QString TestRailInterface::getObject(const QString& path) {
    return path.right(path.length() - path.lastIndexOf("/") - 1);
}


bool TestRailInterface::setPythonCommand() {
    if (QProcessEnvironment::systemEnvironment().contains("PYTHON_PATH")) {
        QString _pythonPath = QProcessEnvironment::systemEnvironment().value("PYTHON_PATH");
        if (!QFile::exists(_pythonPath + "/" + pythonExe)) {
            QMessageBox::critical(0, pythonExe, QString("Python executable not found in ") + _pythonPath);
        }
        _pythonCommand = _pythonPath + "/" + pythonExe;
        return true;
    } else {
        QMessageBox::critical(0, "PYTHON_PATH not defined",
                              "Please set PYTHON_PATH to directory containing the Python executable");
        return false;
    }

    return false;
}

// Creates the testrail.py script
// This is the file linked to from http://docs.gurock.com/testrail-api2/bindings-python
void TestRailInterface::createTestRailDotPyScript() {
    QFile file(_outputDirectory + "/testrail.py");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'testrail.py'");
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
    stream << "\t\t\t\terror = '\"' + result['error'] + '\"'\n";
    stream << "\t\t\telse:\n";
    stream << "\t\t\t\terror = 'No additional error message received'\n";
    stream << "\t\t\traise APIError('TestRail API returned HTTP %s (%s)' % \n";
    stream << "\t\t\t\t(e.code, error))\n";
    stream << "\n";
    stream << "\t\treturn result\n";
    stream << "\n";
    stream << "class APIError(Exception):\n";
    stream << "\tpass\n";

    file.close();
}

// Creates a Stack class
void TestRailInterface::createStackDotPyScript() {
    QString filename = _outputDirectory + "/stack.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'stack.py'");
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

bool TestRailInterface::requestTestRailTestCasesDataFromUser() {
    // Make sure correct fields are enabled before calling
    _testRailTestCasesSelectorWindow.reset();
    _testRailTestCasesSelectorWindow.exec();

    if (_testRailTestCasesSelectorWindow.getUserCancelled()) {
        return false;
    }

    _url = _testRailTestCasesSelectorWindow.getURL() + "/";
    _user = _testRailTestCasesSelectorWindow.getUser();
    _password = _testRailTestCasesSelectorWindow.getPassword();
    ////_password = "tutKA76";////
    _projectID = QString::number(_testRailTestCasesSelectorWindow.getProjectID());
    _suiteID = QString::number(_testRailTestCasesSelectorWindow.getSuiteID());

    return true;
}

bool TestRailInterface::requestTestRailRunDataFromUser() {
    _testRailRunSelectorWindow.reset();
    _testRailRunSelectorWindow.exec();

    if (_testRailRunSelectorWindow.getUserCancelled()) {
        return false;
    }

    _url = _testRailRunSelectorWindow.getURL() + "/";
    _user = _testRailRunSelectorWindow.getUser();
    _password = _testRailRunSelectorWindow.getPassword();
    ////_password = "tutKA76";////
    _projectID = QString::number(_testRailRunSelectorWindow.getProjectID());
    _suiteID = QString::number(_testRailRunSelectorWindow.getSuiteID());

    return true;
}

bool TestRailInterface::requestTestRailResultsDataFromUser() {
    _testRailResultsSelectorWindow.reset();
    _testRailResultsSelectorWindow.exec();

    if (_testRailResultsSelectorWindow.getUserCancelled()) {
        return false;
    }

    _url = _testRailResultsSelectorWindow.getURL() + "/";
    _user = _testRailResultsSelectorWindow.getUser();
    _password = _testRailResultsSelectorWindow.getPassword();
    ////_password = "tutKA76";////
    _projectID = QString::number(_testRailResultsSelectorWindow.getProjectID());
    _suiteID = QString::number(_testRailResultsSelectorWindow.getSuiteID());

    return true;
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

void TestRailInterface::processDirectoryPython(const QString& directory,
                                               QTextStream& stream,
                                               const QString& userGitHub,
                                               const QString& branchGitHub) {
    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
        QString nextDirectory = it.next();

        QString objectName = getObject(nextDirectory);

        if (isAValidTestDirectory(nextDirectory)) {
            // The name of the section is the directory at the end of the path
            stream << "parent_id = parent_ids.peek()\n";
            stream << "data = { 'name': '" << objectName << "', 'suite_id': " + _suiteID + ", 'parent_id': parent_id }\n";

            stream << "section = client.send_post('add_section/' + str(" << _projectID << "), data)\n";

            // Now we push the parent_id, and recursively process each directory
            stream << "parent_ids.push(section['id'])\n\n";
            processDirectoryPython(nextDirectory, stream, userGitHub, branchGitHub);
        } else if (objectName == "test.js") {
            processTestPython(nextDirectory, stream, userGitHub, branchGitHub);
        }
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
void TestRailInterface::createAddTestCasesPythonScript(const QString& testDirectory,
                                                       const QString& userGitHub,
                                                       const QString& branchGitHub) {
    QString filename = _outputDirectory + "/addTestCases.py";
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

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    stream << "from stack import *\n";
    stream << "parent_ids = Stack()\n\n";

    // top-level section
    stream << "data = { 'name': '"
           << "Test Section - " << QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm") + "', "
           << "'suite_id': " + _suiteID + "}\n";

    stream << "section = client.send_post('add_section/' + str(" << _projectID << "), data)\n";

    // Now we push the parent_id, and recursively process each directory
    stream << "parent_ids.push(section['id'])\n\n";
    processDirectoryPython(testDirectory, stream, userGitHub, branchGitHub);

    file.close();

    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Python script has been created",
                                        "Do you want to run the script and update TestRail?",
                                        QMessageBox::Yes | QMessageBox::No).exec()
    ) {
        QProcess* process = new QProcess();
        connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });

        connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
                [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

        QStringList parameters = QStringList() << _outputDirectory + "/addTestCases.py";
        process->start(_pythonCommand, parameters);
    }
}

void TestRailInterface::updateReleasesComboData(int exitCode, QProcess::ExitStatus exitStatus) {
    // Quit if user has previously cancelled
    if (_testRailTestCasesSelectorWindow.getUserCancelled()) {
        return;
    }

    // Check if process completed successfully
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not get 'added to release' data from TestRail");
        exit(-1);
    }

    // Create map of releases from the file created by the process
    _releaseNames.clear();

    QString filename = _outputDirectory + "/releases.txt";
    if (!QFile::exists(filename)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not find releases.txt in " + _outputDirectory);
        exit(-1);
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open " + _outputDirectory + "/releases.txt");
        exit(-1);
    }

    QTextStream in(&file);
    QString line = in.readLine();
    while (!line.isNull()) {
        _releaseNames << line;
        line = in.readLine();
    }

    file.close();

    // Update the combo
    _testRailTestCasesSelectorWindow.updateReleasesComboBoxData(_releaseNames);

    _testRailTestCasesSelectorWindow.exec();

    if (_testRailTestCasesSelectorWindow.getUserCancelled()) {
        return;
    }

    createAddTestCasesPythonScript(_testDirectory, _userGitHub, _branchGitHub);
}

void TestRailInterface::addRun() {
    QString filename = _outputDirectory + "/addRun.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create " + filename);
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    // A test suite is a forest.  Each node is a section and leaves are either sections or test cases
    // The user has selected a root for the run
    // To find the cases in this tree we need all the sections in the tree
    // These are found by building a set of all relevant sections.  The first section is the selected root
    // As the sections are in an ordered array we use the following snippet to find the relevant sections:
    //      initialize section set with the root
    //      for each section in the ordered array of sections in the suite
    //          if the parent of the section is in the section set then
    //              add this section to the section set
    //
    stream << "sections = client.send_get('get_sections/" + _projectID + "&suite_id=" + _suiteID + "')\n\n";

    int sectionID = _sectionIDs[_testRailRunSelectorWindow.getSectionID()];

    stream << "relevantSections = { " + QString::number(sectionID) + " }\n";
    stream << "for section in sections:\n";
    stream << "\tif section['parent_id'] in relevantSections:\n";
    stream << "\t\trelevantSections.add(section['id'])\n\n";

    // We now loop over each section in the set and collect the cases into an array
    stream << "cases = []\n";
    stream << "for section_id in relevantSections:\n";
    stream << "\tcases = cases + client.send_get('get_cases/" + _projectID + "&suite_id=" + _suiteID + "&section_id=' + str(section_id))\n\n";

    // To create a run we need an array of the relevant case ids
    stream << "case_ids = []\n";
    stream << "for case in cases:\n";
    stream << "\tcase_ids.append(case['id'])\n\n";

    // Now, we can create the run
    stream << "data = { 'name': '" + _sectionNames[_testRailRunSelectorWindow.getSectionID()].replace("Section", "Run") +
                  "', 'suite_id': " + _suiteID + 
                  ", 'include_all': False, 'case_ids': case_ids}\n";

    stream << "run = client.send_post('add_run/" + _projectID + "', data)\n";

    file.close();

    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Python script has been created",
                                        "Do you want to run the script and update TestRail?",
                                        QMessageBox::Yes | QMessageBox::No).exec()
    ) {
        QProcess* process = new QProcess();
        connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });

        connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
                [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

        QStringList parameters = QStringList() << _outputDirectory + "/addRun.py";
        process->start(_pythonCommand, parameters);
    }
}
void TestRailInterface::updateRunWithResults() {
    QString filename = _outputDirectory + "/updateRunWithResults.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create " + filename);
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    // It is assumed that all the tests that haven't failed have passed
    // The failed tests are read, formatted and inserted into a set
    //      A failure named 'Failure_1--tests.content.entity.material.apply.avatars.00000' is formatted to 'content/entity/material/apply/avatars'
    //      This is the name of the test in TestRail
    stream << "from os import listdir\n";

    stream << "failed_tests = set()\n";

    stream << "for entry in listdir('" + _outputDirectory + "/" + tempName + "'):\n";
    stream << "\tparts = entry.split('--tests.')[1].split('.')\n";
    stream << "\tfailed_test = parts[0]\n";
    stream << "\tfor i in range(1, len(parts) - 1):\n";
    stream << "\t\tfailed_test = failed_test + '/' + parts[i]\n";

    stream << "\tfailed_tests.add(failed_test)\n\n";

    // Initialize the array of results that will be eventually used to update TestRail
    stream << "status_ids = []\n";
    stream << "case_ids = []\n";

    int runID = _runIDs[_testRailResultsSelectorWindow.getRunID()];
    stream << "tests = client.send_get('get_tests/" + QString::number(runID) + "')\n\n";
    stream << "for test in tests:\n";

    // status_id's: 1 - Passed
    //              2 - Blocked
    //              3 - Untested
    //              4 - Retest
    //              5 - Failed
    stream << "\tstatus_id = 1\n";
    stream << "\tif test['title'] in failed_tests:\n";
    stream << "\t\tstatus_id = 5\n";
    stream << "\tcase_ids.append(test['case_id'])\n";
    stream << "\tstatus_ids.append(status_id)\n\n";

    // We can now update the test (note that all tests need to be updated)
    // An example request is as follows:
    //
    //      "results" : [
    //          { 'case_id': 1, 'status_id': 5 },
    //          { 'case_id': 2, 'status_id': 1 }
    //      ]
    //
    stream << "results = []\n";
    stream << "for i in range(len(case_ids)):\n";
    stream << "\tresults.append({'case_id': case_ids[i], 'status_id': status_ids[i] })\n\n";

    stream << "data = { 'results': results }\n";
    stream << "section = client.send_post('add_results_for_cases/' + str(" << runID << "), data)\n";

    file.close();

    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Python script has been created",
                                        "Do you want to run the script and update TestRail?",
                                        QMessageBox::Yes | QMessageBox::No).exec()
    ) {
        QProcess* process = new QProcess();
        connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });

        connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
                [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

        QStringList parameters = QStringList() << _outputDirectory + "/updateRunWithResults.py";
        process->start(_pythonCommand, parameters);
    }
}

void TestRailInterface::updateSectionsComboData(int exitCode, QProcess::ExitStatus exitStatus) {
    // Quit if user has previously cancelled
    if (_testRailRunSelectorWindow.getUserCancelled()) {
        return;
    }

    // Check if process completed successfully
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not get sections from TestRail");
        exit(-1);
    }

    // Create map of sections from the file created by the process
    _sectionNames.clear();

    QString filename = _outputDirectory + "/sections.txt";
    if (!QFile::exists(filename)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not find sections.txt in " + _outputDirectory);
        exit(-1);
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open " + filename);
        exit(-1);
    }

    QTextStream in(&file);
    QString line = in.readLine();
    while (!line.isNull()) {
        // The section name is all the words except for the last
        // The id is the last word
        QString section = line.left(line.lastIndexOf(" "));
        QString id = line.right(line.length() - line.lastIndexOf(" ") - 1);

        _sectionIDs.push_back(id.toInt());
        _sectionNames << section;

        line = in.readLine();
    }

    file.close();

    // Update the combo
    _testRailRunSelectorWindow.updateSectionsComboBoxData(_sectionNames);

    _testRailRunSelectorWindow.exec();

    if (_testRailRunSelectorWindow.getUserCancelled()) {
        return;
    }

    // The test cases are now read from TestRail
    // When this is complete, the Run can be created
    addRun();
}

void TestRailInterface::updateRunsComboData(int exitCode, QProcess::ExitStatus exitStatus) {
    // Quit if user has previously cancelled
    if (_testRailRunSelectorWindow.getUserCancelled()) {
        return;
    }

    // Check if process completed successfully
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not get runs from TestRail");
        exit(-1);
    }

    // Create map of sections from the file created by the process
    _runNames.clear();

    QString filename = _outputDirectory + "/runs.txt";
    if (!QFile::exists(filename)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not find runs.txt in " + _outputDirectory);
        exit(-1);
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not open " + filename);
        exit(-1);
    }

    QTextStream in(&file);
    QString line = in.readLine();
    while (!line.isNull()) {
        // The run name is all the words except for the last
        // The id is the last word
        QString section = line.left(line.lastIndexOf(" "));
        QString id = line.right(line.length() - line.lastIndexOf(" ") - 1);

        _runIDs.push_back(id.toInt());
        _runNames << section;

        line = in.readLine();
    }

    file.close();

    // Update the combo
    _testRailResultsSelectorWindow.updateRunsComboBoxData(_runNames);

    _testRailResultsSelectorWindow.exec();

    if (_testRailResultsSelectorWindow.getUserCancelled()) {
        return;
    }

    // The test cases are now read from TestRail
    // When this is complete, the Run can be updated
    updateRunWithResults();
}

void TestRailInterface::getReleasesFromTestRail() {
    QString filename = _outputDirectory + "/getReleases.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'getReleases.py'");
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    // Print the list of releases
    stream << "case_fields = client.send_get('get_case_fields/" + _projectID + "')\n";
    stream << "for case_field in case_fields:\n";
    stream << "\tif case_field['name'] == 'added_to_release':\n";
    stream << "\t\trelease_string = case_field['configs'][0]['options']['items']\n\n";

    // The list read from TestRail looks like this:
    //      '0,< RC65\n1,RC65\n2,RC66\n3,RC67\n4,RC68\n5,RC69\n6,v0.70.0\n7,v0.71.0\n8,v0.72.0\n9,v0.73.0\n10,v0.74.0\n11,v0.75.0\n12,v0.76.0\n13,v0.77.0\n14,v0.78.0\n15,v0.79.0'
    // Splitting on newline gives an array:
    //      ['0,< RC65', '1,RC65', '2,RC66', '3,RC67', '4,RC68', '5,RC69', '6,v0.70.0', '7,v0.71.0', '8,v0.72.0', '9,v0.73.0', '10,v0.74.0', '11,v0.75.0', '12,v0.76.0', '13,v0.77.0', '14,v0.78.0', '15,v0.79.0']
    // Each element consists of an index and a string, separated by a comma.
    // We just need the strings
    stream << "file = open('" + _outputDirectory + "/releases.txt', 'w')\n\n";
    stream << "releases = release_string.split('\\n')\n";
    stream << "for release in releases:\n";
    stream << "\twords = release.split(',')\n";
    stream << "\tfile.write(words[1] + '\\n')\n\n";

    stream << "file.close()\n";

    file.close();

    QProcess* process = new QProcess();
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [=](int exitCode, QProcess::ExitStatus exitStatus) { updateReleasesComboData(exitCode, exitStatus); });

    QStringList parameters = QStringList() << filename;
    process->start(_pythonCommand, parameters);
}

void TestRailInterface::createTestSuitePython(const QString& testDirectory,
                                              const QString& outputDirectory,
                                              const QString& userGitHub,
                                              const QString& branchGitHub) {
    _testDirectory = testDirectory;
    _outputDirectory = outputDirectory;
    _userGitHub = userGitHub;
    _branchGitHub = branchGitHub;

    if (!setPythonCommand()) {
        return;
    }

    if (!requestTestRailTestCasesDataFromUser()) {
        return;
    }

    createTestRailDotPyScript();
    createStackDotPyScript();

    // TestRail will be updated after the process initiated by getReleasesFromTestRail has completed
    getReleasesFromTestRail();
}

void TestRailInterface::createTestSuiteXML(const QString& testDirectory,
                                           const QString& outputDirectory,
                                           const QString& userGitHub,
                                           const QString& branchGitHub) {
    _outputDirectory = outputDirectory;

    QDomProcessingInstruction instruction = _document.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");
    _document.appendChild(instruction);

    // We create a single section, within sections
    QDomElement root = _document.createElement("sections");
    _document.appendChild(root);

    QDomElement topLevelSection = _document.createElement("section");

    QDomElement suiteName = _document.createElement("name");
    suiteName.appendChild(
        _document.createTextNode("Test Suite - " + QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm")));
    topLevelSection.appendChild(suiteName);

    // This is the first call to 'process'.  This is then called recursively to build the full XML tree
    QDomElement secondLevelSections = _document.createElement("sections");
    topLevelSection.appendChild(processDirectoryXML(testDirectory, userGitHub, branchGitHub, secondLevelSections));

    topLevelSection.appendChild(secondLevelSections);
    root.appendChild(topLevelSection);

    // Write to file
    const QString testRailsFilename{ _outputDirectory + "/TestRailSuite.xml" };
    QFile file(testRailsFilename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create XML file");
        exit(-1);
    }

    QTextStream stream(&file);
    stream << _document.toString();

    file.close();

    QMessageBox::information(0, "Success", "TestRail XML file has been created");
}

QDomElement TestRailInterface::processDirectoryXML(const QString& directory,
                                                   const QString& userGitHub,
                                                   const QString& branchGitHub,
                                                   const QDomElement& element) {
    QDomElement result = element;

    // Loop over all entries in directory
    QDirIterator it(directory.toStdString().c_str());
    while (it.hasNext()) {
        QString nextDirectory = it.next();

        // The object name appears after the last slash (we are assured there is at least 1).
        QString objectName = getObject(nextDirectory);

        // Only process directories
        if (isAValidTestDirectory(nextDirectory)) {
            // Create a section and process it
            QDomElement sectionElement = _document.createElement("section");

            QDomElement sectionElementName = _document.createElement("name");
            sectionElementName.appendChild(_document.createTextNode(objectName));
            sectionElement.appendChild(sectionElementName);

            QDomElement testsElement = _document.createElement("sections");
            sectionElement.appendChild(processDirectoryXML(nextDirectory, userGitHub, branchGitHub, testsElement));

            result.appendChild(sectionElement);
        } else if (objectName == "test.js" || objectName == "testStory.js") {
            QDomElement sectionElement = _document.createElement("section");
            QDomElement sectionElementName = _document.createElement("name");
            sectionElementName.appendChild(_document.createTextNode("all"));
            sectionElement.appendChild(sectionElementName);
            sectionElement.appendChild(
                processTestXML(nextDirectory, objectName, userGitHub, branchGitHub, _document.createElement("cases")));

            result.appendChild(sectionElement);
        }
    }

    return result;
}

QDomElement TestRailInterface::processTestXML(const QString& fullDirectory,
                                              const QString& test,
                                              const QString& userGitHub,
                                              const QString& branchGitHub,
                                              const QDomElement& element) {
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
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                                  "Folder \"tests\" not found  in " + fullDirectory);
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
    domain_bot_loadElementValue.appendChild(
        _document.createTextNode(" Without Bots (hifiqa-rc / hifi-qa-stable / hifiqa-master)"));
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
    added_to_releaseElementValue.appendChild(_document.createTextNode(branchGitHub));
    added_to_releaseElement.appendChild(added_to_releaseElementValue);
    customElement.appendChild(added_to_releaseElement);

    QDomElement precondsElement = _document.createElement("preconds");
    precondsElement.appendChild(_document.createTextNode(
        "Tester is in an empty region of a domain in which they have edit rights\n\n*Note: Press 'n' to advance test script"));
    customElement.appendChild(precondsElement);

    QString testMDName = QString("https://github.com/") + userGitHub + "/hifi_tests/blob/" + branchGitHub +
                         "/tests/content/entity/light/point/create/test.md";

    QDomElement steps_seperatedElement = _document.createElement("steps_separated");
    QDomElement stepElement = _document.createElement("step");
    QDomElement stepIndexElement = _document.createElement("index");
    stepIndexElement.appendChild(_document.createTextNode("1"));
    stepElement.appendChild(stepIndexElement);
    QDomElement stepContentElement = _document.createElement("content");
    stepContentElement.appendChild(
        _document.createTextNode(QString("Execute instructions in [THIS TEST](") + testMDName + ")"));
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

void TestRailInterface::processTestPython(const QString& fullDirectory,
                                          QTextStream& stream,
                                          const QString& userGitHub,
                                          const QString& branchGitHub) {
    // The name of the test is derived from the full path.
    // The first term is the first word after "tests"
    // The last word is the penultimate word
    QStringList words = fullDirectory.split('/');
    int i = 0;
    while (words[i] != "tests") {
        ++i;
        if (i >= words.length() - 1) {
            QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                                  "Folder \"tests\" not found  in " + fullDirectory);
            exit(-1);
        }
    }

    ++i;
    QString title{ words[i] };
    for (++i; i < words.length() - 1; ++i) {
        title += " / " + words[i];
    }

    // To create the path to test.md, prefix by tests, and remove blanks
    QString pathToTestMD = QString("/tests/") + title.remove(" ");

    stream << "section_id = parent_ids.peek()\n";

    QString testMDName =
        QString("https://github.com/") + userGitHub + "/hifi_tests/blob/" + branchGitHub + pathToTestMD + "/test.md ";

    QString testContent = QString("Execute instructions in [THIS TEST](") + testMDName + ")";
    QString testExpected = QString("Refer to the expected result in the linked description.");


    stream << "data = {\n"
           << "\t'title': '" << title << "',\n"
           << "\t'template_id': 2,\n"
           << "\t'custom_added_to_release': " << _testRailTestCasesSelectorWindow.getReleaseID() << ",\n"
           << "\t'custom_tester_count': 1,\n"
           << "\t'custom_domain_bot_load': 1,\n"
           << "\t'custom_preconds': "
           << "'Tester is in an empty region of a domain in which they have edit rights\\n\\n*Note: Press \\'n\\' to advance "
              "test script',\n"
           << "\t'custom_steps_separated': "
           << "[\n\t\t{\n\t\t\t'content': '" << testContent << "',\n\t\t\t'expected': '" << testExpected << "'\n\t\t}\n\t]\n"
           << "}\n";

    stream << "case = client.send_post('add_case/' + str(section_id), data)\n";
}

void TestRailInterface::getTestSectionsFromTestRail() {
    QString filename = _outputDirectory + "/getSections.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create 'getSections.py'");
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    // Print the list of sections without parents
    stream << "sections = client.send_get('get_sections/" + _projectID + "&suite_id=" + _suiteID + "')\n\n";
    stream << "file = open('" + _outputDirectory + "/sections.txt', 'w')\n\n";
    stream << "for section in sections:\n";
    stream << "\tif section['parent_id'] == None:\n";
    stream << "\t\tfile.write(section['name'] + ' ' + str(section['id']) + '\\n')\n\n";
    stream << "file.close()\n";

    file.close();

    QProcess* process = new QProcess();
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [=](int exitCode, QProcess::ExitStatus exitStatus) { updateSectionsComboData(exitCode, exitStatus); });

    QStringList parameters = QStringList() << filename;
    process->start(_pythonCommand, parameters);
}

void TestRailInterface::getRunsFromTestRail() {
    QString filename = _outputDirectory + "/getRuns.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
                              "Could not create " + filename);
        exit(-1);
    }

    QTextStream stream(&file);

    // Code to access TestRail
    stream << "from testrail import *\n";
    stream << "client = APIClient('" << _url.toStdString().c_str() << "')\n";
    stream << "client.user = '" << _user << "'\n";
    stream << "client.password = '" << _password << "'\n\n";

    // Print the list of runs
    stream << "runs = client.send_get('get_runs/" + _projectID + "')\n\n";
    stream << "file = open('" + _outputDirectory + "/runs.txt', 'w')\n\n";
    stream << "for run in runs:\n";
    stream << "\tfile.write(run['name'] + ' ' + str(run['id']) + '\\n')\n\n";
    stream << "file.close()\n";

    file.close();

    QProcess* process = new QProcess();
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [=](int exitCode, QProcess::ExitStatus exitStatus) { updateRunsComboData(exitCode, exitStatus); });

    QStringList parameters = QStringList() << filename;

    process->start(_pythonCommand, parameters);
}

void TestRailInterface::createTestRailRun(const QString& outputDirectory) {
    _outputDirectory = outputDirectory;

    if (!setPythonCommand()) {
        return;
    }

    if (!requestTestRailRunDataFromUser()) {
        return;
    }

    createTestRailDotPyScript();
    createStackDotPyScript();

    // TestRail will be updated after the process initiated by getTestCasesFromTestRail has completed
    getTestSectionsFromTestRail();
}

void TestRailInterface::updateTestRailRunResults(const QString& testResults, const QString& tempDirectory) {
    _outputDirectory = tempDirectory;

    if (!setPythonCommand()) {
        return;
    }

    if (!requestTestRailResultsDataFromUser()) {
        return;
    }

    // This is needed to access TestRail
    createTestRailDotPyScript();

    // Extract test failures from zipped folder
    QString tempSubDirectory = tempDirectory + "/" + tempName;
    QDir dir = tempSubDirectory;
    dir.mkdir(tempSubDirectory);
    JlCompress::extractDir(testResults, tempSubDirectory);

    // TestRail will be updated after the process initiated by getTestRunFromTestRail has completed
    getRunsFromTestRail();
}