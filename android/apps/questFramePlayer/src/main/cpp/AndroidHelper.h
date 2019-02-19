//
//  Created by Bradley Austin Davis on 2019/02/15
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Android_Helper_h
#define hifi_Android_Helper_h

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtCore/QEventLoop>

class AndroidHelper : public QObject {
    Q_OBJECT
public:
    AndroidHelper(AndroidHelper const&)  = delete;
    void operator=(AndroidHelper const&) = delete;

    static AndroidHelper& instance() {
        static AndroidHelper instance;
        return instance;
    }

    void notifyLoadComplete();
    void notifyEnterForeground();
    void notifyEnterBackground();


signals:
    void qtAppLoadComplete();
    void enterForeground();
    void enterBackground();

private:
    AndroidHelper();
    ~AndroidHelper();
};

#endif
