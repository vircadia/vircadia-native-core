//
//  ScreenshareScriptingInterface.h
//  interface/src/scripting/
//
//  Created by Milad Nazeri and Zach Fox on 2019-10-23.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScreenshareScriptingInterface_h
#define hifi_ScreenshareScriptingInterface_h

#include <QObject>
#include <QProcess>
#include <QtCore/QCoreApplication>
#include <QNetworkReply>
#include <QtCore/QSharedPointer>

#include <PathUtils.h>
#include <ReceivedMessage.h>

class ScreenshareScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(float localWebEntityZOffset MEMBER _localWebEntityZOffset NOTIFY localWebEntityZOffsetChanged)
public:
    ScreenshareScriptingInterface();
    ~ScreenshareScriptingInterface();

    Q_INVOKABLE void startScreenshare(const QUuid& screenshareZoneID, const QUuid& smartboardEntityID, const bool& isPresenter = false);
    Q_INVOKABLE void stopScreenshare();

signals:
    void screenshareError();
    void screenshareProcessTerminated();
    void startScreenshareViewer();
    void localWebEntityZOffsetChanged(const float& newZOffset);

private slots:
    void processAvatarZonePresencePacketOnClient(QSharedPointer<ReceivedMessage> message);
    void onWebEventReceived(const QUuid& entityID, const QVariant& message);
    void handleSuccessfulScreenshareInfoGet(QNetworkReply* reply);
    void handleFailedScreenshareInfoGet(QNetworkReply* reply);

private:
#if DEV_BUILD
#ifdef Q_OS_WIN
    const QString SCREENSHARE_EXE_PATH{ PathUtils::projectRootPath() + "/screenshare/hifi-screenshare-win32-x64/hifi-screenshare.exe" };
#elif defined(Q_OS_MAC)
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/../Resources/hifi-screenshare.app" };
#else
    // This path won't exist on other platforms, so the Screenshare Scripting Interface will exit early when invoked.
    const QString SCREENSHARE_EXE_PATH{ PathUtils::projectRootPath() + "/screenshare/hifi-screenshare-other-os/hifi-screenshare" };
#endif
#else
#ifdef Q_OS_WIN
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/hifi-screenshare/hifi-screenshare.exe" };
#elif defined(Q_OS_MAC)
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/../Resources/hifi-screenshare.app" };
#else
    // This path won't exist on other platforms, so the Screenshare Scripting Interface will exit early when invoked.
    const QString SCREENSHARE_EXE_PATH{ QCoreApplication::applicationDirPath() + "/hifi-screenshare/hifi-screenshare" };
#endif
#endif

    QTimer* _requestScreenshareInfoRetryTimer{ nullptr };
    int _requestScreenshareInfoRetries{ 0 };
    void requestScreenshareInfo();

    // Empirically determined. The default value here can be changed in Screenshare scripts, which enables faster iteration when we discover
    // positional issues with various Smartboard entities.
    // The following four values are closely linked:
    // 1. The z-offset of whiteboard polylines (`STROKE_FORWARD_OFFSET_M` in `drawSphereClient.js`).
    // 2. The z-offset of the screenshare local web entity (`LOCAL_WEB_ENTITY_Z_OFFSET` in `smartboardZoneClient.js`).
    // 3. The z-offset of the screenshare "glass bezel" (`DEFAULT_SMARTBOARD_SCREENSHARE_GLASS_PROPS` in `smartboardZoneClient.js`).
    // 4. The z-offset of the screenshare "status icon" (handled in the screenshare JSON file).
    float _localWebEntityZOffset{ 0.0375f };

    std::unique_ptr<QProcess> _screenshareProcess{ nullptr };
    QUuid _screenshareViewerLocalWebEntityUUID;
    QString _token{ "" };
    QString _projectAPIKey{ "" };
    QString _sessionID{ "" };
    QUuid _screenshareZoneID;
    QUuid _smartboardEntityID;
    bool _isPresenter{ false };

    QUuid _lastAuthorizedZoneID;
    bool _waitingForAuthorization{ false };
};

#endif // hifi_ScreenshareScriptingInterface_h
