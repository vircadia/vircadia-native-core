//
//  PreferencesDialog.h
//  hifi
//
//  Created by Stojce Slavkovski on 2/22/14.
//
//

#ifndef __hifi__PreferencesDialog__
#define __hifi__PreferencesDialog__

#include "FramelessDialog.h"
#include "ui_preferencesDialog.h"

#include <QString>

class PreferencesDialog : public FramelessDialog {
    Q_OBJECT
    
public:
    PreferencesDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~PreferencesDialog();

protected:
    void resizeEvent(QResizeEvent* resizeEvent);
    
private:
    void loadPreferences();
    void savePreferences();
    void openHeadModelBrowser();
    void openBodyModelBrowser();

    Ui_PreferencesDialog ui;
    QString _faceURLString;
    QString _skeletonURLString;
    QString _displayNameString;

private slots:
    void accept();
    void setHeadUrl(QString modelUrl);
    void setSkeletonUrl(QString modelUrl);
    
};

#endif /* defined(__hifi__PreferencesDialog__) */
