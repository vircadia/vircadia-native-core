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

class QColorEditor;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QListWidget;
class QPushButton;
class QScrollArea;
class QSpinBox;

class MetavoxelTool;
class SharedObjectEditor;
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
    
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    
    void selectedAttributeChanged();
    void createNewAttribute();
    void deleteSelectedAttribute();
    void centerGridPosition();
    void alignGridPosition();
    void updateAttributes(const QString& select = QString());
    void updateTool();
    
    void simulate(float deltaTime);
    void render();
    void renderPreview();
    
private:
    
    void addTool(MetavoxelTool* tool);
    MetavoxelTool* getActiveTool() const;
    
    QListWidget* _attributes;
    QPushButton* _deleteAttribute;
    QCheckBox* _showAll;
    
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

    MetavoxelTool(MetavoxelEditor* editor, const QString& name, bool usesValue = true, bool userFacing = true);
    
    bool getUsesValue() const { return _usesValue; }
    
    bool isUserFacing() const { return _userFacing; }
    
    virtual bool appliesTo(const AttributePointer& attribute) const;
    
    virtual void simulate(float deltaTime);
    
    /// Renders the tool's interface, if any.
    virtual void render();

    /// Renders the tool's metavoxel preview, if any.
    virtual void renderPreview();

protected:
    
    MetavoxelEditor* _editor;
    bool _usesValue;
    bool _userFacing;
};

/// Base class for tools that allow dragging out a 3D box.
class BoxTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    BoxTool(MetavoxelEditor* editor, const QString& name, bool usesValue = true, bool userFacing = true);
    
    virtual void render();

    virtual bool eventFilter(QObject* watched, QEvent* event);

protected:

    virtual bool shouldSnapToGrid();
    
    virtual QColor getColor() = 0;
    
    virtual void applyValue(const glm::vec3& minimum, const glm::vec3& maximum) = 0; 

private:
    
    void resetState();
    
    enum State { HOVERING_STATE, DRAGGING_STATE, RAISING_STATE };
    
    State _state;
    
    glm::vec2 _mousePosition; ///< the position of the mouse in rotated space
    glm::vec2 _startPosition; ///< the first corner of the selection base
    glm::vec2 _endPosition; ///< the second corner of the selection base
    float _height; ///< the selection height
};

/// Allows setting the value of a region by dragging out a box.
class BoxSetTool : public BoxTool {
    Q_OBJECT

public:
    
    BoxSetTool(MetavoxelEditor* editor);

protected:
    
    virtual QColor getColor();
    
    virtual void applyValue(const glm::vec3& minimum, const glm::vec3& maximum);
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
    
    PlaceSpannerTool(MetavoxelEditor* editor, const QString& name,
        const QString& placeText = QString(), bool usesValue = true);

    virtual void simulate(float deltaTime);

    virtual void renderPreview();

    virtual bool appliesTo(const AttributePointer& attribute) const;

    virtual bool eventFilter(QObject* watched, QEvent* event);

protected:

    virtual QColor getColor();
    virtual SharedObjectPointer getSpanner();
    virtual void applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner) = 0;

protected slots:
    
    void place();

private:
    
    QCheckBox* _followMouse;
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

/// Base class for heightfield tools.
class HeightfieldTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    HeightfieldTool(MetavoxelEditor* editor, const QString& name);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;

protected slots:

    virtual void apply() = 0;

protected:
    
    QFormLayout* _form;
    Vec3Editor* _translation;
    QDoubleSpinBox* _spacing;
};

/// Allows importing a heightfield.
class ImportHeightfieldTool : public HeightfieldTool {
    Q_OBJECT

public:
    
    ImportHeightfieldTool(MetavoxelEditor* editor);
    
    virtual void simulate(float deltaTime);
    
    virtual void renderPreview();
    
protected:

    virtual void apply();

private slots:

    void updateSpanner();

private:

    QDoubleSpinBox* _heightScale;
    QDoubleSpinBox* _heightOffset;
    
    HeightfieldHeightEditor* _height;
    HeightfieldColorEditor* _color;
    
    SharedObjectPointer _spanner;
};

/// Base class for tools that allow painting on heightfields.
class HeightfieldBrushTool : public MetavoxelTool {
    Q_OBJECT

public:
    
    HeightfieldBrushTool(MetavoxelEditor* editor, const QString& name);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;
     
    virtual void render();

    virtual bool eventFilter(QObject* watched, QEvent* event);
    
protected:
    
    virtual QVariant createEdit(bool alternate) = 0;
    
    QFormLayout* _form;
    QDoubleSpinBox* _radius;
    
    glm::vec3 _position;
    bool _positionValid;
};

/// Allows raising or lowering parts of the heightfield.
class HeightfieldHeightBrushTool : public HeightfieldBrushTool {
    Q_OBJECT

public:
    
    HeightfieldHeightBrushTool(MetavoxelEditor* editor);
    
protected:
    
    virtual QVariant createEdit(bool alternate);
    
private:
    
    QDoubleSpinBox* _height;
};

/// Contains widgets for editing materials.
class MaterialControl : public QObject {
    Q_OBJECT

public:

    MaterialControl(QWidget* widget, QFormLayout* form, bool clearable = false);
    
    SharedObjectPointer getMaterial();
    
    const QColor& getColor() const { return _color->getColor(); }
    
private slots:
    
    void clearColor();
    void clearTexture();
    void updateTexture();
    void textureLoaded();
    
private:
    
    QColorEditor* _color;
    SharedObjectEditor* _materialEditor;
    QSharedPointer<NetworkTexture> _texture;
};

/// Allows texturing parts of the heightfield.
class HeightfieldMaterialBrushTool : public HeightfieldBrushTool {
    Q_OBJECT

public:
    
    HeightfieldMaterialBrushTool(MetavoxelEditor* editor);

protected:
    
    virtual QVariant createEdit(bool alternate);
    
private:
    
    MaterialControl* _materialControl;
};

/// Allows sculpting parts of the heightfield.
class HeightfieldSculptBrushTool : public HeightfieldBrushTool {
    Q_OBJECT

public:
    
    HeightfieldSculptBrushTool(MetavoxelEditor* editor);
    
protected:
    
    virtual QVariant createEdit(bool alternate);

private:
    
    MaterialControl* _materialControl;
};

/// Allows setting heightfield materials by dragging out a box.
class HeightfieldMaterialBoxTool : public BoxTool {
    Q_OBJECT

public:
    
    HeightfieldMaterialBoxTool(MetavoxelEditor* editor);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;
    
protected:
    
    virtual bool shouldSnapToGrid();
    
    virtual QColor getColor();
    
    virtual void applyValue(const glm::vec3& minimum, const glm::vec3& maximum);

private:
    
    QCheckBox* _snapToGrid;
    MaterialControl* _materialControl;
};

/// Allows setting heightfield materials by placing a spanner.
class HeightfieldMaterialSpannerTool : public PlaceSpannerTool {
    Q_OBJECT

public:
    
    HeightfieldMaterialSpannerTool(MetavoxelEditor* editor);
    
    virtual bool appliesTo(const AttributePointer& attribute) const;

protected:

    virtual SharedObjectPointer getSpanner();
    virtual QColor getColor();
    virtual void applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner);
    
private:
    
    SharedObjectEditor* _spannerEditor;
    MaterialControl* _materialControl;
};

#endif // hifi_MetavoxelEditor_h
