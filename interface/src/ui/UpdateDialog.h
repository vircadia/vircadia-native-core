//
//  UpdateDialog.h
//  interface
//
//  Created by Leonardo Murillo <leo@highfidelity.io> on 1/8/14.
//  Copyright (c) 2013, 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__UpdateDialog__
#define __hifi__UpdateDialog__

#include <QDialog>
#include <QWidget>

#include <iostream>

class UpdateDialog : public QDialog {
    Q_OBJECT
    
public:
    UpdateDialog(QWidget* parent, QString releaseNotes, QString latestVersion, QUrl downloadURL);
    
private:
    QWidget* _dialogWidget;
    
private slots:
    void handleDownload();
    void handleSkip();
    void handleClose();
};

#endif /* defined(__hifi__UpdateDialog__) */
