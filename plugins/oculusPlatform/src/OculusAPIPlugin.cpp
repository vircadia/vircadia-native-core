//
//  Created by Wayne Chen on 2018/12/20
//  Copyright 2018 High Fidelity Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2-0.html
//

#include "OculusAPIPlugin.h"

#include <atomic>
#include <chrono>
#include <thread>

#include <QtCore/QTimer>

#define OVRPL_DISABLED
#include <OVR_Platform.h>

#include <shared/GlobalAppProperties.h>
#include "OculusHelpers.h"

static std::atomic_bool initialized { false };
static ovrSession session { nullptr };

bool OculusAPIPlugin::init() {
    if (session) {
        return initialized;
    }

    ovrInitParams initParams{ ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, nullptr, 0, 0 };
    initParams.Flags |= ovrInit_Invisible;

    if (!OVR_SUCCESS(ovr_Initialize(&initParams))) {
        qCWarning(oculusLog) << "Failed to initialze Oculus SDK" << hifi::ovr::getError();
        return initialized;
    }

#ifdef OCULUS_APP_ID
    if (qApp->property(hifi::properties::OCULUS_STORE).toBool()) {
        auto result = ovr_PlatformInitializeWindows(OCULUS_APP_ID);
        if (result != ovrPlatformInitialize_Success && result != ovrPlatformInitialize_PreLoaded) {
            qCWarning(oculusLog) << "Unable to initialize the platform for entitlement check - fail the check" << hifi::ovr::getError();
        } else {
            qCDebug(oculusLog) << "Performing Oculus Platform entitlement check";
            ovr_Entitlement_GetIsViewerEntitled();
            QTimer timer;
            timer.start(1000);
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(50ms);
            while (auto message = ovr_PopMessage()) {
                if (timer.remainingTime() == 0) {
                    qCDebug(oculusLog) << "login user id timeout after 1 second";
                    break;
                } 
                switch (ovr_Message_GetType(message)) {
                    case ovrMessage_Entitlement_GetIsViewerEntitled:
                        if (ovr_Message_IsError(message)) {
                            qDebug() << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                        }
                        initialized = true;
                    default:
                        ovr_FreeMessage(message);
                        break;
                }
            }
        }
    }
#endif

    return initialized;
}

void OculusAPIPlugin::shutdown() {
}

void OculusAPIPlugin::runCallbacks() {
}

void OculusAPIPlugin::requestTicket(OculusTicketRequestCallback callback) {
    if (!initialized) {
        if (!ovr_IsPlatformInitialized()) {
            init();
        }
        else {
            qWarning() << "Oculus is not running";
            callback("", "");
            return;
        }
    }

    if (!initialized) {
        qDebug() << "Oculus not initialized";
        return;
    }

    auto userProof = getUserProof();
    auto userID = getLoggedInUserID();
    callback(userProof, userID);
    return;
}

bool OculusAPIPlugin::isRunning() {
    return initialized;
}

QString OculusAPIPlugin::getUserProof() {
    if (initialized) {
        QTimer timer;
        timer.start(5000);
        auto request = ovr_User_GetUserProof();
        ovrMessageHandle message { nullptr };
        bool messageNotReceived = true;
        while (timer.isActive() && messageNotReceived) {
            message = ovr_PopMessage();
            if (timer.remainingTime() == 0) {
                qCDebug(oculusLog) << "user proof timeout after 5 seconds";
                return "";
            }
            if (message != nullptr) {
                switch (ovr_Message_GetType(message)) {
                    case ovrMessage_User_GetUserProof:
                        messageNotReceived = false;
                        if (!ovr_Message_IsError(message)) {
                            ovrUserProofHandle userProof = ovr_Message_GetUserProof(message);
                            QString nonce = ovr_UserProof_GetNonce(userProof);
                            ovr_FreeMessage(message);
                            return nonce;
                        } else {
                            qDebug() << "Error getting user proof: " << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                            ovr_FreeMessage(message);
                            return "";
                        }
                        break;
                    default:
                        ovr_FreeMessage(message);
                        break;
                }
            }
        }
    }
    return "";
}

QString OculusAPIPlugin::getLoggedInUserID() {
    if (initialized) {
        QTimer timer;
        timer.start(5000);
        auto request = ovr_User_GetLoggedInUser();
        ovrMessageHandle message { nullptr };
        bool messageNotReceived = true;
        while (messageNotReceived) {
            if (timer.remainingTime() == 0) {
                qCDebug(oculusLog) << "login user id timeout after 5 seconds";
                return "";
            }
            switch (ovr_Message_GetType(message)) {
                case ovrMessage_User_GetLoggedInUser:
                    messageNotReceived = false;
                    if (!ovr_Message_IsError(message)) {
                        ovrUserHandle user = ovr_Message_GetUser(message);
                        ovr_FreeMessage(message);
                        qCDebug(oculusLog) << "UserID: " << ovr_User_GetID(user) << ", Oculus ID: " << QString(ovr_User_GetOculusID(user));
                        return QString(ovr_User_GetOculusID(user));
                    } else {
                        qDebug() << "Error getting user id: " << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                        ovr_FreeMessage(message);
                        return "";
                    }
                    break;
                default:
                    ovr_FreeMessage(message);
                    break;
            }
        }
    }
    return "";
}

QString OculusAPIPlugin::getOculusVRBuildID() {
    return QString(OVR_MAJOR_VERSION + "." + OVR_MINOR_VERSION);
}
