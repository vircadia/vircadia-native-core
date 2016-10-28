//
//  UserActivityLoggerScriptingInterface.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 6/06/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserActivityLoggerScriptingInterface_h
#define hifi_UserActivityLoggerScriptingInterface_h

#include <QObject>
#include <QJsonObject>

#include <DependencyManager.h>

class UserActivityLoggerScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    Q_INVOKABLE void enabledEdit();
    Q_INVOKABLE void openedMarketplace();
    Q_INVOKABLE void toggledAway(bool isAway);
    Q_INVOKABLE void tutorialProgress(QString stepName, int stepNumber, float secondsToComplete,
        float tutorialElapsedTime, QString tutorialRunID = "", int tutorialVersion = 0);

private:
    void logAction(QString action, QJsonObject details = {});
};

#endif // hifi_UserActivityLoggerScriptingInterface_h
