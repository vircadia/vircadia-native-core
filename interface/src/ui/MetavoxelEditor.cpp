//
//  MetavoxelEditor.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMetaProperty>
#include <QOpenGLFramebufferObject>
#include <QPushButton>
#include <QRunnable>
#include <QScrollArea>
#include <QSpinBox>
#include <QThreadPool>
#include <QVBoxLayout>

#include <AttributeRegistry.h>
#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "Application.h"
#include "MetavoxelEditor.h"

enum GridPlane {
    GRID_PLANE_XY, GRID_PLANE_XZ, GRID_PLANE_YZ
};

const glm::vec2 INVALID_VECTOR(FLT_MAX, FLT_MAX);

MetavoxelEditor::MetavoxelEditor() :
    QWidget(Application::getInstance()->getGLWidget(), Qt::Tool | Qt::WindowStaysOnTopHint) {
    
    setWindowTitle("Metavoxel Editor");
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout* topLayout = new QVBoxLayout();
    setLayout(topLayout);
    
    QGroupBox* attributeGroup = new QGroupBox();
    attributeGroup->setTitle("Attributes");
    topLayout->addWidget(attributeGroup);
    
    QVBoxLayout* attributeLayout = new QVBoxLayout();
    attributeGroup->setLayout(attributeLayout);
    
    attributeLayout->addWidget(_attributes = new QListWidget());
    connect(_attributes, SIGNAL(itemSelectionChanged()), SLOT(selectedAttributeChanged()));

    QHBoxLayout* attributeButtonLayout = new QHBoxLayout();
    attributeLayout->addLayout(attributeButtonLayout);

    QPushButton* newAttribute = new QPushButton("New...");
    attributeButtonLayout->addWidget(newAttribute);
    connect(newAttribute, SIGNAL(clicked()), SLOT(createNewAttribute()));

    attributeButtonLayout->addWidget(_deleteAttribute = new QPushButton("Delete"));
    _deleteAttribute->setEnabled(false);
    connect(_deleteAttribute, SIGNAL(clicked()), SLOT(deleteSelectedAttribute()));

    QFormLayout* formLayout = new QFormLayout();
    topLayout->addLayout(formLayout);
    
    formLayout->addRow("Grid Plane:", _gridPlane = new QComboBox());
    _gridPlane->addItem("X/Y");
    _gridPlane->addItem("X/Z");
    _gridPlane->addItem("Y/Z");
    _gridPlane->setCurrentIndex(GRID_PLANE_XZ);
    connect(_gridPlane, SIGNAL(currentIndexChanged(int)), SLOT(centerGridPosition()));
    
    formLayout->addRow("Grid Spacing:", _gridSpacing = new QDoubleSpinBox());
    _gridSpacing->setMinimum(-FLT_MAX);
    _gridSpacing->setMaximum(FLT_MAX);
    _gridSpacing->setPrefix("2^");
    _gridSpacing->setValue(-3.0);
    connect(_gridSpacing, SIGNAL(valueChanged(double)), SLOT(alignGridPosition()));

    formLayout->addRow("Grid Position:", _gridPosition = new QDoubleSpinBox());
    _gridPosition->setMinimum(-FLT_MAX);
    _gridPosition->setMaximum(FLT_MAX);
    alignGridPosition();
    centerGridPosition();
    
    formLayout->addRow("Tool:", _toolBox = new QComboBox());
    connect(_toolBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTool()));
    
    _value = new QGroupBox();
    _value->setTitle("Value");
    topLayout->addWidget(_value);
    
    QVBoxLayout* valueLayout = new QVBoxLayout();
    _value->setLayout(valueLayout);

    valueLayout->addWidget(_valueArea = new QScrollArea());
    _valueArea->setMinimumHeight(200);
    _valueArea->setWidgetResizable(true);

    addTool(new BoxSetTool(this));
    addTool(new GlobalSetTool(this));
    addTool(new InsertSpannerTool(this));
    addTool(new RemoveSpannerTool(this));
    addTool(new ClearSpannersTool(this));
    addTool(new SetSpannerTool(this));
    addTool(new ImportHeightfieldTool(this));
    addTool(new EraseHeightfieldTool(this));
    addTool(new HeightfieldHeightBrushTool(this));
    addTool(new HeightfieldColorBrushTool(this));
    addTool(new HeightfieldMaterialBrushTool(this));
    
    updateAttributes();
    
    connect(Application::getInstance(), SIGNAL(simulating(float)), SLOT(simulate(float)));
    connect(Application::getInstance(), SIGNAL(renderingInWorldInterface()), SLOT(render()));
    
    Application::getInstance()->getGLWidget()->installEventFilter(this);
    
    show();
    
    if (_gridProgram.isLinked()) {
        return;
    }
        
    _gridProgram.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/grid.frag");
    _gridProgram.link();
}

QString MetavoxelEditor::getSelectedAttribute() const {
    QList<QListWidgetItem*> selectedItems = _attributes->selectedItems();
    return selectedItems.isEmpty() ? QString() : selectedItems.first()->text();
}

double MetavoxelEditor::getGridSpacing() const {
    return pow(2.0, _gridSpacing->value());
}

double MetavoxelEditor::getGridPosition() const {
    return _gridPosition->value();
}

