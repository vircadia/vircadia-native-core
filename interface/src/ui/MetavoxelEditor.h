//
//  MetavoxelEditor.h
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelEditor_h
#define hifi_MetavoxelEditor_h

#include <QList>
#include <QWidget>

#include "MetavoxelSystem.h"
#include "renderer/ProgramObject.h"

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QListWidget;
class QPushButton;
class QScrollArea;
class QSpinBox;

class MetavoxelTool;
class Vec3Editor;

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

/// Base class for insert/set spanner tools.
class PlaceSpannerTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    PlaceSpannerTool(MetavoxelEditor* editor, const QString& name, const QString& placeText);

    virtual void simulate(float deltaTime);

    virtual void render();

    virtual bool appliesTo(const AttributePointer& attribute) const;

    virtual bool eventFilter(QObject* watched, QEvent* event);

protected:

    virtual void applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner) = 0;

private slots:
    
    void place();
};

/// Allows inserting a spanner into the scene.
class InsertSpannerTool : public PlaceSpannerTool {
    Q_OBJECT

public:
    
    InsertSpannerTool(MetavoxelEditor* editor);

protected:

    virtual void applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner);
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

/// Allows setting the value by placing a spanner.
class SetSpannerTool : public PlaceSpannerTool {
    Q_OBJECT

public:
    
    SetSpannerTool(MetavoxelEditor* editor);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;

protected:

    virtual void applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner);
};

/// Base class for heightfield tools.
class HeightfieldTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    HeightfieldTool(MetavoxelEditor* editor, const QString& name);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;

    virtual void render();
    
protected slots:

    virtual void apply() = 0;

protected:
    
    QFormLayout* _form;
    Vec3Editor* _translation;
    QDoubleSpinBox* _scale;
};

/// Allows importing a heightfield.
class ImportHeightfieldTool : public HeightfieldTool {
    Q_OBJECT

public:
    
    ImportHeightfieldTool(MetavoxelEditor* editor);
    
    virtual void render();

protected:

    virtual void apply();

private slots:

    void selectHeightFile();
    void selectColorFile();
    void updatePreview();
    
private:

    QSpinBox* _blockSize;
    
    QPushButton* _height;
    QPushButton* _color;
    
    QImage _heightImage;
    QImage _colorImage;
    
    HeightfieldPreview _preview;
};

// Allows clearing heighfield blocks.
class EraseHeightfieldTool : public HeightfieldTool {
    Q_OBJECT

public:
    
    EraseHeightfieldTool(MetavoxelEditor* editor);
    
    virtual void render();
    
protected:
    
    virtual void apply();

private:
    
    QSpinBox* _width;
    QSpinBox* _length;
};

#endif // hifi_MetavoxelEditor_h
