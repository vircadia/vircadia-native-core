//
//  InfoView.h
//  hifi
//
//  Created by Stojce Slavkovski on 9/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__InfoView__
#define __hifi__InfoView__

#include <QtWebKitWidgets/QWebView>

class InfoView : public QWebView {
    Q_OBJECT
public:
    static void showFirstTime();
    static void forcedShow();
    
private:
    InfoView(bool forced);
    bool _forced;
    bool shouldShow();
    
private slots:
    void loaded(bool ok);
};

#endif /* defined(__hifi__InfoView__) */