glm::quat MetavoxelEditor::getGridRotation() const {
    // for simplicity, we handle the other two planes by rotating them onto X/Y and performing computation there
    switch (_gridPlane->currentIndex()) {
        case GRID_PLANE_XY:
            return glm::quat();
            
        case GRID_PLANE_XZ:
            return glm::angleAxis(-PI_OVER_TWO, glm::vec3(1.0f, 0.0f, 0.0f));
            
        case GRID_PLANE_YZ:
        default:
            return glm::angleAxis(PI_OVER_TWO, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

QVariant MetavoxelEditor::getValue() const {
    QWidget* editor = _valueArea->widget();
    return editor ? editor->metaObject()->userProperty().read(editor) : QVariant();
}

void MetavoxelEditor::detachValue() {
    SharedObjectEditor* editor = qobject_cast<SharedObjectEditor*>(_valueArea->widget());
    if (editor) {
        editor->detachObject();
    }
}

bool MetavoxelEditor::eventFilter(QObject* watched, QEvent* event) {
    // pass along to the active tool
    MetavoxelTool* tool = getActiveTool();
    return tool && tool->eventFilter(watched, event);
}

void MetavoxelEditor::selectedAttributeChanged() {
    _toolBox->clear();
    
    QString selected = getSelectedAttribute();
    if (selected.isNull()) {
        _deleteAttribute->setEnabled(false);
        _toolBox->setEnabled(false); 
        _value->setVisible(false);
        return;
    }
    _deleteAttribute->setEnabled(true);
    _toolBox->setEnabled(true);
    
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(selected);
    foreach (MetavoxelTool* tool, _tools) {
        if (tool->appliesTo(attribute)) {
            _toolBox->addItem(tool->objectName(), QVariant::fromValue(tool));
        }
    }
    _value->setVisible(true);
    
    if (_valueArea->widget()) {
        delete _valueArea->widget();
    }
    QWidget* editor = attribute->createEditor();
    if (editor) {
        editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        _valueArea->setWidget(editor);
    }
}

void MetavoxelEditor::createNewAttribute() {
    QDialog dialog(this);
    dialog.setWindowTitle("New Attribute");
    
    QVBoxLayout layout;
    dialog.setLayout(&layout);
    
    QFormLayout form;
    layout.addLayout(&form);
    
    QLineEdit name;
    form.addRow("Name:", &name);
    
    SharedObjectEditor editor(&Attribute::staticMetaObject, false);
    editor.setObject(new QRgbAttribute());
    layout.addWidget(&editor);
    
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.connect(&buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(&buttons, SIGNAL(rejected()), SLOT(reject()));
    
    layout.addWidget(&buttons);
    
    if (!dialog.exec()) {
        return;
    }
    QString nameText = name.text().trimmed();
    SharedObjectPointer attribute = editor.getObject();
    attribute->setObjectName(nameText);
    AttributeRegistry::getInstance()->registerAttribute(attribute.staticCast<Attribute>());
    
    updateAttributes(nameText);
}

void MetavoxelEditor::deleteSelectedAttribute() {
    AttributeRegistry::getInstance()->deregisterAttribute(getSelectedAttribute());
    _attributes->selectionModel()->clear();
    updateAttributes();
}

void MetavoxelEditor::centerGridPosition() {
    const float CENTER_OFFSET = 0.625f;
    float eyePosition = (glm::inverse(getGridRotation()) * Application::getInstance()->getCamera()->getPosition()).z -
        Application::getInstance()->getAvatar()->getScale() * CENTER_OFFSET;
    double step = getGridSpacing();
    _gridPosition->setValue(step * floor(eyePosition / step));
}

void MetavoxelEditor::alignGridPosition() {
    // make sure our grid position matches our grid spacing
    double step = getGridSpacing();
    _gridPosition->setSingleStep(step);
    _gridPosition->setValue(step * floor(_gridPosition->value() / step));
}

void MetavoxelEditor::updateTool() {
    MetavoxelTool* active = getActiveTool();
    foreach (MetavoxelTool* tool, _tools) {
        tool->setVisible(tool == active);
    }
    _value->setVisible(active && active->getUsesValue());
}

void MetavoxelEditor::simulate(float deltaTime) {
    MetavoxelTool* tool = getActiveTool();
    if (tool) {
        tool->simulate(deltaTime);
    }
}

const float GRID_BRIGHTNESS = 0.5f;

void MetavoxelEditor::render() {
    glDisable(GL_LIGHTING);
    
    MetavoxelTool* tool = getActiveTool();
    if (tool) {
        tool->render();
    }
    
    glDepthMask(GL_FALSE);
    
    glPushMatrix();
    
    glm::quat rotation = getGridRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    
    glLineWidth(1.0f);
    
    // center the grid around the camera position on the plane
    glm::vec3 rotated = glm::inverse(rotation) * Application::getInstance()->getCamera()->getPosition();
    float spacing = getGridSpacing();
    const int GRID_DIVISIONS = 300;
    glTranslatef(spacing * (floorf(rotated.x / spacing) - GRID_DIVISIONS / 2),
        spacing * (floorf(rotated.y / spacing) - GRID_DIVISIONS / 2), _gridPosition->value());
    
    float scale = GRID_DIVISIONS * spacing;
    glScalef(scale, scale, scale);
    
    glColor3f(GRID_BRIGHTNESS, GRID_BRIGHTNESS, GRID_BRIGHTNESS);
    
    _gridProgram.bind();
    
    Application::getInstance()->getGeometryCache()->renderGrid(GRID_DIVISIONS, GRID_DIVISIONS);
    
    _gridProgram.release();
    
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
}

void MetavoxelEditor::addTool(MetavoxelTool* tool) {
    _tools.append(tool);
    layout()->addWidget(tool);
}

void MetavoxelEditor::updateAttributes(const QString& select) {
    // remember the selection in order to preserve it
    QString selected = select.isNull() ? getSelectedAttribute() : select;
    _attributes->clear();
    
    // sort the names for consistent ordering
    QList<QString> names = AttributeRegistry::getInstance()->getAttributes().keys();
    qSort(names);
    
    foreach (const QString& name, names) {
        QListWidgetItem* item = new QListWidgetItem(name);
        _attributes->addItem(item);
        if (name == selected || selected.isNull()) {
            item->setSelected(true);
            selected = name;
        }
    }
}

MetavoxelTool* MetavoxelEditor::getActiveTool() const {
    int index = _toolBox->currentIndex();
    return (index == -1) ? NULL : static_cast<MetavoxelTool*>(_toolBox->itemData(index).value<QObject*>());
}

ProgramObject MetavoxelEditor::_gridProgram;

MetavoxelTool::MetavoxelTool(MetavoxelEditor* editor, const QString& name, bool usesValue) :
    _editor(editor),
    _usesValue(usesValue) {
    
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    setObjectName(name);
    setVisible(false);
}

bool MetavoxelTool::appliesTo(const AttributePointer& attribute) const {
    // shared object sets are a special case
    return !attribute->inherits("SharedObjectSetAttribute");
}

void MetavoxelTool::simulate(float deltaTime) {
    // nothing by default
}

void MetavoxelTool::render() {
    // nothing by default
}

BoxSetTool::BoxSetTool(MetavoxelEditor* editor) :
    MetavoxelTool(editor, "Set Value (Box)") {
    
    resetState();
}

void BoxSetTool::render() {
    if (Application::getInstance()->isMouseHidden()) {
        resetState();
        return;
    }
    QString selected = _editor->getSelectedAttribute();
    if (selected.isNull()) {
        resetState();
        return;
    }
    glDepthMask(GL_FALSE);
    
    glPushMatrix();
    
    glm::quat rotation = _editor->getGridRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
    
    glm::quat inverseRotation = glm::inverse(rotation);
    glm::vec3 rayOrigin = inverseRotation * Application::getInstance()->getMouseRayOrigin();
    glm::vec3 rayDirection = inverseRotation * Application::getInstance()->getMouseRayDirection();
    float spacing = _editor->getGridSpacing();
    float position = _editor->getGridPosition();
    if (_state == RAISING_STATE) {
        // find the plane at the mouse position, orthogonal to the plane, facing the eye position
        glLineWidth(4.0f);  
        glm::vec3 eyePosition = inverseRotation * Application::getInstance()->getViewFrustum()->getOffsetPosition();
        glm::vec3 mousePoint = glm::vec3(_mousePosition, position);
        glm::vec3 right = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), eyePosition - mousePoint);
        glm::vec3 normal = glm::cross(right, glm::vec3(0.0f, 0.0f, 1.0f));
        float divisor = glm::dot(normal, rayDirection);
        if (fabs(divisor) > EPSILON) {
            float distance = (glm::dot(normal, mousePoint) - glm::dot(normal, rayOrigin)) / divisor;
            float projection = rayOrigin.z + distance * rayDirection.z;
            _height = spacing * roundf(projection / spacing) - position;
        }
    } else if (fabs(rayDirection.z) > EPSILON) {
        // find the intersection of the rotated mouse ray with the plane
        float distance = (position - rayOrigin.z) / rayDirection.z;
        _mousePosition = glm::vec2(rayOrigin + rayDirection * distance);
        glm::vec2 snappedPosition = spacing * glm::floor(_mousePosition / spacing);
        
        if (_state == HOVERING_STATE) {
            _startPosition = _endPosition = snappedPosition;
            glLineWidth(2.0f);
            
        } else if (_state == DRAGGING_STATE) {
            _endPosition = snappedPosition;
            glLineWidth(4.0f);
        }
    } else {
        // cancel any operation in progress
        resetState();
    }
    
    if (_startPosition != INVALID_VECTOR) {   
        glm::vec2 minimum = glm::min(_startPosition, _endPosition);
        glm::vec2 maximum = glm::max(_startPosition, _endPosition);
    
        glPushMatrix();
        glTranslatef(minimum.x, minimum.y, position);
        glScalef(maximum.x + spacing - minimum.x, maximum.y + spacing - minimum.y, _height);
    
        glTranslatef(0.5f, 0.5f, 0.5f);
        if (_state != HOVERING_STATE) {
            const float BOX_ALPHA = 0.25f;
            QColor color = _editor->getValue().value<QColor>();
            if (color.isValid()) {
                glColor4f(color.redF(), color.greenF(), color.blueF(), BOX_ALPHA);
            } else {
                glColor4f(GRID_BRIGHTNESS, GRID_BRIGHTNESS, GRID_BRIGHTNESS, BOX_ALPHA);
            }
            glEnable(GL_CULL_FACE);
            glutSolidCube(1.0);
            glDisable(GL_CULL_FACE);
        }
        glColor3f(GRID_BRIGHTNESS, GRID_BRIGHTNESS, GRID_BRIGHTNESS);
        glutWireCube(1.0);
    
        glPopMatrix();   
    }
    
    glPopMatrix();
}

bool BoxSetTool::eventFilter(QObject* watched, QEvent* event) {
    switch (_state) {
        case HOVERING_STATE:
            if (event->type() == QEvent::MouseButtonPress && _startPosition != INVALID_VECTOR) {
                _state = DRAGGING_STATE;
                return true;
            }
            break;
            
        case DRAGGING_STATE:
            if (event->type() == QEvent::MouseButtonRelease) {
                _state = RAISING_STATE;
                return true;
            }
            break;
            
        case RAISING_STATE:
            if (event->type() == QEvent::MouseButtonPress) {
                if (_height != 0) {
                    // find the start and end corners in X/Y
                    float base = _editor->getGridPosition();
                    float top = base + _height;
                    glm::quat rotation = _editor->getGridRotation();
                    glm::vec3 start = rotation * glm::vec3(glm::min(_startPosition, _endPosition), glm::min(base, top));
                    float spacing = _editor->getGridSpacing();
                    glm::vec3 end = rotation * glm::vec3(glm::max(_startPosition, _endPosition) +
                        glm::vec2(spacing, spacing), glm::max(base, top));
                    
                    // find the minimum and maximum extents after rotation
                    applyValue(glm::min(start, end), glm::max(start, end));
                }
                resetState();
                return true;
            }
            break;
    }
    return false;
}

void BoxSetTool::resetState() {
    _state = HOVERING_STATE;
    _startPosition = INVALID_VECTOR;
    _height = 0.0f;
}

void BoxSetTool::applyValue(const glm::vec3& minimum, const glm::vec3& maximum) {
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(_editor->getSelectedAttribute());
    if (!attribute) {
        return;
    }
    OwnedAttributeValue value(attribute, attribute->createFromVariant(_editor->getValue()));
    MetavoxelEditMessage message = { QVariant::fromValue(BoxSetEdit(Box(minimum, maximum),
        _editor->getGridSpacing(), value)) };
    Application::getInstance()->getMetavoxels()->applyEdit(message);
}

GlobalSetTool::GlobalSetTool(MetavoxelEditor* editor) :
    MetavoxelTool(editor, "Set Value (Global)") {
    
    QPushButton* button = new QPushButton("Apply");
    layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(apply()));
}

void GlobalSetTool::apply() {
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(_editor->getSelectedAttribute());
    if (!attribute) {
        return;
    }
    OwnedAttributeValue value(attribute, attribute->createFromVariant(_editor->getValue()));
    MetavoxelEditMessage message = { QVariant::fromValue(GlobalSetEdit(value)) };
    Application::getInstance()->getMetavoxels()->applyEdit(message);
}

PlaceSpannerTool::PlaceSpannerTool(MetavoxelEditor* editor, const QString& name, const QString& placeText) :
    MetavoxelTool(editor, name) {
    
    QPushButton* button = new QPushButton(placeText);
    layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(place()));
}

