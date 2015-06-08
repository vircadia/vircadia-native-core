//
//  UpdateDialog.h
//  hifi
//
//  Created by Leonardo Murillo on 6/3/15.
//
//

#pragma once
#ifndef __hifi__UpdateDialog__
#define __hifi__UpdateDialog__


#include <QtCore/QCoreApplication>
#include <OffscreenQmlDialog.h>

class UpdateDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL
    
    Q_PROPERTY(QString updateAvailableDetails READ updateAvailableDetails)
    Q_PROPERTY(QString releaseNotes READ releaseNotes)
    
public:
    UpdateDialog(QQuickItem* parent = nullptr);
    QString updateAvailableDetails() const;
    QString releaseNotes() const;
    
private:
    QString _updateAvailableDetails;
    QString _releaseNotes;

protected:
    void hide();
    Q_INVOKABLE void triggerUpgrade();
    Q_INVOKABLE void closeDialog();

};

#endif /* defined(__hifi__UpdateDialog__) */
