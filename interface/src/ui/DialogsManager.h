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

#include <DependencyManager.h>

class DialogsManager : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    
public:
    QPointer<OctreeStatsDialog> getOctreeStatsDialog() { return _octreeStatsDialog; }
    
public slots:
    void toggleAddressBar();
    void toggleLoginDialog();
    void showLoginDialog();
    void octreeStatsDetails();
    void cachesSizeDialog();
    void editPreferences();
    void editAttachments();
    void editAnimations();
    
private:
    DialogsManager() {}
    
    template<typename T>
    void maybeCreateDialog(QPointer<T>& member) {
        if (!member) {
            MainWindow* parent = qApp->getWindow();
            Q_CHECK_PTR(parent);
            member = new T(parent);
            Q_CHECK_PTR(member);
        }
    }
    
    QPointer<AnimationsDialog> _animationsDialog;
    QPointer<AttachmentsDialog> _attachmentsDialog;
    QPointer<CachesSizeDialog> _cachesSizeDialog;
    QPointer<PreferencesDialog> _preferencesDialog;
    
    QPointer<AddressBarDialog> _addressBarDialog;
    QPointer<LoginDialog> _loginDialog;
    QPointer<OctreeStatsDialog> _octreeStatsDialog;
};

#endif // hifi_DialogsManager_h