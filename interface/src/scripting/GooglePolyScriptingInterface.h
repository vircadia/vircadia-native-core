//
//  GooglePolyScriptingInterface.h
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 12/3/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GooglePolyScriptingInterface_h
#define hifi_GooglePolyScriptingInterface_h

#include <QObject>
#include <DependencyManager.h>

class GooglePolyScriptingInterface : public QObject, public Dependency {
    Q_OBJECT

public:
    GooglePolyScriptingInterface();

public slots:
    void setAPIKey(const QString& key);

    QString getAssetList(const QString& keyword, const QString& category, const QString& format);
    QString getFBX(const QString& keyword, const QString& category);
    QString getOBJ(const QString& keyword, const QString& category);
    QString getBlocks(const QString& keyword, const QString& categoryy);
    QString getGLTF(const QString& keyword, const QString& category);
    QString getGLTF2(const QString& keyword, const QString& category);
    QString getTilt(const QString& keyword, const QString& category);
    QString getModelInfo(const QString& input);

private:
    QString _authCode;

    QUrl formatURLQuery(const QString& keyword, const QString& category, const QString& format);
    QString getModelURL(const QUrl& url);
    QByteArray getHTTPRequest(const QUrl& url);
    QVariant parseJSON(const QUrl& url, int fileType);
    int getRandIntInRange(int length);

};

#endif // hifi_GooglePolyScriptingInterface_h
