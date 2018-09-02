//
//  Downloader.h
//
//  Created by Nissim Hadar on 1 Sep 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_testRunner_h
#define hifi_testRunner_h

#include <QObject>
#include <QDirIterator>

class TestRunner : public QObject {
Q_OBJECT
public:
    explicit TestRunner(QObject *parent = 0);

    void run();

    void saveExistingHighFidelityAppDataFolder();
    void restoreHighFidelityAppDataFolder();

private:
    QDir appDataFolder;
    QDir savedAppDataFolder;
};

#endif // hifi_testRunner_h