void PlaceSpannerTool::simulate(float deltaTime) {
    if (Application::getInstance()->isMouseHidden()) {
        return;
    }
    _editor->detachValue();
    Spanner* spanner = static_cast<Spanner*>(_editor->getValue().value<SharedObjectPointer>().data());
    Transformable* transformable = qobject_cast<Transformable*>(spanner);
    if (transformable) {
        // find the intersection of the mouse ray with the grid and place the transformable there
        glm::quat rotation = _editor->getGridRotation();
        glm::quat inverseRotation = glm::inverse(rotation);
        glm::vec3 rayOrigin = inverseRotation * Application::getInstance()->getMouseRayOrigin();
        glm::vec3 rayDirection = inverseRotation * Application::getInstance()->getMouseRayDirection();
        float position = _editor->getGridPosition();
        float distance = (position - rayOrigin.z) / rayDirection.z;
        
        transformable->setTranslation(rotation * glm::vec3(glm::vec2(rayOrigin + rayDirection * distance), position));
    }
    spanner->getRenderer()->simulate(deltaTime);
}

void PlaceSpannerTool::render() {
    if (Application::getInstance()->isMouseHidden()) {
        return;
    }
    Spanner* spanner = static_cast<Spanner*>(_editor->getValue().value<SharedObjectPointer>().data());
    const float SPANNER_ALPHA = 0.25f;
    spanner->getRenderer()->render(SPANNER_ALPHA, SpannerRenderer::DEFAULT_MODE, glm::vec3(), 0.0f);
}

