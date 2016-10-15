//
//  RunningMarker.h
//  interface/src
//
//  Created by David Rowe on 24 Aug 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RunningMarker_h
#define hifi_RunningMarker_h

#include <QObject>
#include <QString>

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
};

#endif // hifi_RunningMarker_h
