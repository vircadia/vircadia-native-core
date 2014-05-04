//
//  MetavoxelEditor.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "Application.h"
#include "AttachmentsDialog.h"

AttachmentsDialog::AttachmentsDialog() :
    QDialog(Application::getInstance()->getWindow()) {
    
    setWindowTitle("Edit Attachments");
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    QPushButton* newAttachment = new QPushButton("New Attachment");
    connect(newAttachment, SIGNAL(clicked(bool)), SLOT(addAttachment()));
    layout->addWidget(newAttachment);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    connect(buttons, SIGNAL(accepted()), SLOT(deleteLater()));
}

void AttachmentsDialog::addAttachment() {
    
}
