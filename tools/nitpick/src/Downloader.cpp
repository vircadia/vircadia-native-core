//
//  Downloader.cpp
//
//  Created by Nissim Hadar on 1 Mar 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Downloader.h"
#include "PythonInterface.h"

#include <QFile>
#include <QMessageBox>
#include <QProcess>
#include <QThread>
#include <QTextStream>

Downloader::Downloader() {
    PythonInterface* pythonInterface = new PythonInterface();
    _pythonCommand = pythonInterface->getPythonCommand();
}

void Downloader::downloadFiles(const QStringList& URLs, const QString& directoryName, const QStringList& filenames, void *caller) {
    if (URLs.size() <= 0) {
        return;
    }

    QString filename = directoryName + "/downloadFiles.py";
    if (QFile::exists(filename)) {
        QFile::remove(filename);
    }
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(0, "Internal error: " + QString(__FILE__) + ":" + QString::number(__LINE__),
            "Could not create 'downloadFiles.py'");
        exit(-1);
    }

    QTextStream stream(&file);

    stream << "import requests\n";

    for (int i = 0; i < URLs.size(); ++i) {
        stream << "\nurl = '" + URLs[i] + "'\n";
        stream << "r = requests.get(url)\n";
        stream << "open('" + directoryName + '/' + filenames [i] + "', 'wb').write(r.content)\n";
    }

    file.close();

#ifdef Q_OS_WIN
    QProcess* process = new QProcess();
    _busyWindow.setWindowTitle("Downloading Files");
    connect(process, &QProcess::started, this, [=]() { _busyWindow.exec(); });
    connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [=](int exitCode, QProcess::ExitStatus exitStatus) { _busyWindow.hide(); });

    QStringList parameters = QStringList() << filename;
    process->start(_pythonCommand, parameters);
#elif defined Q_OS_MAC
    QProcess* process = new QProcess();
    QStringList parameters = QStringList() << "-c" << _pythonCommand + " " + filename;
    process->start("sh", parameters);
    
    // Wait for the last file to download
    while (!QFile::exists(directoryName + '/' + filenames[filenames.length() - 1])) {
        QThread::msleep(200);
    }
#endif
}