bool PlaceSpannerTool::appliesTo(const AttributePointer& attribute) const {
    return attribute->inherits("SpannerSetAttribute");
}

bool PlaceSpannerTool::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        place();
        return true;
    }
    return false;
}

void PlaceSpannerTool::place() {
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(_editor->getSelectedAttribute());
    if (attribute) {
        applyEdit(attribute, _editor->getValue().value<SharedObjectPointer>());
    }
}

InsertSpannerTool::InsertSpannerTool(MetavoxelEditor* editor) :
    PlaceSpannerTool(editor, "Insert Spanner", "Insert") {
}

void InsertSpannerTool::applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner) {
    MetavoxelEditMessage message = { QVariant::fromValue(InsertSpannerEdit(attribute, spanner)) };
    Application::getInstance()->getMetavoxels()->applyEdit(message);
}

RemoveSpannerTool::RemoveSpannerTool(MetavoxelEditor* editor) :
    MetavoxelTool(editor, "Remove Spanner", false) {
}

bool RemoveSpannerTool::appliesTo(const AttributePointer& attribute) const {
    return attribute->inherits("SpannerSetAttribute");
}

bool RemoveSpannerTool::eventFilter(QObject* watched, QEvent* event) {
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(_editor->getSelectedAttribute());
    if (!attribute) {
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        float distance;
        SharedObjectPointer spanner = Application::getInstance()->getMetavoxels()->findFirstRaySpannerIntersection(
            Application::getInstance()->getMouseRayOrigin(), Application::getInstance()->getMouseRayDirection(),
            attribute, distance);
        if (spanner) {
            MetavoxelEditMessage message = { QVariant::fromValue(RemoveSpannerEdit(attribute, spanner->getRemoteID())) };
            Application::getInstance()->getMetavoxels()->applyEdit(message);
        }
        return true;
    }
    return false;
}

ClearSpannersTool::ClearSpannersTool(MetavoxelEditor* editor) :
    MetavoxelTool(editor, "Clear Spanners", false) {
    
    QPushButton* button = new QPushButton("Clear");
    layout()->addWidget(button);
    connect(button, SIGNAL(clicked()), SLOT(clear()));
}

bool ClearSpannersTool::appliesTo(const AttributePointer& attribute) const {
    return attribute->inherits("SpannerSetAttribute");
}

void ClearSpannersTool::clear() {
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(_editor->getSelectedAttribute());
    if (!attribute) {
        return;
    }
    MetavoxelEditMessage message = { QVariant::fromValue(ClearSpannersEdit(attribute)) };
    Application::getInstance()->getMetavoxels()->applyEdit(message);
}

