//
//  Snapshot.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 1/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Snapshot_h
#define hifi_Snapshot_h

#include <glm/glm.hpp>

#include <QString>
#include <QStandardPaths>

#include <SettingHandle.h>

class QFile;
class QTemporaryFile;

class SnapshotMetaData {
public:

    QUrl getURL() { return _URL; }
    void setURL(QUrl URL) { _URL = URL; }

private:
    QUrl _URL;
};

class Snapshot {
public:
    static QString saveSnapshot(QImage image);
    static QTemporaryFile* saveTempSnapshot(QImage image);
    static SnapshotMetaData* parseSnapshotData(QString snapshotPath);

    static Setting::Handle<QString> snapshotsLocation;
    static Setting::Handle<bool> hasSetSnapshotsLocation;
private:
    static QFile* savedFileForSnapshot(QImage & image, bool isTemporary);
};

#endif // hifi_Snapshot_h
