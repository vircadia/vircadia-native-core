//
//  BakeAssetTask.h
//  assignment-client/src/assets
//
//  Created by Stephen Birarda on 9/18/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BakeAssetTask_h
#define hifi_BakeAssetTask_h

#include <memory>

#include <QtCore/QDebug>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QDir>
#include <QProcess>

#include <AssetUtils.h>

class BakeAssetTask : public QObject, public QRunnable {
    Q_OBJECT
public:
    BakeAssetTask(const AssetUtils::AssetHash& assetHash, const AssetUtils::AssetPath& assetPath, const QString& filePath);

    bool isBaking() { return _isBaking.load(); }

    void run() override;

    void abort();
    bool wasAborted() const { return _wasAborted.load(); }

signals:
    void bakeComplete(QString assetHash, QString assetPath, QString tempOutputDir, QVector<QString> outputFiles);
    void bakeFailed(QString assetHash, QString assetPath, QString errors);
    void bakeAborted(QString assetHash, QString assetPath);
    
private:
    std::atomic<bool> _isBaking { false };
    AssetUtils::AssetHash _assetHash;
    AssetUtils::AssetPath _assetPath;
    QString _filePath;
    std::unique_ptr<QProcess> _ovenProcess { nullptr };
    std::atomic<bool> _wasAborted { false };
};

#endif // hifi_BakeAssetTask_h