SetSpannerTool::SetSpannerTool(MetavoxelEditor* editor) :
    PlaceSpannerTool(editor, "Set Spanner", "Set") {
}

bool SetSpannerTool::appliesTo(const AttributePointer& attribute) const {
    return attribute == AttributeRegistry::getInstance()->getSpannersAttribute();
}

glm::quat DIRECTION_ROTATIONS[] = {
    rotationBetween(glm::vec3(-1.0f, 0.0f, 0.0f), IDENTITY_FRONT),
    rotationBetween(glm::vec3(1.0f, 0.0f, 0.0f), IDENTITY_FRONT),
    rotationBetween(glm::vec3(0.0f, -1.0f, 0.0f), IDENTITY_FRONT),
    rotationBetween(glm::vec3(0.0f, 1.0f, 0.0f), IDENTITY_FRONT),
    rotationBetween(glm::vec3(0.0f, 0.0f, -1.0f), IDENTITY_FRONT),
    rotationBetween(glm::vec3(0.0f, 0.0f, 1.0f), IDENTITY_FRONT) };

/// Represents a view from one direction of the spanner to be voxelized.
class DirectionImages {
public:
    QImage color;
    QVector<float> depth;
    glm::vec3 minima;
    glm::vec3 maxima;
    glm::vec3 scale;
};

class Voxelizer : public QRunnable {
public:

    Voxelizer(float size, const Box& bounds, float granularity, const QVector<DirectionImages>& directionImages);
    
    virtual void run();

private:
    
    void voxelize(const glm::vec3& center);
    
    float _size;
    Box _bounds;
    float _granularity;
    QVector<DirectionImages> _directionImages;
};

Voxelizer::Voxelizer(float size, const Box& bounds, float granularity, const QVector<DirectionImages>& directionImages) :
    _size(size),
    _bounds(bounds),
    _granularity(granularity),
    _directionImages(directionImages) {
}

void Voxelizer::run() {
    // voxelize separately each cell within the bounds
    float halfSize = _size * 0.5f;
    for (float x = _bounds.minimum.x + halfSize; x < _bounds.maximum.x; x += _size) {
        for (float y = _bounds.minimum.y + halfSize; y < _bounds.maximum.y; y += _size) {
            for (float z = _bounds.minimum.z + halfSize; z < _bounds.maximum.z; z += _size) {
                voxelize(glm::vec3(x, y, z));
            }
        }
    }
}

class VoxelizationVisitor : public MetavoxelVisitor {
public:
    
    VoxelizationVisitor(const QVector<DirectionImages>& directionImages, const glm::vec3& center, float granularity);
    
    virtual int visit(MetavoxelInfo& info);

private:
    
    QVector<DirectionImages> _directionImages;
    glm::vec3 _center;
    float _granularity;
};

VoxelizationVisitor::VoxelizationVisitor(const QVector<DirectionImages>& directionImages,
        const glm::vec3& center, float granularity) :
    MetavoxelVisitor(QVector<AttributePointer>(), QVector<AttributePointer>() <<
        AttributeRegistry::getInstance()->getColorAttribute()),
    _directionImages(directionImages),
    _center(center),
    _granularity(granularity) {
}

bool checkDisjoint(const DirectionImages& images, const glm::vec3& minimum, const glm::vec3& maximum, float extent) {
    for (int x = qMax(0, (int)minimum.x), xmax = qMin(images.color.width(), (int)maximum.x); x < xmax; x++) {
        for (int y = qMax(0, (int)minimum.y), ymax = qMin(images.color.height(), (int)maximum.y); y < ymax; y++) {
            float depth = 1.0f - images.depth.at(y * images.color.width() + x);
            if (depth - minimum.z >= -extent - EPSILON) {
                return false;
            }
        }
    }
    return true;
}

int VoxelizationVisitor::visit(MetavoxelInfo& info) {
    float halfSize = info.size * 0.5f;
    glm::vec3 center = info.minimum + _center + glm::vec3(halfSize, halfSize, halfSize);
    const float EXTENT_SCALE = 2.0f;
    if (info.size > _granularity) {    
        for (unsigned int i = 0; i < sizeof(DIRECTION_ROTATIONS) / sizeof(DIRECTION_ROTATIONS[0]); i++) {
            glm::vec3 rotated = DIRECTION_ROTATIONS[i] * center;
            const DirectionImages& images = _directionImages.at(i);
            glm::vec3 relative = (rotated - images.minima) * images.scale;
            glm::vec3 extents = images.scale * halfSize;
            glm::vec3 minimum = relative - extents;
            glm::vec3 maximum = relative + extents;
            if (checkDisjoint(images, minimum, maximum, extents.z * EXTENT_SCALE)) {
                info.outputValues[0] = AttributeValue(_outputs.at(0));
                return STOP_RECURSION;
            }
        }
        return DEFAULT_ORDER;
    }
    QRgb closestColor = QRgb();
    float closestDistance = FLT_MAX;
    for (unsigned int i = 0; i < sizeof(DIRECTION_ROTATIONS) / sizeof(DIRECTION_ROTATIONS[0]); i++) {
        glm::vec3 rotated = DIRECTION_ROTATIONS[i] * center;
        const DirectionImages& images = _directionImages.at(i);
        glm::vec3 relative = (rotated - images.minima) * images.scale;
        int x = qMax(qMin((int)glm::round(relative.x), images.color.width() - 1), 0);
        int y = qMax(qMin((int)glm::round(relative.y), images.color.height() - 1), 0);
        float depth = 1.0f - images.depth.at(y * images.color.width() + x);
        float distance = depth - relative.z;
        float extent = images.scale.z * halfSize * EXTENT_SCALE;
        if (distance < -extent - EPSILON) {
            info.outputValues[0] = AttributeValue(_outputs.at(0));
            return STOP_RECURSION;
        }
        QRgb color = images.color.pixel(x, y);
        if (distance < extent + EPSILON) {
            info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline<QRgb>(color));
            return STOP_RECURSION;
        }
        if (distance < closestDistance) {
            closestColor = color;
            closestDistance = distance;
        }
    }
    info.outputValues[0] = AttributeValue(_outputs.at(0), encodeInline<QRgb>(closestColor));
    return STOP_RECURSION;
}

