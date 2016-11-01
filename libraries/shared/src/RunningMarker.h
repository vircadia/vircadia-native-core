//
//  RunningMarker.h
//  interface/src
//
//  Created by Brad Hefta-Gaub on 2016-10-15.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RunningMarker_h
#define hifi_RunningMarker_h

#include <QObject>
#include <QString>

class QThread;
class QTimer;

class RunningMarker {
public:
    RunningMarker(QObject* parent, QString name);
    ~RunningMarker();

    void startRunningMarker();

    QString getFilePath();
    static QString getMarkerFilePath(QString name);
protected:
    void writeRunningMarkerFiler();
    void deleteRunningMarkerFile();

    QObject* _parent { nullptr };
    QString _name;
    QThread* _runningMarkerThread { nullptr };
    QTimer* _runningMarkerTimer { nullptr };
};

#endif // hifi_RunningMarker_h
