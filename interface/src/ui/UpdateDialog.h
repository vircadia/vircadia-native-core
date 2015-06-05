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

#include <OffscreenQmlDialog.h>

class UpdateDialog : public OffscreenQmlDialog {
    Q_OBJECT
    HIFI_QML_DECL
    
    Q_PROPERTY(QString updateAvailableDetails READ updateAvailableDetails WRITE setUpdateAvailableDetails NOTIFY updateAvailableDetailsChanged)
    
public:
    UpdateDialog(QQuickItem* parent = nullptr);
    ~UpdateDialog();
    
    void displayDialog();
    void setUpdateAvailableDetails(const QString& updateAvailableDetails);
    QString updateAvailableDetails() const;
    
signals:
    void updateAvailableDetailsChanged();
    
protected:
    void hide();
    
    
private:
    QString _updateAvailableDetails;

protected:
    Q_INVOKABLE void triggerBuildDownload(const int& buildNumber);

};

#endif /* defined(__hifi__UpdateDialog__) */