void Voxelizer::voxelize(const glm::vec3& center) {
    MetavoxelData data;
    data.setSize(_size);
    
    VoxelizationVisitor visitor(_directionImages, center, _granularity);
    data.guide(visitor);
    
    MetavoxelEditMessage edit = { QVariant::fromValue(SetDataEdit(
        center - glm::vec3(_size, _size, _size) * 0.5f, data, true)) };
    QMetaObject::invokeMethod(Application::getInstance()->getMetavoxels(), "applyEdit",
        Q_ARG(const MetavoxelEditMessage&, edit), Q_ARG(bool, true));
}

void SetSpannerTool::applyEdit(const AttributePointer& attribute, const SharedObjectPointer& spanner) {
    Spanner* spannerData = static_cast<Spanner*>(spanner.data());
    Box bounds = spannerData->getBounds();
    float longestSide(qMax(bounds.getLongestSide(), spannerData->getPlacementGranularity()));
    float size = powf(2.0f, floorf(logf(longestSide) / logf(2.0f)));
    Box cellBounds(glm::floor(bounds.minimum / size) * size, glm::ceil(bounds.maximum / size) * size);
    
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
    
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
        
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    QVector<DirectionImages> directionImages;
    
    for (unsigned int i = 0; i < sizeof(DIRECTION_ROTATIONS) / sizeof(DIRECTION_ROTATIONS[0]); i++) {
        glm::vec3 minima(FLT_MAX, FLT_MAX, FLT_MAX);
        glm::vec3 maxima(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        for (int j = 0; j < Box::VERTEX_COUNT; j++) {
            glm::vec3 rotated = DIRECTION_ROTATIONS[i] * cellBounds.getVertex(j);
            minima = glm::min(minima, rotated);
            maxima = glm::max(maxima, rotated);
        }
        float renderGranularity = spannerData->getVoxelizationGranularity() / 4.0f;
        int width = glm::round((maxima.x - minima.x) / renderGranularity);
        int height = glm::round((maxima.y - minima.y) / renderGranularity);
        
        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glLoadIdentity();
        glOrtho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z);
        
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glm::vec3 axis = glm::axis(DIRECTION_ROTATIONS[i]);
        glRotatef(glm::degrees(glm::angle(DIRECTION_ROTATIONS[i])), axis.x, axis.y, axis.z);
        
        Application::getInstance()->setupWorldLight();
        
        Application::getInstance()->updateUntranslatedViewMatrix();
        
        spannerData->getRenderer()->render(1.0f, SpannerRenderer::DIFFUSE_MODE, glm::vec3(), 0.0f);
        
        DirectionImages images = { QImage(width, height, QImage::Format_ARGB32),
            QVector<float>(width * height), minima, maxima, glm::vec3(width / (maxima.x - minima.x),
                height / (maxima.y - minima.y), 1.0f / (maxima.z - minima.z)) };
        glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, images.color.bits());
        glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, images.depth.data());
        directionImages.append(images);
        
        glMatrixMode(GL_PROJECTION);
    }
    glPopMatrix();
 
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glEnable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->release();

    glViewport(0, 0, Application::getInstance()->getGLWidget()->width(), Application::getInstance()->getGLWidget()->height());
    
    // send the images off to the lab for processing
    QThreadPool::globalInstance()->start(new Voxelizer(size, cellBounds,
        spannerData->getVoxelizationGranularity(), directionImages));
}

HeightfieldTool::HeightfieldTool(MetavoxelEditor* editor, const QString& name) :
    MetavoxelTool(editor, name, false) {
    
    QWidget* widget = new QWidget();
    widget->setLayout(_form = new QFormLayout());
    layout()->addWidget(widget);
    
    _form->addRow("Translation:", _translation = new Vec3Editor(widget));
    _form->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setMinimum(-FLT_MAX);
    _scale->setMaximum(FLT_MAX);
    _scale->setPrefix("2^");
    _scale->setValue(3.0);
    
    QPushButton* applyButton = new QPushButton("Apply");
    layout()->addWidget(applyButton);
    connect(applyButton, &QAbstractButton::clicked, this, &HeightfieldTool::apply);
}

bool HeightfieldTool::appliesTo(const AttributePointer& attribute) const {
    return attribute->inherits("HeightfieldAttribute");
}

void HeightfieldTool::render() {
    float scale = pow(2.0, _scale->value());
    _translation->setSingleStep(scale);
    glm::vec3 quantizedTranslation = scale * glm::floor(_translation->getValue() / scale);
    _translation->setValue(quantizedTranslation);
}

ImportHeightfieldTool::ImportHeightfieldTool(MetavoxelEditor* editor) :
    HeightfieldTool(editor, "Import Heightfield") {
    
    _form->addRow("Block Size:", _blockSize = new QSpinBox());
    _blockSize->setPrefix("2^");
    _blockSize->setMinimum(1);
    _blockSize->setValue(5);
    
    connect(_blockSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
        &ImportHeightfieldTool::updatePreview);
    _form->addRow("Height:", _height = new QPushButton());
    connect(_height, &QAbstractButton::clicked, this, &ImportHeightfieldTool::selectHeightFile);
    _form->addRow("Color:", _color = new QPushButton());
    connect(_color, &QAbstractButton::clicked, this, &ImportHeightfieldTool::selectColorFile);
}

