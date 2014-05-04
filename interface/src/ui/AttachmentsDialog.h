//
//  AttachmentsDialog.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 5/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AttachmentsDialog_h
#define hifi_AttachmentsDialog_h

#include <QDialog>

/// Allows users to edit the avatar attachments.
class AttachmentsDialog : public QDialog {
    Q_OBJECT

public:
    
    AttachmentsDialog();

private slots:
    
    void addAttachment();
};

#endif // hifi_AttachmentsDialog_h
