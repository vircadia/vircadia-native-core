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

#include "UserActivityLoggerScriptingInterface.h"
#include "UserActivityLogger.h"

void UserActivityLoggerScriptingInterface::enabledEdit() {
    logAction("enabled_edit");
}

void UserActivityLoggerScriptingInterface::openedMarketplace() {
    logAction("opened_marketplace");
}

void UserActivityLoggerScriptingInterface::toggledAway(bool isAway) {
    logAction("toggled_away", { { "is_away", isAway } });
}

void UserActivityLoggerScriptingInterface::tutorialProgress( QString stepName, int stepNumber, float secondsToComplete,
        float tutorialElapsedTime, QString tutorialRunID, int tutorialVersion, QString controllerType = "") {
    logAction("tutorial_progress", {
        { "tutorial_run_id", tutorialRunID },
        { "tutorial_version", tutorialVersion },
        { "step", stepName },
        { "step_number", stepNumber },
        { "seconds_to_complete", secondsToComplete },
        { "tutorial_elapsed_seconds", tutorialElapsedTime },
        { "controller_type", controllerType }
    });

}

void UserActivityLoggerScriptingInterface::logAction(QString action, QJsonObject details) {
    QMetaObject::invokeMethod(&UserActivityLogger::getInstance(), "logAction",
                              Q_ARG(QString, action),
                              Q_ARG(QJsonObject, details));
}
