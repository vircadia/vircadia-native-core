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

#include <QString>

class RunningMarker {
public:
    RunningMarker(QString name);
    ~RunningMarker();

    QString getFilePath() const;

    bool fileExists() const;

    void writeRunningMarkerFile();
    void deleteRunningMarkerFile();

private:
    QString _name;
};

#endif // hifi_RunningMarker_h
