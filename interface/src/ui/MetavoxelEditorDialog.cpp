//
//  MetavoxelEditorDialog.cpp
//  interface
//
//  Created by Andrzej Kapolka on 1/21/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <AttributeRegistry.h>

#include "Application.h"
#include "MetavoxelEditorDialog.h"

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

    _value = new QGroupBox();
    _value->setTitle("Value");
    topLayout->addWidget(_value);
    
    QVBoxLayout* valueLayout = new QVBoxLayout();
    _value->setLayout(valueLayout);

    updateAttributes();
    
    show();
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
