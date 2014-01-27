//
//  Snapshot.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 1/26/14.
//
//

#include "Snapshot.h"

#include <FileUtils.h>

#include <QDateTime>
#include <QStandardPaths>

// filename format: hifi-snap_username_current-coordinates_date_time-with-seconds.jpg
// %1 <= username, %2 <= current location, %3 <= date and time
const QString FILENAME_PATH_FORMAT = "hifi-snap_%1_%2_%3.jpg";
const QString DATETIME_FORMAT = "yyyy-MM-dd_hh.mm.ss";
const QString SNAPSHOTS_DIRECTORY = "Snapshots";

void Snapshot::saveSnapshot(QGLWidget* widget, QString username, glm::vec3 location) {
    QImage shot = widget->grabFrameBuffer();

    // add metadata
    shot.setText("location-x", QString::number(location.x));
    shot.setText("location-y", QString::number(location.y));
    shot.setText("location-z", QString::number(location.z));

    QString formattedLocation = QString("%1-%2-%3").arg(location.x).arg(location.y).arg(location.z);
    QDateTime now = QDateTime::currentDateTime();
    
    QString fileName = FileUtils::standardPath(SNAPSHOTS_DIRECTORY);
    fileName.append(QString(FILENAME_PATH_FORMAT.arg(username, formattedLocation, now.toString(DATETIME_FORMAT))));
    shot.save(fileName);
}


