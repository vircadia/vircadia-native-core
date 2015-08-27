//
//  AssetUploadDialogFactory.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 2015-08-26.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetUploadDialogFactory.h"

#include <AssetClient.h>
#include <AssetUpload.h>
#include <AssetUtils.h>

#include <QtCore/QDebug>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>

AssetUploadDialogFactory& AssetUploadDialogFactory::getInstance() {
    static AssetUploadDialogFactory staticInstance;
    return staticInstance;
}

AssetUploadDialogFactory::AssetUploadDialogFactory() {
    
}

void AssetUploadDialogFactory::showDialog() {
    auto filename = QFileDialog::getOpenFileUrl(_dialogParent, "Select a file to upload");
    
    if (!filename.isEmpty()) {
        qDebug() << "Selected filename for upload to asset-server: " << filename;
        
        auto assetClient = DependencyManager::get<AssetClient>();
        auto upload = assetClient->createUpload(filename.path());
        
        if (upload) {
            // connect to the finished signal so we know when the AssetUpload is done
            QObject::connect(upload, &AssetUpload::finished, this, &AssetUploadDialogFactory::handleUploadFinished);
            
            // start the upload now
            upload->start();
        }
    }
}

void AssetUploadDialogFactory::handleUploadFinished(AssetUpload* upload, const QString& hash) {
    if (true) {
        // show message box for successful upload, with copiable text for ATP hash
        QDialog* hashCopyDialog = new QDialog(_dialogParent);
        
        // delete the dialog on close
        hashCopyDialog->setAttribute(Qt::WA_DeleteOnClose);
        
        // set the window title
        hashCopyDialog->setWindowTitle(tr("Successful Asset Upload"));
        
        // setup a layout for the contents of the dialog
        QVBoxLayout* boxLayout = new QVBoxLayout;
        
        // set the label text (this shows above the text box)
        QLabel* lineEditLabel = new QLabel;
        lineEditLabel->setText(QString("ATP URL for %1").arg(upload->getFilename()));
        
        // setup the line edit to hold the copiable text
        QLineEdit* lineEdit = new QLineEdit;
        
        // set the ATP URL as the text value so it's copiable
        lineEdit->insert(QString("%1://%2").arg(ATP_SCHEME).arg(hash));
        
        // add the label and line edit to the dialog
        boxLayout->addWidget(lineEditLabel);
        boxLayout->addWidget(lineEdit);
        
        // setup an OK button to close the dialog
        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
        connect(buttonBox, &QDialogButtonBox::accepted, hashCopyDialog, &QDialog::close);
        boxLayout->addWidget(buttonBox);
        
        // set the new layout on the dialog
        hashCopyDialog->setLayout(boxLayout);
        
        // show the new dialog
        hashCopyDialog->show();
    } else {
        //
    }
}
