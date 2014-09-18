//
//  DataWebDialog.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-09-17.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DataWebDialog_h
#define hifi_DataWebDialog_h

#include <qobject.h>
#include <qwebview.h>

class DataWebDialog : public QWebView {
    Q_OBJECT
public:
    DataWebDialog();
    static DataWebDialog* dialogForPath(const QString& path);
};

#endif // hifi_WebkitDialog_h