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
#include <QUrl>

#include <SettingHandle.h>
#include <DependencyManager.h>

class QFile;
class QTemporaryFile;

class SnapshotMetaData {
public:

    QUrl getURL() { return _URL; }
    void setURL(QUrl URL) { _URL = URL; }

private:
    QUrl _URL;
};

class Snapshot : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    Snapshot();
    static QString saveSnapshot(QImage image, const QString& filename, const QString& pathname = QString());
    static void save360Snapshot(const glm::vec3& cameraPosition, const bool& cubemapOutputFormat, const QString& filename);
    static QTemporaryFile* saveTempSnapshot(QImage image);
    static SnapshotMetaData* parseSnapshotData(QString snapshotPath);

    static Setting::Handle<QString> snapshotsLocation;
    static void uploadSnapshot(const QString& filename, const QUrl& href = QUrl(""));

signals:
    void snapshotLocationSet(const QString& value);

public slots:
    Q_INVOKABLE QString getSnapshotsLocation();
    Q_INVOKABLE void setSnapshotsLocation(const QString& location);

private slots:
    static void takeNextSnapshot();

private:
    static QFile* savedFileForSnapshot(QImage& image,
                                       bool isTemporary,
                                       const QString& userSelectedFilename = QString(),
                                       const QString& userSelectedPathname = QString());

    static QString snapshotFilename;
    static bool cubemapOutputFormat;
    static QTimer snapshotTimer;
    static qint16 snapshotIndex;
    static bool oldEnabled;
    static QVariant oldAttachedEntityId;
    static QVariant oldOrientation;
    static QVariant oldvFoV;
    static QVariant oldNearClipPlaneDistance;
    static QVariant oldFarClipPlaneDistance;
    static QImage imageArray[6];
    static void convertToCubemap();
    static void convertToEquirectangular();
};

#endif // hifi_Snapshot_h
