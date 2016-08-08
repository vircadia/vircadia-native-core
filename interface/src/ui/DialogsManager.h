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

#include <DependencyManager.h>

#include "HMDToolsDialog.h"

class AnimationsDialog;
class AttachmentsDialog;
class AudioStatsDialog;
class BandwidthDialog;
class CachesSizeDialog;
class DiskCacheEditor;
class LodToolsDialog;
class OctreeStatsDialog;
class ScriptEditorWindow;
class QMessageBox;
class DomainConnectionDialog;

class DialogsManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    QPointer<AudioStatsDialog> getAudioStatsDialog() const { return _audioStatsDialog; }
    QPointer<BandwidthDialog> getBandwidthDialog() const { return _bandwidthDialog; }
    QPointer<HMDToolsDialog> getHMDToolsDialog() const { return _hmdToolsDialog; }
    QPointer<LodToolsDialog> getLodToolsDialog() const { return _lodToolsDialog; }
    QPointer<OctreeStatsDialog> getOctreeStatsDialog() const { return _octreeStatsDialog; }
    void emitAddressBarShown(bool visible) { emit addressBarShown(visible); }

public slots:
    void toggleAddressBar();
    void showAddressBar();
    void toggleDiskCacheEditor();
    void toggleLoginDialog();
    void showLoginDialog();
    void octreeStatsDetails();
    void cachesSizeDialog();
    void audioStatsDetails();
    void bandwidthDetails();
    void lodTools();
    void hmdTools(bool showTools);
    void showScriptEditor();
    void showDomainConnectionDialog();
    
    // Application Update
    void showUpdateDialog();

signals:
    void addressBarToggled();
    void addressBarShown(bool visible);

private slots:
    void hmdToolsClosed();

private:
    DialogsManager() {}

    template<typename T>
    void maybeCreateDialog(QPointer<T>& member);

    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<AudioStatsDialog> _audioStatsDialog;
    QPointer<BandwidthDialog> _bandwidthDialog;
    QPointer<CachesSizeDialog> _cachesSizeDialog;
    QPointer<DiskCacheEditor> _diskCacheEditor;
    QPointer<QMessageBox> _ircInfoBox;
    QPointer<HMDToolsDialog> _hmdToolsDialog;
    QPointer<LodToolsDialog> _lodToolsDialog;
    QPointer<OctreeStatsDialog> _octreeStatsDialog;
    QPointer<ScriptEditorWindow> _scriptEditor;
    QPointer<DomainConnectionDialog> _domainConnectionDialog;
};

#endif // hifi_DialogsManager_h
