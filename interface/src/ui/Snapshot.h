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
#include <QTimer>
#include <QtGui/QImage>

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


/**jsdoc
 * @namespace Snapshot
 * 
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

class Snapshot : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
public:
    Snapshot();
    QString saveSnapshot(QImage image, const QString& filename, const QString& pathname = QString());
    void save360Snapshot(const glm::vec3& cameraPosition,
                         const bool& cubemapOutputFormat,
                         const bool& notify,
                         const QString& filename);
    QTemporaryFile* saveTempSnapshot(QImage image);
    SnapshotMetaData* parseSnapshotData(QString snapshotPath);

    Setting::Handle<QString> _snapshotsLocation{ "snapshotsLocation" };
    void uploadSnapshot(const QString& filename, const QUrl& href = QUrl(""));

signals:

    /**jsdoc
     * @function Snapshot.snapshotLocationSet
     * @param {string} location
     * @returns {Signal}
     */
    void snapshotLocationSet(const QString& value);

public slots:

    /**jsdoc
     * @function Snapshot.getSnapshotsLocation
     * @returns {string}
     */
    Q_INVOKABLE QString getSnapshotsLocation();

    /**jsdoc
     * @function Snapshot.setSnapshotsLocation
     * @param {String} location
     */
    Q_INVOKABLE void setSnapshotsLocation(const QString& location);

private slots:
    void takeNextSnapshot();

private:
    QFile* savedFileForSnapshot(QImage& image,
                                       bool isTemporary,
                                       const QString& userSelectedFilename = QString(),
                                       const QString& userSelectedPathname = QString());
    QString _snapshotFilename;
    bool _notify360;
    bool _cubemapOutputFormat;
    QTimer _snapshotTimer;
    qint16 _snapshotIndex;
    bool _oldEnabled;
    QVariant _oldAttachedEntityId;
    QVariant _oldOrientation;
    QVariant _oldvFoV;
    QVariant _oldNearClipPlaneDistance;
    QVariant _oldFarClipPlaneDistance;
    QImage _imageArray[6];
    void convertToCubemap();
    void convertToEquirectangular();
};

#endif // hifi_Snapshot_h
