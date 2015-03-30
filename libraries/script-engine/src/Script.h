//
//  Script.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Script_h
#define hifi_Script_h

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>
#include <QtScript/qscriptengine.h>

#include <ResourceCache.h>

class Script : public Resource {
    Q_OBJECT
public:
    Script(const QUrl& url);
    bool isReady() const { return _isReady; }
    const QString& getContents() { return _contents; }

private:
    QString _contents;
    bool _isReady;
    
    virtual void downloadFinished(QNetworkReply* reply);
};

typedef QSharedPointer<Script> SharedScriptPointer;

#endif // hifi_Script_h
