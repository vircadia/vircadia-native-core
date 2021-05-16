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


/*@jsdoc
 * The <code>Snapshot</code> API provides access to the path that snapshots are saved to. This path is that provided in 
 * Settings &gt; General &gt; Snapshots. Snapshots may be taken using <code>Window</code> API functions such as 
 * {@link Window.takeSnapshot}.
 *
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

    /*@jsdoc
     * Triggered when the path that snapshots are saved to is changed.
     * @function Snapshot.snapshotLocationSet
     * @param {string} location - The new snapshots location.
     * @returns {Signal}
     * @example <caption>Report when the snapshots location is changed.</caption>
     * // Run this script then change the snapshots location in Settings > General > Snapshots.
     * Snapshot.snapshotLocationSet.connect(function (path) {
     *     print("New snapshot location: " + path);
     * });
     */
    void snapshotLocationSet(const QString& value);

public slots:

    /*@jsdoc
     * Gets the path that snapshots are saved to.
     * @function Snapshot.getSnapshotsLocation
     * @returns {string} The path to save snapshots to.
     */
    Q_INVOKABLE QString getSnapshotsLocation();

    /*@jsdoc
     * Sets the path that snapshots are saved to.
     * @function Snapshot.setSnapshotsLocation
     * @param {String} location - The path to save snapshots to.
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
    bool _waitingOnSnapshot { false };
    bool _taking360Snapshot { false };
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
