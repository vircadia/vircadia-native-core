//
//  WindowScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDir>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QScriptValue>

#include "Application.h"
#include "Menu.h"
#include "ui/ModelsBrowser.h"

#include "WindowScriptingInterface.h"

WindowScriptingInterface* WindowScriptingInterface::getInstance() {
    static WindowScriptingInterface sharedInstance;
    return &sharedInstance;
}

WindowScriptingInterface::WindowScriptingInterface() {
}

QScriptValue WindowScriptingInterface::hasFocus() {
    return Application::getInstance()->getGLWidget()->hasFocus();
}

void WindowScriptingInterface::setCursorPosition(int x, int y) {
    QCursor::setPos(x, y);
}

QScriptValue WindowScriptingInterface::getCursorPositionX() {
    QPoint pos = QCursor::pos();
    return pos.x();
}

QScriptValue WindowScriptingInterface::getCursorPositionY() {
    QPoint pos = QCursor::pos();
    return pos.y();
}

QScriptValue WindowScriptingInterface::alert(const QString& message) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showAlert", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QScriptValue, retVal), Q_ARG(const QString&, message));
    return retVal;
}

QScriptValue WindowScriptingInterface::confirm(const QString& message) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showConfirm", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal), Q_ARG(const QString&, message));
    return retVal;
}

QScriptValue WindowScriptingInterface::form(const QString& title, QScriptValue form) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showForm", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, title), Q_ARG(QScriptValue, form));
    return retVal;
}

QScriptValue WindowScriptingInterface::prompt(const QString& message, const QString& defaultText) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showPrompt", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, message), Q_ARG(const QString&, defaultText));
    return retVal;
}

QScriptValue WindowScriptingInterface::browse(const QString& title, const QString& directory,  const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showBrowse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, title), Q_ARG(const QString&, directory), Q_ARG(const QString&, nameFilter));
    return retVal;
}

QScriptValue WindowScriptingInterface::save(const QString& title, const QString& directory,  const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showBrowse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, title), Q_ARG(const QString&, directory), Q_ARG(const QString&, nameFilter),
                              Q_ARG(QFileDialog::AcceptMode, QFileDialog::AcceptSave));
    return retVal;
}

QScriptValue WindowScriptingInterface::s3Browse(const QString& nameFilter) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "showS3Browse", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(const QString&, nameFilter));
    return retVal;
}

/// Display an alert box
/// \param const QString& message message to display
/// \return QScriptValue::UndefinedValue
QScriptValue WindowScriptingInterface::showAlert(const QString& message) {
    QMessageBox::warning(Application::getInstance()->getWindow(), "", message);
    return QScriptValue::UndefinedValue;
}

/// Display a confirmation box with the options 'Yes' and 'No'
/// \param const QString& message message to display
/// \return QScriptValue `true` if 'Yes' was clicked, `false` otherwise
QScriptValue WindowScriptingInterface::showConfirm(const QString& message) {
    QMessageBox::StandardButton response = QMessageBox::question(Application::getInstance()->getWindow(), "", message);
    return QScriptValue(response == QMessageBox::Yes);
}

