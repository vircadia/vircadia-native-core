//
//  Snapshot.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 1/26/14.
//
//

#include "Snapshot.h"

#include <FileUtils.h>
#include <ui/snapshotShareDialog.cpp>

#include <QDateTime>
#include <QFileInfo>
#include <QDebug>

// filename format: hifi-snap-by-%username%-on-%date%_%time%_@-%location%.jpg
// %1 <= username, %2 <= date and time, %3 <= current location
const QString FILENAME_PATH_FORMAT = "hifi-snap-by-%1-on-%2@%3.jpg";

const QString DATETIME_FORMAT = "yyyy-MM-dd_hh-mm-ss";
const QString SNAPSHOTS_DIRECTORY = "Snapshots";

void Snapshot::saveSnapshot(QGLWidget* widget, QString username, glm::vec3 location) {
    QImage shot = widget->grabFrameBuffer();

    // add metadata
    shot.setText("location-x", QString::number(location.x));
    shot.setText("location-y", QString::number(location.y));
    shot.setText("location-z", QString::number(location.z));

    QString formattedLocation = QString("%1_%2_%3").arg(location.x).arg(location.y).arg(location.z);
    // replace decimal . with '-'
    formattedLocation.replace('.', '-');
    
    // normalize username, replace all non alphanumeric with '-'
    username.replace(QRegExp("[^A-Za-z0-9_]"), "-");
    
    QDateTime now = QDateTime::currentDateTime();
    
    QString fileName = FileUtils::standardPath(SNAPSHOTS_DIRECTORY);
    fileName.append(QString(FILENAME_PATH_FORMAT.arg(username, now.toString(DATETIME_FORMAT), formattedLocation)));
    shot.save(fileName, 0, 100);
    
    
}