void ImportHeightfieldTool::render() {
    HeightfieldTool::render();
    _preview.render(_translation->getValue(), _translation->getSingleStep());
}

void ImportHeightfieldTool::apply() {
    float scale = _translation->getSingleStep();
    foreach (const BufferDataPointer& bufferData, _preview.getBuffers()) {
        HeightfieldBuffer* buffer = static_cast<HeightfieldBuffer*>(bufferData.data());
        MetavoxelData data;
        data.setSize(scale);
        
        QByteArray height = buffer->getUnextendedHeight();
        HeightfieldHeightDataPointer heightPointer(new HeightfieldHeightData(height));
        data.setRoot(AttributeRegistry::getInstance()->getHeightfieldAttribute(), new MetavoxelNode(AttributeValue(
            AttributeRegistry::getInstance()->getHeightfieldAttribute(), encodeInline(heightPointer))));
        
        QByteArray color;
        if (buffer->getColor().isEmpty()) {
            const int WHITE_VALUE = 0xFF;
            color = QByteArray(height.size() * DataBlock::COLOR_BYTES, WHITE_VALUE);
        } else {
            color = buffer->getUnextendedColor();
        }
        HeightfieldColorDataPointer colorPointer(new HeightfieldColorData(color));
        data.setRoot(AttributeRegistry::getInstance()->getHeightfieldColorAttribute(), new MetavoxelNode(AttributeValue(
            AttributeRegistry::getInstance()->getHeightfieldColorAttribute(), encodeInline(colorPointer))));
        
        int size = glm::sqrt(height.size()) + HeightfieldBuffer::SHARED_EDGE; 
        QByteArray material(size * size, 0);
        HeightfieldMaterialDataPointer materialPointer(new HeightfieldMaterialData(material));
        data.setRoot(AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute(), new MetavoxelNode(AttributeValue(
            AttributeRegistry::getInstance()->getHeightfieldMaterialAttribute(), encodeInline(materialPointer))));
        
        MetavoxelEditMessage message = { QVariant::fromValue(SetDataEdit(
            _translation->getValue() + buffer->getTranslation() * scale, data)) };
        Application::getInstance()->getMetavoxels()->applyEdit(message, true);
    }
}

void ImportHeightfieldTool::selectHeightFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Select Height Image", QString(), "Images (*.png *.jpg)");
    if (filename.isNull()) {
        return;
    }
    if (!_heightImage.load(filename)) {
        QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
        return;
    }
    _heightImage = _heightImage.convertToFormat(QImage::Format_RGB888);
    _height->setText(filename);
    updatePreview();
}

void ImportHeightfieldTool::selectColorFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Select Color Image", QString(), "Images (*.png *.jpg)");
    if (filename.isNull()) {
        return;
    }
    if (!_colorImage.load(filename)) {
        QMessageBox::warning(this, "Invalid Image", "The selected image could not be read.");
        return;
    }
    _colorImage = _colorImage.convertToFormat(QImage::Format_RGB888);
    _color->setText(filename);
    updatePreview();
}

void ImportHeightfieldTool::updatePreview() {
    QVector<BufferDataPointer> buffers;
    if (_heightImage.width() > 0 && _heightImage.height() > 0) {
        float z = 0.0f;
        int blockSize = pow(2.0, _blockSize->value());
        int heightSize = blockSize + HeightfieldBuffer::HEIGHT_EXTENSION;
        int colorSize = blockSize + HeightfieldBuffer::SHARED_EDGE;
        for (int i = 0; i < _heightImage.height(); i += blockSize, z++) {
            float x = 0.0f;
            for (int j = 0; j < _heightImage.width(); j += blockSize, x++) {
                QByteArray height(heightSize * heightSize, 0);
                int extendedI = qMax(i - HeightfieldBuffer::HEIGHT_BORDER, 0);
                int extendedJ = qMax(j - HeightfieldBuffer::HEIGHT_BORDER, 0);
                int offsetY = extendedI - i + HeightfieldBuffer::HEIGHT_BORDER;
                int offsetX = extendedJ - j + HeightfieldBuffer::HEIGHT_BORDER;
                int rows = qMin(heightSize - offsetY, _heightImage.height() - extendedI);
                int columns = qMin(heightSize - offsetX, _heightImage.width() - extendedJ);
                for (int y = 0; y < rows; y++) {
                    uchar* src = _heightImage.scanLine(extendedI + y) + extendedJ * DataBlock::COLOR_BYTES;
                    char* dest = height.data() + (y + offsetY) * heightSize + offsetX;
                    for (int x = 0; x < columns; x++) {
                        *dest++ = *src;
                        src += DataBlock::COLOR_BYTES;
                    }
                }
                QByteArray color;
                if (!_colorImage.isNull()) {
                    color = QByteArray(colorSize * colorSize * DataBlock::COLOR_BYTES, 0);
                    rows = qMax(0, qMin(colorSize, _colorImage.height() - i));
                    columns = qMax(0, qMin(colorSize, _colorImage.width() - j));
                    for (int y = 0; y < rows; y++) {
                        memcpy(color.data() + y * colorSize * DataBlock::COLOR_BYTES,
                            _colorImage.scanLine(i + y) + j * DataBlock::COLOR_BYTES,
                            columns * DataBlock::COLOR_BYTES);
                    }
                }
                buffers.append(BufferDataPointer(new HeightfieldBuffer(glm::vec3(x, 0.0f, z), 1.0f, height, color)));
            }
        }
    }
    _preview.setBuffers(buffers);
}

EraseHeightfieldTool::EraseHeightfieldTool(MetavoxelEditor* editor) :
    HeightfieldTool(editor, "Erase Heightfield") {
    
    _form->addRow("Width:", _width = new QSpinBox());
    _width->setMinimum(1);
    _width->setMaximum(INT_MAX);
    _form->addRow("Length:", _length = new QSpinBox());
    _length->setMinimum(1);
    _length->setMaximum(INT_MAX);
}

