//
//  UpdateDialog.h
//  interface
//
//  Created by Leonardo Murillo <leo@highfidelity.io> on 1/8/14.
//  Copyright (c) 2013, 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__UpdateDialog__
#define __hifi__UpdateDialog__

#include <QWidget>

class UpdateDialog : public QDialog {
    Q_OBJECT
    
public:
    UpdateDialog(QWidget* parent, const QString& releaseNotes, const QString& latestVersion, const QUrl& downloadURL);
    
private:
    QString _latestVersion;
    QUrl _downloadUrl;
    
private slots:
    void handleDownload();
    void handleSkip();
};

#endif /* defined(__hifi__UpdateDialog__) */
