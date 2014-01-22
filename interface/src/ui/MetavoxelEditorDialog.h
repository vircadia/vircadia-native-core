//
//  MetavoxelEditorDialog.h
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelEditorDialog__
#define __interface__MetavoxelEditorDialog__

#include <QDialog>

class QGroupBox;
class QListWidget;

/// Allows editing metavoxels.
class MetavoxelEditorDialog : public QDialog {
    Q_OBJECT

public:
    
    MetavoxelEditorDialog();

private slots:
    
    void updateValueEditor();
    void createNewAttribute();
    
private:
    
    void updateAttributes(const QString& select = QString());
    QString getSelectedAttribute() const;
    
    QListWidget* _attributes;
    QGroupBox* _value;
};

#endif /* defined(__interface__MetavoxelEditorDialog__) */
