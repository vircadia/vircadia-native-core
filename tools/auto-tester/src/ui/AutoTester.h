//
//  AutoTester.h
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_AutoTester_h
#define hifi_AutoTester_h

#include <QtWidgets/QMainWindow>
#include <QSignalMapper>
#include <QTextEdit>
#include "ui_AutoTester.h"

#include "../Downloader.h"
#include "../Test.h"

#include "HelpWindow.h"

class AutoTester : public QMainWindow {
    Q_OBJECT

public:
    AutoTester(QWidget *parent = Q_NULLPTR);

    void setup();

    void runFromCommandLine(const QString& testFolder, const QString& branch, const QString& user);

    void downloadImage(const QUrl& url);
    void downloadImages(const QStringList& URLs, const QString& directoryName, const QStringList& filenames);

    void setUserText(const QString& user);
    QString getSelectedUser();

    void setBranchText(const QString& branch);
    QString getSelectedBranch();

private slots:
    void on_tabWidget_currentChanged(int index);

    void on_evaluateTestsButton_clicked();
    void on_createRecursiveScriptButton_clicked();
    void on_createAllRecursiveScriptsButton_clicked();
	void on_createTestsButton_clicked();

    void on_createMDFileButton_clicked();
    void on_createAllMDFilesButton_clicked();

    void on_createTestsOutlineButton_clicked();

    void on_createTestRailTestCasesButton_clicked();
    void on_createTestRailRunButton_clicked();
    void on_updateTestRailRunResultsButton_clicked();

    void on_hideTaskbarButton_clicked();
    void on_showTaskbarButton_clicked();

    void on_createPythonScriptRadioButton_clicked();
    void on_createXMLScriptRadioButton_clicked();

    void on_closeButton_clicked();

    void saveImage(int index);

    void about();
    void content();

private:
    Ui::AutoTesterClass _ui;
    Test* _test;

    std::vector<Downloader*> _downloaders;

    // local storage for parameters - folder to store downloaded files in, and a list of their names
    QString _directoryName;
    QStringList _filenames;

    // Used to enable passing a parameter to slots
    QSignalMapper* _signalMapper;

    int _numberOfImagesToDownload { 0 };
    int _numberOfImagesDownloaded { 0 };
    int _index { 0 };

    bool _isRunningFromCommandline { false };

    HelpWindow helpWindow;
};

#endif // hifi_AutoTester_h