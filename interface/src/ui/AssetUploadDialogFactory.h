//
//  AssetUploadDialogFactory.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 2015-08-26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_AssetUploadDialogFactory_h
#define hifi_AssetUploadDialogFactory_h

#include <QtCore/QObject>

class AssetUpload;

class AssetUploadDialogFactory : public QObject {
    Q_OBJECT
public:
    AssetUploadDialogFactory(const AssetUploadDialogFactory& other) = delete;
    AssetUploadDialogFactory& operator=(const AssetUploadDialogFactory& rhs) = delete;
    
    static AssetUploadDialogFactory& getInstance();
    static void showErrorDialog(AssetUpload* upload, QWidget* dialogParent, const QString& overrideMessage = QString());
    
    void setDialogParent(QWidget* dialogParent) { _dialogParent = dialogParent; }
    
public slots:
    void showDialog();
    void handleUploadFinished(AssetUpload* upload, const QString& hash);
    
private:
    AssetUploadDialogFactory() = default;
    
    
    
    QWidget* _dialogParent { nullptr };
};

#endif // hifi_AssetUploadDialogFactory_h
