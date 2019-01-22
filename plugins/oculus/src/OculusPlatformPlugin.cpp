//
//  Created by Wayne Chen on 2019/01/08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusPlatformPlugin.h"

#include <shared/GlobalAppProperties.h>

#include <QMetaObject>

#include "OculusHelpers.h"

const char* OculusAPIPlugin::NAME { "Oculus Rift" };

OculusAPIPlugin::OculusAPIPlugin() {
}

OculusAPIPlugin::~OculusAPIPlugin() {
}

bool OculusAPIPlugin::isRunning() {
    return (qApp->property(hifi::properties::OCULUS_STORE).toBool());
}

void OculusAPIPlugin::requestNonceAndUserID(NonceUserIDCallback callback) {
#ifdef OCULUS_APP_ID
    _nonceUserIDCallback = callback;
    ovr_User_GetUserProof();
    ovr_User_GetLoggedInUser();
#endif
}

void OculusAPIPlugin::handleOVREvents() {
#ifdef OCULUS_APP_ID
    if (qApp->property(hifi::properties::OCULUS_STORE).toBool()) {
        // pop messages to see if we got a return for an entitlement check
        ovrMessageHandle message = ovr_PopMessage();

        while (message) {
            switch (ovr_Message_GetType(message)) {
                case ovrMessage_Entitlement_GetIsViewerEntitled: {
                    if (!ovr_Message_IsError(message)) {
                        // this viewer is entitled, no need to flag anything
                        qCDebug(oculusLog) << "Oculus Platform entitlement check succeeded, proceeding normally";
                    } else {
                        // we failed the entitlement check, quit
                        qCDebug(oculusLog) << "Oculus Platform entitlement check failed, app will now quit" << OCULUS_APP_ID;
                        QMetaObject::invokeMethod(qApp, "quit");
                    }
                    break;
                }
                case ovrMessage_User_Get: {
                    if (!ovr_Message_IsError(message)) {
                        qCDebug(oculusLog) << "Oculus Platform user retrieval succeeded";
                        ovrUserHandle user = ovr_Message_GetUser(message);
                        _user = ovr_User_GetOculusID(user);
                        // went all the way through the `requestNonceAndUserID()` pipeline successfully.
                        _nonceChanged = true;
                    } else {
                        qCDebug(oculusLog) << "Oculus Platform user retrieval failed" << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                        // emit the signal so we don't hang for it anywhere else.
                        _nonceChanged = true;
                        _user = "";
                    }
                    break;
                }
                case ovrMessage_User_GetLoggedInUser: {
                    if (!ovr_Message_IsError(message)) {
                        ovrUserHandle user = ovr_Message_GetUser(message);
                        _userID = ovr_User_GetID(user);
                        ovr_User_Get(_userID);
                    } else {
                        qCDebug(oculusLog) << "Oculus Platform user ID retrieval failed" << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                        // emit the signal so we don't hang for it anywhere else.
                        _nonceChanged = true;
                    }
                    break;
                }
                case ovrMessage_User_GetUserProof: {
                    if (!ovr_Message_IsError(message)) {
                        ovrUserProofHandle userProof = ovr_Message_GetUserProof(message);
                        _nonce = ovr_UserProof_GetNonce(userProof);
                        qCDebug(oculusLog) << "Oculus Platform nonce retrieval succeeded: " << _nonce;
                    } else {
                        qCDebug(oculusLog) << "Oculus Platform nonce retrieval failed" << QString(ovr_Error_GetMessage(ovr_Message_GetError(message)));
                        _nonce = "";
                        // emit the signal so we don't hang for it anywhere else.
                        _nonceChanged = true;
                    }
                    break;
                }
            }

            if (_nonceChanged) {
                _nonceUserIDCallback(_nonce, QString::number(_userID));
                _nonceChanged = false;
            }

            // free the message handle to cleanup and not leak
            ovr_FreeMessage(message);

            // pop the next message to check, if there is one
            message = ovr_PopMessage();
        }
    }
#endif
}