void EraseHeightfieldTool::render() {
    HeightfieldTool::render();
    
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(4.0f);
    
    glPushMatrix();
    glm::vec3 translation = _translation->getValue();
    glTranslatef(translation.x, translation.y, translation.z);
    float scale = _translation->getSingleStep();
    glScalef(scale * _width->value(), scale, scale * _length->value());
    glTranslatef(0.5f, 0.5f, 0.5f);
    
    glutWireCube(1.0);
    
    glPopMatrix();
    
    glLineWidth(1.0f);
}

void EraseHeightfieldTool::apply() {
    // clear the heightfield
    float scale = _translation->getSingleStep();
    BoxSetEdit edit(Box(_translation->getValue(), _translation->getValue() +
        glm::vec3(_width->value() * scale, scale, _length->value() * scale)), scale,
        OwnedAttributeValue(AttributeRegistry::getInstance()->getHeightfieldAttribute()));
    MetavoxelEditMessage message = { QVariant::fromValue(edit) };
    Application::getInstance()->getMetavoxels()->applyEdit(message, true);
    
    // and the color
    edit.value = OwnedAttributeValue(AttributeRegistry::getInstance()->getHeightfieldColorAttribute());
    message.edit = QVariant::fromValue(edit);
    Application::getInstance()->getMetavoxels()->applyEdit(message, true);
}

HeightfieldBrushTool::HeightfieldBrushTool(MetavoxelEditor* editor, const QString& name) :
    MetavoxelTool(editor, name, false) {
    
    QWidget* widget = new QWidget();
    widget->setLayout(_form = new QFormLayout());
    layout()->addWidget(widget);
    
    _form->addRow("Radius:", _radius = new QDoubleSpinBox());
    _radius->setSingleStep(0.01);
    _radius->setMaximum(FLT_MAX);
    _radius->setValue(1.0);
}

bool HeightfieldBrushTool::appliesTo(const AttributePointer& attribute) const {
    return attribute->inherits("HeightfieldAttribute");
}

void HeightfieldBrushTool::render() {
    if (Application::getInstance()->isMouseHidden()) {
        return;
    }
    
    // find the intersection with the heightfield
    glm::vec3 origin = Application::getInstance()->getMouseRayOrigin();
    glm::vec3 direction = Application::getInstance()->getMouseRayDirection();
    
    float distance;
    if (!Application::getInstance()->getMetavoxels()->findFirstRayHeightfieldIntersection(origin, direction, distance)) {
        return;
    }
    Application::getInstance()->getMetavoxels()->renderHeightfieldCursor(
        _position = origin + distance * direction, _radius->value());
}

bool HeightfieldBrushTool::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Wheel) {
        float angle = static_cast<QWheelEvent*>(event)->angleDelta().y();
        const float ANGLE_SCALE = 1.0f / 1000.0f;
        _radius->setValue(_radius->value() * glm::pow(2.0f, angle * ANGLE_SCALE));
        return true;
    
    } else if (event->type() == QEvent::MouseButtonPress) {
        MetavoxelEditMessage message = { createEdit(static_cast<QMouseEvent*>(event)->button() == Qt::RightButton) };
        Application::getInstance()->getMetavoxels()->applyEdit(message, true);
        return true;
    }
    return false;
}

HeightfieldHeightBrushTool::HeightfieldHeightBrushTool(MetavoxelEditor* editor) :
    HeightfieldBrushTool(editor, "Height Brush") {
    
    _form->addRow("Height:", _height = new QDoubleSpinBox());
    _height->setMinimum(-FLT_MAX);
    _height->setMaximum(FLT_MAX);
    _height->setValue(1.0);
}

QVariant HeightfieldHeightBrushTool::createEdit(bool alternate) {
    return QVariant::fromValue(PaintHeightfieldHeightEdit(_position, _radius->value(),
        alternate ? -_height->value() : _height->value()));
}

HeightfieldColorBrushTool::HeightfieldColorBrushTool(MetavoxelEditor* editor) :
    HeightfieldBrushTool(editor, "Color Brush") {
    
    _form->addRow("Color:", _color = new QColorEditor(this));
}

QVariant HeightfieldColorBrushTool::createEdit(bool alternate) {
    return QVariant::fromValue(PaintHeightfieldColorEdit(_position, _radius->value(),
        alternate ? QColor() : _color->getColor()));
}

HeightfieldMaterialBrushTool::HeightfieldMaterialBrushTool(MetavoxelEditor* editor) :
    HeightfieldBrushTool(editor, "Material Brush") {
    
    _form->addRow(_materialEditor = new SharedObjectEditor(&MaterialObject::staticMetaObject, false));
    connect(_materialEditor, &SharedObjectEditor::objectChanged, this, &HeightfieldMaterialBrushTool::updateTexture);
}

QVariant HeightfieldMaterialBrushTool::createEdit(bool alternate) {
    if (alternate) {
        return QVariant::fromValue(PaintHeightfieldMaterialEdit(_position, _radius->value(), SharedObjectPointer(), QColor()));
    } else {
        SharedObjectPointer material = _materialEditor->getObject();
        _materialEditor->detachObject();
        return QVariant::fromValue(PaintHeightfieldMaterialEdit(_position, _radius->value(), material,
            _texture ? _texture->getAverageColor() : QColor()));
    }   
}

void HeightfieldMaterialBrushTool::updateTexture() {
    MaterialObject* material = static_cast<MaterialObject*>(_materialEditor->getObject().data());
    _texture = Application::getInstance()->getTextureCache()->getTexture(material->getDiffuse(), SPLAT_TEXTURE);
}
