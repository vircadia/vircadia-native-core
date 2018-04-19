//
//  AndroidHelper.h
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 3/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Android_Helper_h
#define hifi_Android_Helper_h

#include <QObject>
#include <QThread>
#include <AccountManager.h>

class AndroidHelper : public QObject {
    Q_OBJECT
public:
    static AndroidHelper& instance() {
            static AndroidHelper instance;
            return instance;
    }
    void requestActivity(const QString &activityName);
    void notifyLoadComplete();
    void goBackFromAndroidActivity();

    void notifyLoginComplete(bool success);

    QSharedPointer<AccountManager> getAccountManager();

    AndroidHelper(AndroidHelper const&)  = delete;
    void operator=(AndroidHelper const&) = delete;
signals:
    void androidActivityRequested(const QString &activityName);
    void backFromAndroidActivity();
    void qtAppLoadComplete();

    void loginComplete(bool success);

private:
    AndroidHelper();
    ~AndroidHelper();
    QSharedPointer<AccountManager> _accountManager;
    QThread workerThread;
};

#endif