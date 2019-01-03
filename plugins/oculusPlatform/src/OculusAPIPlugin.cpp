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

static const Ticket INVALID_TICKET = Ticket();
static std::atomic_bool initialized { false };
static ovrSession session { nullptr };

class OculusCallbackManager {
public:
    OculusCallbackManager();
};

static OculusCallbackManager oculusCallbackManager;

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
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(100ms);
            auto message = ovr_PopMessage();
            if (ovr_Message_GetType(message) == ovrMessage_Entitlement_GetIsViewerEntitled && ovr_Message_IsError(message)) {
                qDebug() << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
            }
        }
    }
#endif

    initialized = true;
    return initialized;
}

void OculusAPIPlugin::shutdown() {
}

void OculusAPIPlugin::runCallbacks() {
}

void OculusAPIPlugin::requestTicket(TicketRequestCallback callback) {
    if (!initialized) {
        if (!ovr_IsPlatformInitialized()) {
            init();
        }
        else {
            qWarning() << "Oculus is not running";
            callback(INVALID_TICKET);
            return;
        }
    }

    if (!initialized) {
        qDebug() << "Oculus not initialized";
        return;
    }

    auto userProof = requestUserProof();
    if (userProof == "") {
        qWarning() << "User proof unavailable.";
        callback(INVALID_TICKET);
        return;
    } else {
        oculusCallbackManager;
    }
}

bool OculusAPIPlugin::isRunning() {
    return initialized;
}

QString OculusAPIPlugin::requestUserProof() {
    if (initialized) {
        QTimer timer;
        timer.start(1000);
        auto request = ovr_User_GetUserProof();
        ovrMessageHandle message { nullptr };
        while (message = ovr_PopMessage()) {
            if (message == nullptr) {
                break;
            } else if (!timer.isActive()) {
                qCDebug(oculusLog) << "login user id timeout after 1 second";
                return "";
            } else if (ovr_Message_GetType(message) == ovrMessage_User_GetUserProof) {
                if (!ovr_Message_IsError(message)) {
                    ovrUserProofHandle userProof = ovr_Message_GetUserProof(message);
                    QString nonce = ovr_UserProof_GetNonce(userProof);
                    qCDebug(oculusLog) << "User nonce: " << nonce;
                    return nonce;
                } else {
                    qDebug() << "Error getting user proof: " << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                    return "";
                }
            }
        }
        return "";
    }
}

QString OculusAPIPlugin::getLoggedInUserID() {
    if (initialized) {
        QTimer timer;
        timer.start(1000);
        auto request = ovr_User_GetLoggedInUser();
        ovrMessageHandle message { nullptr };
        while (message = ovr_PopMessage()) {
            if (message == nullptr) {
                break;
            } else if (!timer.isActive()) {
                qCDebug(oculusLog) << "login user id timeout after 1 second";
                return "";
            } else if (ovr_Message_GetType(message) == ovrMessage_User_GetLoggedInUser) {
                if (!ovr_Message_IsError(message)) {
                    ovrUserHandle user = ovr_Message_GetUser(message);
                    qCDebug(oculusLog) << "UserID: " << ovr_User_GetID(user) << ", Oculus ID: " << ovr_User_GetOculusID(user);
                    return ovr_User_GetOculusID(user);
                } else {
                    qDebug() << "Error getting user id: " << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                    return "";
                }
            }
        }
        return "";
    }
}

QString OculusAPIPlugin::getOculusVRBuildID() {
    return QString(OVR_MAJOR_VERSION + "." + OVR_MINOR_VERSION);
}
