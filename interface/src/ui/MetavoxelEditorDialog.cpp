//
//  MetavoxelEditorDialog.cpp
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <AttributeRegistry.h>

#include "Application.h"
#include "MetavoxelEditorDialog.h"

enum GridPlane {
    GRID_PLANE_XY, GRID_PLANE_XZ, GRID_PLANE_YZ
};

MetavoxelEditorDialog::MetavoxelEditorDialog() :
    QDialog(Application::getInstance()->getGLWidget()) {
    
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
    connect(_attributes, SIGNAL(itemSelectionChanged()), SLOT(updateValueEditor()));

    QPushButton* newAttribute = new QPushButton("New...");
    attributeLayout->addWidget(newAttribute);
    connect(newAttribute, SIGNAL(clicked()), SLOT(createNewAttribute()));

    QFormLayout* formLayout = new QFormLayout();
    topLayout->addLayout(formLayout);
    
    formLayout->addRow("Grid Plane:", _gridPlane = new QComboBox());
    _gridPlane->addItem("X/Y");
    _gridPlane->addItem("X/Z");
    _gridPlane->addItem("Y/Z");
    _gridPlane->setCurrentIndex(GRID_PLANE_XZ);
    
    formLayout->addRow("Grid Spacing:", _gridSpacing = new QDoubleSpinBox());
    _gridSpacing->setValue(0.1);
    _gridSpacing->setMaximum(FLT_MAX);
    _gridSpacing->setSingleStep(0.01);
    connect(_gridSpacing, SIGNAL(valueChanged(double)), SLOT(updateGridPosition()));

    formLayout->addRow("Grid Position:", _gridPosition = new QDoubleSpinBox());
    _gridPosition->setSingleStep(0.1);
    _gridPosition->setMinimum(-FLT_MAX);
    _gridPosition->setMaximum(FLT_MAX);

    _value = new QGroupBox();
    _value->setTitle("Value");
    topLayout->addWidget(_value);
    
    QVBoxLayout* valueLayout = new QVBoxLayout();
    _value->setLayout(valueLayout);

    updateAttributes();
    
    connect(Application::getInstance(), SIGNAL(renderingInWorldInterface()), SLOT(render()));
    
    show();
    
    if (_gridProgram.isLinked()) {
        return;
    }
    switchToResourcesParentIfRequired();
    _gridProgram.addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/grid.frag");
    _gridProgram.link();
}

void MetavoxelEditorDialog::updateValueEditor() {
    QString selected = getSelectedAttribute();
    if (selected.isNull()) {
        _value->setVisible(false);
        return;
    }
    _value->setVisible(true);
    
    if (!_value->layout()->isEmpty()) {
        delete _value->layout()->takeAt(0);
    }
      
    AttributePointer attribute = AttributeRegistry::getInstance()->getAttribute(selected);
    QWidget* editor = attribute->createEditor();
    if (editor) {
        _value->layout()->addWidget(editor);
    }
}

void MetavoxelEditorDialog::createNewAttribute() {
    QDialog dialog(this);
    dialog.setWindowTitle("New Attribute");
    
    QVBoxLayout layout;
    dialog.setLayout(&layout);
    
    QFormLayout form;
    layout.addLayout(&form);
    
    QLineEdit name;
    form.addRow("Name:", &name);
    
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog.connect(&buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(&buttons, SIGNAL(rejected()), SLOT(reject()));
    
    layout.addWidget(&buttons);
    
    if (!dialog.exec()) {
        return;
    }
    QString nameText = name.text().trimmed();
    AttributeRegistry::getInstance()->registerAttribute(new QRgbAttribute(nameText));
    
    updateAttributes(nameText);
}

void MetavoxelEditorDialog::updateGridPosition() {
    // make sure our grid position matches our grid spacing
    double step = _gridSpacing->value();
    if (step > 0.0) {
        _gridPosition->setSingleStep(step);
        _gridPosition->setValue(step * floor(_gridPosition->value() / step));
    }
}

void MetavoxelEditorDialog::render() {
    const float GRID_BRIGHTNESS = 0.5f;
    glColor3f(GRID_BRIGHTNESS, GRID_BRIGHTNESS, GRID_BRIGHTNESS);
    glDisable(GL_LIGHTING);
    
    glPushMatrix();
    
    glm::quat rotation;
    switch (_gridPlane->currentIndex()) {
        case GRID_PLANE_XZ:
            rotation = glm::angleAxis(90.0f, 1.0f, 0.0f, 0.0f);
            glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            break;
            
        case GRID_PLANE_YZ:
            rotation = glm::angleAxis(-90.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            break;
    }
    
    // find the intersection of the rotated mouse ray with the plane
    glm::vec3 rayOrigin = rotation * Application::getInstance()->getMouseRayOrigin();
    glm::vec3 rayDirection = rotation * Application::getInstance()->getMouseRayDirection();
    float spacing = _gridSpacing->value();
    float position = _gridPosition->value();
    if (fabs(rayDirection.z) > EPSILON) {
        float distance = (position - rayOrigin.z) / rayDirection.z;
        glm::vec3 intersection = rayOrigin + rayDirection * distance;
        
        glLineWidth(4.0f);
        glBegin(GL_LINE_LOOP);
        float x = spacing * floorf(intersection.x / spacing);
        float y = spacing * floorf(intersection.y / spacing);
        glVertex3f(x, y, position);
        glVertex3f(x + spacing, y, position);
        glVertex3f(x + spacing, y + spacing, position);
        glVertex3f(x, y + spacing, position);
        glEnd();
        glLineWidth(1.0f);
    }
    
    // center the grid around the camera position on the plane
    glm::vec3 rotated = rotation * Application::getInstance()->getCamera()->getPosition();
    const int GRID_DIVISIONS = 300;
    glTranslatef(spacing * (floorf(rotated.x / spacing) - GRID_DIVISIONS / 2),
        spacing * (floorf(rotated.y / spacing) - GRID_DIVISIONS / 2), position);
    
    float scale = GRID_DIVISIONS * spacing;
    glScalef(scale, scale, scale);
    
    _gridProgram.bind();
    
    Application::getInstance()->getGeometryCache()->renderGrid(GRID_DIVISIONS, GRID_DIVISIONS);
    
    _gridProgram.release();
    
    glPopMatrix();
    
    glEnable(GL_LIGHTING);
}

void MetavoxelEditorDialog::updateAttributes(const QString& select) {
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

QString MetavoxelEditorDialog::getSelectedAttribute() const {
    QList<QListWidgetItem*> selectedItems = _attributes->selectedItems();
    return selectedItems.isEmpty() ? QString() : selectedItems.first()->text();
}

ProgramObject MetavoxelEditorDialog::_gridProgram;
