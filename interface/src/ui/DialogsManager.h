//
//  DialogsManager.h
//
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
class CachesSizeDialog;
class ChatWindow;
class BandwidthDialog;
class LodToolsDialog;
class LoginDialog;
class MetavoxelEditor;
class MetavoxelNetworkSimulator;
class OctreeStatsDialog;
class PreferencesDialog;
class ScriptEditorWindow;

class DialogsManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    QPointer<BandwidthDialog> getBandwidthDialog() const { return _bandwidthDialog; }
    QPointer<HMDToolsDialog> getHMDToolsDialog() const { return _hmdToolsDialog; }
    QPointer<LodToolsDialog> getLodToolsDialog() const { return _lodToolsDialog; }
    QPointer<OctreeStatsDialog> getOctreeStatsDialog() const { return _octreeStatsDialog; }
    
    void setupChat();
    
public slots:
    void toggleAddressBar();
    void toggleLoginDialog();
    void showLoginDialog();
    void octreeStatsDetails();
    void cachesSizeDialog();
    void editPreferences();
    void editAttachments();
    void editAnimations();
    void bandwidthDetails();
    void lodTools();
    void hmdTools(bool showTools);
    void showMetavoxelEditor();
    void showMetavoxelNetworkSimulator();
    void showScriptEditor();
    void showChat();
    
private slots:
    void toggleToolWindow();
    void hmdToolsClosed();
    void toggleChat();
    
private:
    DialogsManager() {}
    
    template<typename T>
    void maybeCreateDialog(QPointer<T>& member) {
        if (!member) {
            MainWindow* parent = qApp->getWindow();
            Q_CHECK_PTR(parent);
            member = new T(parent);
            Q_CHECK_PTR(member);
            
            if (_hmdToolsDialog) {
                _hmdToolsDialog->watchWindow(member->windowHandle());
            }
        }
    }
    
    QPointer<AddressBarDialog> _addressBarDialog;
    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<BandwidthDialog> _bandwidthDialog;
    QPointer<CachesSizeDialog> _cachesSizeDialog;
    QPointer<ChatWindow> _chatWindow;
    QPointer<HMDToolsDialog> _hmdToolsDialog;
    QPointer<LodToolsDialog> _lodToolsDialog;
    QPointer<LoginDialog> _loginDialog;
    QPointer<MetavoxelEditor> _metavoxelEditor;
    QPointer<MetavoxelNetworkSimulator> _metavoxelNetworkSimulator;
    QPointer<OctreeStatsDialog> _octreeStatsDialog;
    QPointer<PreferencesDialog> _preferencesDialog;
    QPointer<ScriptEditorWindow> _scriptEditor;
};

#endif // hifi_DialogsManager_h