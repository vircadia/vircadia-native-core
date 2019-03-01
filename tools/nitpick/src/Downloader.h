//
//  Downloader.h
//
//  Created by Nissim Hadar on 1 Mar 2018.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_downloader_h
#define hifi_downloader_h

#include "BusyWindow.h"

#include <QObject>
class Downloader : public QObject {
Q_OBJECT
public:
    Downloader();

    void downloadFiles(const QStringList& URLs, const QString& directoryName, const QStringList& filenames, void *caller);

private:
    QString _pythonCommand;
    BusyWindow _busyWindow;
};

#endif // hifi_downloader_h