//
//  DiskCacheEditor.h
//
//
//  Created by Clement on 3/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DiskCacheEditor_h
#define hifi_DiskCacheEditor_h

#include <QObject>
#include <QPointer>

class QDialog;
class QLabel;
class QWindow;

class DiskCacheEditor : public QObject {
    Q_OBJECT
    
public:
    DiskCacheEditor(QWidget* parent = nullptr);
    
    QWindow* windowHandle();
    
public slots:
    void toggle();
    
private slots:
    void refresh();
    void clear();
    
private:
    void makeDialog();
    
    QPointer<QDialog> _dialog;
    QPointer<QLabel> _path;
    QPointer<QLabel> _size;
    QPointer<QLabel> _maxSize;
};

#endif // hifi_DiskCacheEditor_h