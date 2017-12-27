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
    void setAPIKey(QString key);

    QString getAssetList(QString keyword, QString category, QString format);
    QString getFBX(QString keyword, QString category);
    QString getOBJ(QString keyword, QString category);
    QString getBlocks(QString keyword, QString category);
    QString getGLTF(QString keyword, QString category);
    QString getGLTF2(QString keyword, QString category);
    QString getTilt(QString keyword, QString category);
    QString getModelInfo(QString name);

private:
    QString authCode;

    QUrl formatURLQuery(QString keyword, QString category, QString format);
    QString getModelURL(QUrl url);
    QByteArray getHTTPRequest(QUrl url);
    QVariant parseJSON(QUrl url, int fileType);
    int getRandIntInRange(int length);

};

#endif // hifi_GooglePolyScriptingInterface_h
