//
//  MetavoxelEditor.h
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelEditor__
#define __interface__MetavoxelEditor__

#include <QList>
#include <QWidget>

#include "renderer/ProgramObject.h"

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QListWidget;
class QPushButton;
class QScrollArea;

class MetavoxelTool;

/// Allows editing metavoxels.
class MetavoxelEditor : public QWidget {
    Q_OBJECT

public:
    
    MetavoxelEditor();

    QString getSelectedAttribute() const;
    
    double getGridSpacing() const;
    double getGridPosition() const;
    glm::quat getGridRotation() const;
    
    QVariant getValue() const;
    void detachValue();
    
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    
    void selectedAttributeChanged();
    void createNewAttribute();
    void deleteSelectedAttribute();
    void centerGridPosition();
    void alignGridPosition();
    void updateTool();
    
    void simulate(float deltaTime);
    void render();
    
private:
    
    void addTool(MetavoxelTool* tool);
    void updateAttributes(const QString& select = QString());    
    MetavoxelTool* getActiveTool() const;
    
    QListWidget* _attributes;
    QPushButton* _deleteAttribute;
    
    QComboBox* _gridPlane;
    QDoubleSpinBox* _gridSpacing;
    QDoubleSpinBox* _gridPosition;
    
    QList<MetavoxelTool*> _tools;
    QComboBox* _toolBox;
    
    QGroupBox* _value;
    QScrollArea* _valueArea;
    
    static ProgramObject _gridProgram;
};

/// Base class for editor tools.
class MetavoxelTool : public QWidget {
    Q_OBJECT

public:

    MetavoxelTool(MetavoxelEditor* editor, const QString& name, bool usesValue = true);
    
    bool getUsesValue() const { return _usesValue; }
    
    virtual bool appliesTo(const AttributePointer& attribute) const;
    
    virtual void simulate(float deltaTime);
    
    /// Renders the tool's interface, if any.
    virtual void render();

protected:
    
    MetavoxelEditor* _editor;
    bool _usesValue;
};

/// Allows setting the value of a region by dragging out a box.
class BoxSetTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    BoxSetTool(MetavoxelEditor* editor);

    virtual void render();

    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    
    void resetState();
    void applyValue(const glm::vec3& minimum, const glm::vec3& maximum);
    
    enum State { HOVERING_STATE, DRAGGING_STATE, RAISING_STATE };
    
    State _state;
    
    glm::vec2 _mousePosition; ///< the position of the mouse in rotated space
    glm::vec2 _startPosition; ///< the first corner of the selection base
    glm::vec2 _endPosition; ///< the second corner of the selection base
    float _height; ///< the selection height
};

/// Allows setting the value across the entire space.
class GlobalSetTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    GlobalSetTool(MetavoxelEditor* editor);

private slots:
    
    void apply();
};

/// Allows inserting a spanner into the scene.
class InsertSpannerTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    InsertSpannerTool(MetavoxelEditor* editor);

    virtual void simulate(float deltaTime);

    virtual void render();

    virtual bool appliesTo(const AttributePointer& attribute) const;

    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    
    void insert();
};

/// Allows removing a spanner from the scene.
class RemoveSpannerTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    RemoveSpannerTool(MetavoxelEditor* editor);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;
    
    virtual bool eventFilter(QObject* watched, QEvent* event);
};

/// Allows removing all spanners from the scene.
class ClearSpannersTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    ClearSpannersTool(MetavoxelEditor* editor);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;

private slots:
    
    void clear();
};

#endif /* defined(__interface__MetavoxelEditor__) */
