//
//  AppDelegate.h
//  StackManagerQt/src
//
//  Created by Mohammed Nafees on 06/27/14.
//  Copyright (c) 2014 High Fidelity. All rights reserved.
//

#ifndef hifi_AppDelegate_h
#define hifi_AppDelegate_h


#include <QApplication>
#include <QCoreApplication>
#include <QList>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QUuid>
#include <QHash>
#include <QTimer>

#include "MainWindow.h"

class BackgroundProcess;

class AppDelegate : public QApplication
{
    Q_OBJECT
public:
    static AppDelegate* getInstance() { return static_cast<AppDelegate*>(QCoreApplication::instance()); }

    AppDelegate(int argc, char* argv[]);
    ~AppDelegate();

    void toggleStack(bool start);
    void toggleDomainServer(bool start);
    void toggleAssignmentClientMonitor(bool start);
    void toggleScriptedAssignmentClients(bool start);

    int startScriptedAssignment(const QUuid& scriptID, const QString& pool = QString());
    void stopScriptedAssignment(BackgroundProcess* backgroundProcess);
    void stopScriptedAssignment(const QUuid& scriptID);

    void stopStack() { toggleStack(false); }

    const QString getServerAddress() const;
public slots:
    void downloadContentSet(const QUrl& contentSetURL);
signals:
    void domainServerIDMissing();
    void domainAddressChanged();
    void contentSetDownloadResponse(bool wasSuccessful);
    void indexPathChangeResponse(bool wasSuccessful);
    void stackStateChanged(bool isOn);
private slots:
    void onFileSuccessfullyInstalled(const QUrl& url);
    void requestDomainServerID();
    void handleDomainIDReply();
    void handleDomainGetReply();
    void handleChangeIndexPathResponse();
    void handleContentSetDownloadFinished();
    void checkVersion();
    void parseVersionXml();

private:
    void parseCommandLine();
    void createExecutablePath();
    void downloadLatestExecutablesAndRequirements();

    void changeDomainServerIndexPath(const QString& newPath);

    QNetworkAccessManager* _manager;
    bool _qtReady;
    bool _dsReady;
    bool _dsResourcesReady;
    bool _acReady;
    BackgroundProcess* _domainServerProcess;
    BackgroundProcess* _acMonitorProcess;
    QHash<QUuid, BackgroundProcess*> _scriptProcesses;

    QString _domainServerID;
    QString _domainServerName;

    QTimer _checkVersionTimer;

    MainWindow* _window;
};

#endif
