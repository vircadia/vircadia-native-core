//
//  AnimNodeLoader.h
//
//  Created by Anthony J. Thibault on 9/2/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimNodeLoader_h
#define hifi_AnimNodeLoader_h

#include <memory>

#include <QString>
#include <QUrl>
#include <QNetworkReply>

#include "AnimNode.h"

class Resource;

class AnimNodeLoader : public QObject {
    Q_OBJECT

public:
    AnimNodeLoader(const QUrl& url);

signals:
    void success(AnimNode::Pointer node);
    void error(int error, QString str);

protected:
    // synchronous
    static AnimNode::Pointer load(const QByteArray& contents, const QUrl& jsonUrl);

protected slots:
    void onRequestDone(QNetworkReply& request);
    void onRequestError(QNetworkReply::NetworkError error);

protected:
    QUrl _url;
    Resource* _resource;
private:

    // no copies
    AnimNodeLoader(const AnimNodeLoader&) = delete;
    AnimNodeLoader& operator=(const AnimNodeLoader&) = delete;
};

#endif // hifi_AnimNodeLoader
