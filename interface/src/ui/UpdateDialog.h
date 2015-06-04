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
    
public:
    UpdateDialog(QQuickItem* parent = nullptr);
    
signals:
    
protected:
    void hide();
    
private:
    

};

#endif /* defined(__hifi__UpdateDialog__) */