/// Display a form layout with an edit box
/// \param const QString& title title to display
/// \param const QScriptValue form to display (array containing labels and values)
/// \return QScriptValue result form (unchanged is dialog canceled)
QScriptValue WindowScriptingInterface::showForm(const QString& title, QScriptValue form) {
    if (form.isArray() && form.property("length").toInt32() > 0) {
        QDialog* editDialog = new QDialog(Application::getInstance()->getWindow());
        editDialog->setWindowTitle(title);
        
        QVBoxLayout* layout = new QVBoxLayout();
        editDialog->setLayout(layout);
        
        QScrollArea* area = new QScrollArea();
        layout->addWidget(area);
        area->setWidgetResizable(true);
        QWidget* container = new QWidget();
        QFormLayout* formLayout = new QFormLayout();
        container->setLayout(formLayout);
        container->sizePolicy().setHorizontalStretch(1);
        formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
        formLayout->setLabelAlignment(Qt::AlignLeft);
        
        area->setWidget(container);
        
        QVector<QLineEdit*> edits;
        for (int i = 0; i < form.property("length").toInt32(); ++i) {
            QScriptValue item = form.property(i);
            edits.push_back(new QLineEdit(item.property("value").toString()));
            formLayout->addRow(item.property("label").toString(), edits.back());
        }
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
        connect(buttons, SIGNAL(accepted()), editDialog, SLOT(accept()));
        layout->addWidget(buttons);
        
        if (editDialog->exec() == QDialog::Accepted) {
            for (int i = 0; i < form.property("length").toInt32(); ++i) {
                QScriptValue item = form.property(i);
                QScriptValue value = item.property("value");
                bool ok = true;
                if (value.isNumber()) {
                    value = edits.at(i)->text().toDouble(&ok);
                } else if (value.isString()) {
                    value = edits.at(i)->text();
                } else if (value.isBool()) {
                    if (edits.at(i)->text() == "true") {
                        value = true;
                    } else if (edits.at(i)->text() == "false") {
                        value = false;
                    } else {
                        ok = false;
                    }
                }
                if (ok) {
                    item.setProperty("value", value);
                    form.setProperty(i, item);
                }
            }
        }
        
        delete editDialog;
    }
    
    return form;
}

/// Display a prompt with a text box
/// \param const QString& message message to display
/// \param const QString& defaultText default text in the text box
/// \return QScriptValue string text value in text box if the dialog was accepted, `null` otherwise.
QScriptValue WindowScriptingInterface::showPrompt(const QString& message, const QString& defaultText) {
    QInputDialog promptDialog(Application::getInstance()->getWindow());
    promptDialog.setWindowTitle("");
    promptDialog.setLabelText(message);
    promptDialog.setTextValue(defaultText);
    promptDialog.setFixedSize(600, 200);
    
    if (promptDialog.exec() == QDialog::Accepted) {
        return QScriptValue(promptDialog.textValue());
    }
    
    return QScriptValue::NullValue;
}

/// Display a file dialog.  If `directory` is an invalid file or directory the browser will start at the current
/// working directory.
/// \param const QString& title title of the window
/// \param const QString& directory directory to start the file browser at
/// \param const QString& nameFilter filter to filter filenames by - see `QFileDialog`
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::showBrowse(const QString& title, const QString& directory, const QString& nameFilter,
                                                  QFileDialog::AcceptMode acceptMode) {
    // On OS X `directory` does not work as expected unless a file is included in the path, so we append a bogus
    // filename if the directory is valid.
    QString path = "";
    QFileInfo fileInfo = QFileInfo(directory);
    qDebug() << "File: " << directory << fileInfo.isFile();
    if (fileInfo.isDir()) {
        fileInfo.setFile(directory, "__HIFI_INVALID_FILE__");
        path = fileInfo.filePath();
    }
    
    QFileDialog fileDialog(Application::getInstance()->getWindow(), title, path, nameFilter);
    fileDialog.setAcceptMode(acceptMode);
    qDebug() << "Opening!";
    QUrl fileUrl(directory);
    if (acceptMode == QFileDialog::AcceptSave) {
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.selectFile(fileUrl.fileName());
    }
    if (fileDialog.exec()) {
        return QScriptValue(fileDialog.selectedFiles().first());
    }
    return QScriptValue::NullValue;
}

/// Display a browse window for S3 models
/// \param const QString& nameFilter filter to filter filenames
/// \return QScriptValue file path as a string if one was selected, otherwise `QScriptValue::NullValue`
QScriptValue WindowScriptingInterface::showS3Browse(const QString& nameFilter) {
    ModelsBrowser browser(ENTITY_MODEL);
    if (nameFilter != "") {
        browser.setNameFilter(nameFilter);
    }
    QEventLoop loop;
    connect(&browser, &ModelsBrowser::selected, &loop, &QEventLoop::quit);
    QMetaObject::invokeMethod(&browser, "browse", Qt::QueuedConnection);
    loop.exec();
    
    return browser.getSelectedFile();
}

int WindowScriptingInterface::getInnerWidth() {
    return Application::getInstance()->getWindow()->geometry().width();
}

int WindowScriptingInterface::getInnerHeight() {
    return Application::getInstance()->getWindow()->geometry().height();
}
