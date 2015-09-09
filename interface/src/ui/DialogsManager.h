//
//  DialogsManager.h
//  interface/src/ui
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DialogsManager_h
#define hifi_DialogsManager_h

#include <QPointer>

#include <Application.h>
#include <DependencyManager.h>

#include "HMDToolsDialog.h"

class QAction;

class AddressBarDialog;
class AnimationsDialog;
class AttachmentsDialog;
class AudioStatsDialog;
class BandwidthDialog;
class CachesSizeDialog;
class DiskCacheEditor;
class LodToolsDialog;
class LoginDialog;
class OctreeStatsDialog;
class PreferencesDialog;
class ScriptEditorWindow;
class QMessageBox;
class DomainConnectionDialog;
class UpdateDialog;

class DialogsManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    QPointer<AudioStatsDialog> getAudioStatsDialog() const { return _audioStatsDialog; }
    QPointer<BandwidthDialog> getBandwidthDialog() const { return _bandwidthDialog; }
    QPointer<HMDToolsDialog> getHMDToolsDialog() const { return _hmdToolsDialog; }
    QPointer<LodToolsDialog> getLodToolsDialog() const { return _lodToolsDialog; }
    QPointer<OctreeStatsDialog> getOctreeStatsDialog() const { return _octreeStatsDialog; }
    QPointer<PreferencesDialog> getPreferencesDialog() const { return _preferencesDialog; }

public slots:
    void toggleAddressBar();
    void toggleDiskCacheEditor();
    void toggleLoginDialog();
    void showLoginDialog();
    void octreeStatsDetails();
    void cachesSizeDialog();
    void editPreferences();
    void editAttachments();
    void editAnimations();
    void audioStatsDetails();
    void bandwidthDetails();
    void lodTools();
    void hmdTools(bool showTools);
    void showScriptEditor();
    void showIRCLink();
    void showDomainConnectionDialog();
    
    // Application Update
    void showUpdateDialog();

signals:
    void addressBarToggled();

private slots:
    void toggleToolWindow();
    void hmdToolsClosed();

private:
    DialogsManager() {}

    template<typename T>
    void maybeCreateDialog(QPointer<T>& member) {
        if (!member) {
            MainWindow* parent = qApp->getWindow();
            Q_CHECK_PTR(parent);
            member = new T(parent);
            Q_CHECK_PTR(member);

            if (_hmdToolsDialog && member->windowHandle()) {
                _hmdToolsDialog->watchWindow(member->windowHandle());
            }
        }
    }

    QPointer<AddressBarDialog> _addressBarDialog;
    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<AudioStatsDialog> _audioStatsDialog;
    QPointer<BandwidthDialog> _bandwidthDialog;
    QPointer<CachesSizeDialog> _cachesSizeDialog;
    QPointer<DiskCacheEditor> _diskCacheEditor;
    QPointer<QMessageBox> _ircInfoBox;
    QPointer<HMDToolsDialog> _hmdToolsDialog;
    QPointer<LodToolsDialog> _lodToolsDialog;
    QPointer<LoginDialog> _loginDialog;
    QPointer<OctreeStatsDialog> _octreeStatsDialog;
    QPointer<PreferencesDialog> _preferencesDialog;
    QPointer<ScriptEditorWindow> _scriptEditor;
    QPointer<DomainConnectionDialog> _domainConnectionDialog;
    QPointer<UpdateDialog> _updateDialog;
};

#endif // hifi_DialogsManager_h
