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

class AndroidHelper : public QObject {
    Q_OBJECT
public:
    static AndroidHelper& instance() {
            static AndroidHelper instance;
            return instance;
    }
    void requestActivity(const QString &activityName, const bool backToScene, QList<QString> args = QList<QString>());
    void notifyLoadComplete();
    void notifyEnterForeground();
    void notifyBeforeEnterBackground();
    void notifyEnterBackground();

    void performHapticFeedback(int duration);
    void processURL(const QString &url);

    AndroidHelper(AndroidHelper const&)  = delete;
    void operator=(AndroidHelper const&) = delete;

public slots:
    void showLoginDialog();

signals:
    void androidActivityRequested(const QString &activityName, const bool backToScene, QList<QString> args = QList<QString>());
    void qtAppLoadComplete();
    void enterForeground();
    void beforeEnterBackground();
    void enterBackground();

    void hapticFeedbackRequested(int duration);

private:
    AndroidHelper();
    ~AndroidHelper();
};

#endif
