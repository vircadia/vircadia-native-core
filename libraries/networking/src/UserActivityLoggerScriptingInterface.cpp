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
    doLogAction("enabled_edit");
}

void UserActivityLoggerScriptingInterface::openedTablet(bool visibleToOthers) {
    doLogAction("opened_tablet", { { "visible_to_others", visibleToOthers } });
}

void UserActivityLoggerScriptingInterface::closedTablet() {
    doLogAction("closed_tablet");
}

void UserActivityLoggerScriptingInterface::openedMarketplace() {
    doLogAction("opened_marketplace");
}

void UserActivityLoggerScriptingInterface::toggledAway(bool isAway) {
    doLogAction("toggled_away", { { "is_away", isAway } });
}

void UserActivityLoggerScriptingInterface::tutorialProgress( QString stepName, int stepNumber, float secondsToComplete,
        float tutorialElapsedTime, QString tutorialRunID, int tutorialVersion, QString controllerType) {
    doLogAction("tutorial_progress", {
        { "tutorial_run_id", tutorialRunID },
        { "tutorial_version", tutorialVersion },
        { "step", stepName },
        { "step_number", stepNumber },
        { "seconds_to_complete", secondsToComplete },
        { "tutorial_elapsed_seconds", tutorialElapsedTime },
        { "controller_type", controllerType }
    });

}

void UserActivityLoggerScriptingInterface::palAction(QString action, QString target) {
    QJsonObject payload;
    payload["action"] = action;
    if (target.length() > 0) {
        payload["target"] = target;
    }
    doLogAction("pal_activity", payload);
}

void UserActivityLoggerScriptingInterface::palOpened(float secondsOpened) {
    doLogAction("pal_opened", {
        { "seconds_opened", secondsOpened }
    });
}

void UserActivityLoggerScriptingInterface::makeUserConnection(QString otherID, bool success, QString detailsString) {
    QJsonObject payload;
    payload["otherUser"] = otherID;
    payload["success"] = success;
    if (detailsString.length() > 0) {
        payload["details"] = detailsString;
    }
    doLogAction("makeUserConnection", payload);
}

void UserActivityLoggerScriptingInterface::bubbleToggled(bool newValue) {
    doLogAction(newValue ? "bubbleOn" : "bubbleOff");
}

void UserActivityLoggerScriptingInterface::bubbleActivated() {
    doLogAction("bubbleActivated");
}

void UserActivityLoggerScriptingInterface::logAction(QString action, QVariantMap details) {
    doLogAction(action, QJsonObject::fromVariantMap(details));
}

void UserActivityLoggerScriptingInterface::doLogAction(QString action, QJsonObject details) {
    QMetaObject::invokeMethod(&UserActivityLogger::getInstance(), "logAction",
                              Q_ARG(QString, action),
                              Q_ARG(QJsonObject, details));
}

void UserActivityLoggerScriptingInterface::commercePurchaseSuccess(QString marketplaceID, int cost, bool firstPurchaseOfThisItem) {
    QJsonObject payload;
    payload["marketplaceID"] = marketplaceID;
    payload["cost"] = cost;
    payload["firstPurchaseOfThisItem"] = firstPurchaseOfThisItem;
    doLogAction("commercePurchaseSuccess", payload);
}

void UserActivityLoggerScriptingInterface::commercePurchaseFailure(QString marketplaceID, int cost, bool firstPurchaseOfThisItem, QString errorDetails) {
    QJsonObject payload;
    payload["marketplaceID"] = marketplaceID;
    payload["cost"] = cost;
    payload["firstPurchaseOfThisItem"] = firstPurchaseOfThisItem;
    payload["errorDetails"] = errorDetails;
    doLogAction("commercePurchaseFailure", payload);
}

void UserActivityLoggerScriptingInterface::commerceEntityRezzed(QString marketplaceID, QString source, QString type) {
    QJsonObject payload;
    payload["marketplaceID"] = marketplaceID;
    payload["source"] = source;
    payload["type"] = type;
    doLogAction("commerceEntityRezzed", payload);
}

void UserActivityLoggerScriptingInterface::commerceWalletSetupStarted(int timestamp, QString setupAttemptID, int setupFlowVersion, QString referrer, QString currentDomain) {
    QJsonObject payload;
    payload["timestamp"] = timestamp;
    payload["setupAttemptID"] = setupAttemptID;
    payload["setupFlowVersion"] = setupFlowVersion;
    payload["referrer"] = referrer;
    payload["currentDomain"] = currentDomain;
    doLogAction("commerceWalletSetupStarted", payload);
}

void UserActivityLoggerScriptingInterface::commerceWalletSetupProgress(int timestamp, QString setupAttemptID, int secondsElapsed, int currentStepNumber, QString currentStepName) {
    QJsonObject payload;
    payload["timestamp"] = timestamp;
    payload["setupAttemptID"] = setupAttemptID;
    payload["secondsElapsed"] = secondsElapsed;
    payload["currentStepNumber"] = currentStepNumber;
    payload["currentStepName"] = currentStepName;
    doLogAction("commerceWalletSetupProgress", payload);
}

void UserActivityLoggerScriptingInterface::commerceWalletSetupFinished(int timestamp, QString setupAttemptID, int secondsToComplete) {
    QJsonObject payload;
    payload["timestamp"] = timestamp;
    payload["setupAttemptID"] = setupAttemptID;
    payload["secondsToComplete"] = secondsToComplete;
    doLogAction("commerceWalletSetupFinished", payload);
}

void UserActivityLoggerScriptingInterface::commercePassphraseEntry(QString source) {
    QJsonObject payload;
    payload["source"] = source;
    doLogAction("commercePassphraseEntry", payload);
}

void UserActivityLoggerScriptingInterface::commercePassphraseAuthenticationStatus(QString status) {
    QJsonObject payload;
    payload["status"] = status;
    doLogAction("commercePassphraseAuthenticationStatus", payload);

}
