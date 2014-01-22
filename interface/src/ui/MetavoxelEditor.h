//
//  MetavoxelEditor.h
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelEditor__
#define __interface__MetavoxelEditor__

#include <QDialog>

#include "renderer/ProgramObject.h"

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QListWidget;

/// Allows editing metavoxels.
class MetavoxelEditor : public QDialog {
    Q_OBJECT

public:
    
    MetavoxelEditor();

    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    
    void updateValueEditor();
    void createNewAttribute();
    void updateGridPosition();
    
    void render();
    
private:
    
    void updateAttributes(const QString& select = QString());
    QString getSelectedAttribute() const;
    void resetState();
    
    QListWidget* _attributes;
    QComboBox* _gridPlane;
    QDoubleSpinBox* _gridSpacing;
    QDoubleSpinBox* _gridPosition;
    QGroupBox* _value;
    
    enum State { HOVERING_STATE, DRAGGING_STATE, RAISING_STATE };
    
    State _state;
    
    glm::vec2 _mousePosition; ///< the position of the mouse in rotated space
    
    glm::vec2 _startPosition; ///< the first corner of the selection base
    glm::vec2 _endPosition; ///< the second corner of the selection base
    float _height; ///< the selection height
    
    static ProgramObject _gridProgram;
};

#endif /* defined(__interface__MetavoxelEditor__) */
