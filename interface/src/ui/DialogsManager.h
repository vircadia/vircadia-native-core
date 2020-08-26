//
//  DialogsManager.h
//  interface/src/ui
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DialogsManager_h
#define hifi_DialogsManager_h

#include <QPointer>

#include <DependencyManager.h>

#include "HMDToolsDialog.h"
#include "TestingDialog.h"

class AnimationsDialog;
class AttachmentsDialog;
class CachesSizeDialog;
class LodToolsDialog;
class OctreeStatsDialog;
class ScriptEditorWindow;
class TestingDialog;
class QMessageBox;
class DomainConnectionDialog;

class DialogsManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    QPointer<HMDToolsDialog> getHMDToolsDialog() const { return _hmdToolsDialog; }
    QPointer<LodToolsDialog> getLodToolsDialog() const { return _lodToolsDialog; }
    QPointer<OctreeStatsDialog> getOctreeStatsDialog() const { return _octreeStatsDialog; }
    QPointer<TestingDialog> getTestingDialog() const { return _testingDialog; }
    void emitAddressBarShown(bool visible) { emit addressBarShown(visible); }
    void setAddressBarVisible(bool addressBarVisible);
    void setMetaverseLoginState();
    void setDomainLoginState();
    bool getIsDomainLogin() { return _isDomainLogin; }
    QString getDomainLoginDomain() { return _domainLoginDomain; }

public slots:
    void showAddressBar();
    void hideAddressBar();
    void showFeed();
    void setDomainConnectionFailureVisibility(bool visible);
    void toggleLoginDialog();
    void showLoginDialog();
    void hideLoginDialog();
    void showDomainLoginDialog(const QString& domain = "");
    void octreeStatsDetails();
    void lodTools();
    void hmdTools(bool showTools);
    void showDomainConnectionDialog();
    void toggleAddressBar();
    
    // Application Update
    void showUpdateDialog();

signals:
    void addressBarShown(bool visible);
    void setUseFeed(bool useFeed);

private slots:
    void hmdToolsClosed();

private:
    DialogsManager() {}

    template<typename T>
    void maybeCreateDialog(QPointer<T>& member);

    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<CachesSizeDialog> _cachesSizeDialog;
    QPointer<QMessageBox> _ircInfoBox;
    QPointer<HMDToolsDialog> _hmdToolsDialog;
    QPointer<LodToolsDialog> _lodToolsDialog;
    QPointer<OctreeStatsDialog> _octreeStatsDialog;
    QPointer<TestingDialog> _testingDialog;
    QPointer<DomainConnectionDialog> _domainConnectionDialog;
    bool _dialogCreatedWhileShown { false };
    bool _addressBarVisible { false };

    void setDomainLogin(bool isDomainLogin, const QString& domain = "");
    bool _isDomainLogin { false };
    QString _domainLoginDomain;
};

#endif // hifi_DialogsManager_h
