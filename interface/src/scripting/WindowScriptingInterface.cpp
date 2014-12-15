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
#include "MainWindow.h"
#include "Menu.h"
#include "ui/ModelsBrowser.h"

#include "WindowScriptingInterface.h"

WindowScriptingInterface* WindowScriptingInterface::getInstance() {
    static WindowScriptingInterface sharedInstance;
    return &sharedInstance;
}

WindowScriptingInterface::WindowScriptingInterface() :
    _editDialog(NULL),
    _nonBlockingFormActive(false),
    _formResult(QDialog::Rejected) 
{
}

WebWindowClass* WindowScriptingInterface::doCreateWebWindow(const QString& title, const QString& url, int width, int height) {
    return new WebWindowClass(title, url, width, height);
}

QScriptValue WindowScriptingInterface::hasFocus() {
    return Application::getInstance()->getGLWidget()->hasFocus();
}

void WindowScriptingInterface::setCursorVisible(bool visible) {
    Application::getInstance()->setCursorVisible(visible);
}

void WindowScriptingInterface::setCursorPosition(int x, int y) {
    QCursor::setPos(x, y);
}

QScriptValue WindowScriptingInterface::getCursorPositionX() {
    return QCursor::pos().x();
}

QScriptValue WindowScriptingInterface::getCursorPositionY() {
    return QCursor::pos().y();
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

void WindowScriptingInterface::nonBlockingForm(const QString& title, QScriptValue form) {
    QMetaObject::invokeMethod(this, "showNonBlockingForm", Qt::BlockingQueuedConnection,
                              Q_ARG(const QString&, title), Q_ARG(QScriptValue, form));
}

void WindowScriptingInterface::reloadNonBlockingForm(QScriptValue newValues) {
    QMetaObject::invokeMethod(this, "doReloadNonBlockingForm", Qt::BlockingQueuedConnection,
                              Q_ARG(QScriptValue, newValues));
}


QScriptValue WindowScriptingInterface::getNonBlockingFormResult(QScriptValue form) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "doGetNonBlockingFormResult", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(QScriptValue, form));
    return retVal;
}

QScriptValue WindowScriptingInterface::peekNonBlockingFormResult(QScriptValue form) {
    QScriptValue retVal;
    QMetaObject::invokeMethod(this, "doPeekNonBlockingFormResult", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QScriptValue, retVal),
                              Q_ARG(QScriptValue, form));
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

void WindowScriptingInterface::chooseDirectory() {
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());

    QString title = button->property("title").toString();
    QString path = button->property("path").toString();
    QRegExp displayAs = button->property("displayAs").toRegExp();
    QRegExp validateAs = button->property("validateAs").toRegExp();
    QString errorMessage = button->property("errorMessage").toString();

    QString directory = QFileDialog::getExistingDirectory(button, title, path);
    if (directory.isEmpty()) {
        return;
    }

    if (!validateAs.exactMatch(directory)) {
        QMessageBox::warning(NULL, "Invalid Directory", errorMessage);
        return;
    }

    button->setProperty("path", directory);

    displayAs.indexIn(directory);
    QString buttonText = displayAs.cap(1) != "" ? displayAs.cap(1) : ".";
    button->setText(buttonText);
}

void WindowScriptingInterface::inlineButtonClicked() {
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());
    QString name = button->property("name").toString();
    emit inlineButtonClicked(name);
}

QString WindowScriptingInterface::jsRegExp2QtRegExp(QString string) {
    // Converts string representation of RegExp from JavaScript format to Qt format.
    return string.mid(1, string.length() - 2)  // No enclosing slashes.
        .replace("\\/", "/");                  // No escaping of forward slash.
}

void WindowScriptingInterface::showNonBlockingForm(const QString& title, QScriptValue form) {
    if (!form.isArray() || (form.isArray() && form.property("length").toInt32() <= 0)) {
        return;
    }
    
    // what should we do if someone calls us while we still think we have a dialog showing???
    if (_editDialog) {
        qDebug() << "Show Non-Blocking Form called when form already active.";
        return;
    }
    
    _form = form;
    _editDialog = createForm(title, _form);
    _nonBlockingFormActive = true;
    
    connect(_editDialog, SIGNAL(accepted()), this, SLOT(nonBlockingFormAccepted()));
    connect(_editDialog, SIGNAL(rejected()), this, SLOT(nonBlockingFormRejected()));
    
    _editDialog->setModal(true);
    _editDialog->show();
}

void WindowScriptingInterface::doReloadNonBlockingForm(QScriptValue newValues) {
    if (!newValues.isArray() || (newValues.isArray() && newValues.property("length").toInt32() <= 0)) {
        return;
    }
    
    // what should we do if someone calls us while we still think we have a dialog showing???
    if (!_editDialog) {
        qDebug() << "Reload Non-Blocking Form called when no form is active.";
        return;
    }

    for (int i = 0; i < newValues.property("length").toInt32(); ++i) {
        QScriptValue item = newValues.property(i);
        
        if (item.property("oldIndex").isValid()) {
            int oldIndex = item.property("oldIndex").toInt32();
            QScriptValue oldItem = _form.property(oldIndex);
            if (oldItem.isValid()) {
                QLineEdit* originalEdit = _edits[oldItem.property("editIndex").toInt32()];
                originalEdit->setText(item.property("value").toString());
            }
        }
    }
}


bool WindowScriptingInterface::nonBlockingFormActive() {
    return _nonBlockingFormActive;
}

QScriptValue WindowScriptingInterface::doPeekNonBlockingFormResult(QScriptValue array) {
    QScriptValue retVal;
    
    int e = -1;
    int d = -1;
    int c = -1;
    int h = -1;
    for (int i = 0; i < _form.property("length").toInt32(); ++i) {
        QScriptValue item = _form.property(i);
        QScriptValue value = item.property("value");

        if (item.property("button").toString() != "") {
            // Nothing to do
        } else if (item.property("type").toString() == "inlineButton") {
            // Nothing to do
        } else if (item.property("type").toString() == "header") {
            // Nothing to do
        } else if (item.property("directory").toString() != "") {
            d += 1;
            value = _directories.at(d)->property("path").toString();
            item.setProperty("directory", value);
            _form.setProperty(i, item);
        } else if (item.property("options").isArray()) {
            c += 1;
            item.setProperty("value",
                _combos.at(c)->currentIndex() < item.property("options").property("length").toInt32() ?
                item.property("options").property(_combos.at(c)->currentIndex()) :
                array.engine()->undefinedValue()
            );
            _form.setProperty(i, item);
        } else if (item.property("type").toString() == "checkbox") {
            h++;
            value = _checks.at(h)->checkState() == Qt::Checked;
            item.setProperty("value", value);
            _form.setProperty(i, item);
        } else {
            e += 1;
            bool ok = true;
            if (value.isNumber()) {
                value = _edits.at(e)->text().toDouble(&ok);
            } else if (value.isString()) {
                value = _edits.at(e)->text();
            } else if (value.isBool()) {
                if (_edits.at(e)->text() == "true") {
                    value = true;
                } else if (_edits.at(e)->text() == "false") {
                    value = false;
                } else {
                    ok = false;
                }
            }
            if (ok) {
                item.setProperty("value", value);
                _form.setProperty(i, item);
            }
        }
    }
    
    array = _form;
    return (_formResult == QDialog::Accepted);    
}

QScriptValue WindowScriptingInterface::doGetNonBlockingFormResult(QScriptValue array) {
    QScriptValue retVal;
    
    if (_formResult == QDialog::Accepted) {
        int e = -1;
        int d = -1;
        int c = -1;
        int h = -1;
        for (int i = 0; i < _form.property("length").toInt32(); ++i) {
            QScriptValue item = _form.property(i);
            QScriptValue value = item.property("value");

            if (item.property("button").toString() != "") {
                // Nothing to do
            } else if (item.property("type").toString() == "inlineButton") {
                // Nothing to do
            } else if (item.property("type").toString() == "header") {
                // Nothing to do
            } else if (item.property("directory").toString() != "") {
                d += 1;
                value = _directories.at(d)->property("path").toString();
                item.setProperty("directory", value);
                _form.setProperty(i, item);
            } else if (item.property("options").isArray()) {
                c += 1;
                item.setProperty("value",
                    _combos.at(c)->currentIndex() < item.property("options").property("length").toInt32() ?
                    item.property("options").property(_combos.at(c)->currentIndex()) :
                    array.engine()->undefinedValue()
                );
                _form.setProperty(i, item);
            } else if (item.property("type").toString() == "checkbox") {
                h++;
                value = _checks.at(h)->checkState() == Qt::Checked;
                item.setProperty("value", value);
                _form.setProperty(i, item);
            } else {
                e += 1;
                bool ok = true;
                if (value.isNumber()) {
                    value = _edits.at(e)->text().toDouble(&ok);
                } else if (value.isString()) {
                    value = _edits.at(e)->text();
                } else if (value.isBool()) {
                    if (_edits.at(e)->text() == "true") {
                        value = true;
                    } else if (_edits.at(e)->text() == "false") {
                        value = false;
                    } else {
                        ok = false;
                    }
                }
                if (ok) {
                    item.setProperty("value", value);
                    _form.setProperty(i, item);
                }
            }
        }
    }
    
    delete _editDialog;
    _editDialog = NULL;
    _form = QScriptValue();
    _edits.clear();
    _directories.clear();
    _combos.clear();
    _checks.clear();
    
    array = _form;
    return (_formResult == QDialog::Accepted);    
}


/// Display a form layout with an edit box
/// \param const QString& title title to display
/// \param const QScriptValue form to display as an array of objects:
/// - label, value
/// - label, directory, title, display regexp, validate regexp, error message
/// - button ("Cancel")
/// \return QScriptValue `true` if 'OK' was clicked, `false` otherwise
QScriptValue WindowScriptingInterface::showForm(const QString& title, QScriptValue form) {
    if (form.isArray() && form.property("length").toInt32() <= 0) {
        return false;
    }
    QDialog* editDialog = createForm(title, form);

    int result = editDialog->exec();
    
    if (result == QDialog::Accepted) {
        int e = -1;
        int d = -1;
        int c = -1;
        int h = -1;
        for (int i = 0; i < form.property("length").toInt32(); ++i) {
            QScriptValue item = form.property(i);
            QScriptValue value = item.property("value");

            if (item.property("button").toString() != "") {
                // Nothing to do
            } else if (item.property("type").toString() == "inlineButton") {
                // Nothing to do
            } else if (item.property("type").toString() == "header") {
                // Nothing to do
            } else if (item.property("directory").toString() != "") {
                d += 1;
                value = _directories.at(d)->property("path").toString();
                item.setProperty("directory", value);
                form.setProperty(i, item);
            } else if (item.property("options").isArray()) {
                c += 1;
                item.setProperty("value", 
                    _combos.at(c)->currentIndex() < item.property("options").property("length").toInt32() ?
                    item.property("options").property(_combos.at(c)->currentIndex()) :
                    form.engine()->undefinedValue()
                );
                form.setProperty(i, item);
            } else if (item.property("type").toString() == "checkbox") {
                h++;
                value = _checks.at(h)->checkState() == Qt::Checked;
                item.setProperty("value", value);
                form.setProperty(i, item);
            } else {
                e += 1;
                bool ok = true;
                if (value.isNumber()) {
                    value = _edits.at(e)->text().toDouble(&ok);
                } else if (value.isString()) {
                    value = _edits.at(e)->text();
                } else if (value.isBool()) {
                    if (_edits.at(e)->text() == "true") {
                        value = true;
                    } else if (_edits.at(e)->text() == "false") {
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
    }
    
    delete editDialog;
    _combos.clear();
    _checks.clear();
    _edits.clear();
    _directories.clear();
    return (result == QDialog::Accepted);
}



QDialog* WindowScriptingInterface::createForm(const QString& title, QScriptValue form) {
    QDialog* editDialog = new QDialog(Application::getInstance()->getWindow());
    editDialog->setWindowTitle(title);

    bool cancelButton = false;
    
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
    
    for (int i = 0; i < form.property("length").toInt32(); ++i) {
        QScriptValue item = form.property(i);

        if (item.property("button").toString() != "") {
            cancelButton = cancelButton || item.property("button").toString().toLower() == "cancel";

        } else if (item.property("directory").toString() != "") {
            QString path = item.property("directory").toString();
            QString title = item.property("title").toString();
            if (title == "") {
                title = "Choose Directory";
            }
            QString displayAsString = item.property("displayAs").toString();
            QRegExp displayAs = QRegExp(displayAsString != "" ? jsRegExp2QtRegExp(displayAsString) : "^(.*)$");
            QString validateAsString = item.property("validateAs").toString();
            QRegExp validateAs = QRegExp(validateAsString != "" ? jsRegExp2QtRegExp(validateAsString) : ".*"); 
            QString errorMessage = item.property("errorMessage").toString();
            if (errorMessage == "") {
                errorMessage = "Invalid directory";
            }

            QPushButton* directory = new QPushButton(displayAs.cap(1));
            directory->setProperty("title", title);
            directory->setProperty("path", path);
            directory->setProperty("displayAs", displayAs);
            directory->setProperty("validateAs", validateAs);
            directory->setProperty("errorMessage", errorMessage);
            displayAs.indexIn(path);
            directory->setText(displayAs.cap(1) != "" ? displayAs.cap(1) : ".");

            directory->setMinimumWidth(200);
            _directories.push_back(directory);

            formLayout->addRow(new QLabel(item.property("label").toString()), directory);
            connect(directory, SIGNAL(clicked(bool)), SLOT(chooseDirectory()));

        } else if (item.property("type").toString() == "inlineButton") {
            QString buttonLabel = item.property("buttonLabel").toString();

            QPushButton* inlineButton = new QPushButton(buttonLabel);
            inlineButton->setMinimumWidth(200);
            inlineButton->setProperty("name", item.property("name").toString());
            formLayout->addRow(new QLabel(item.property("label").toString()), inlineButton);
            connect(inlineButton, SIGNAL(clicked(bool)), SLOT(inlineButtonClicked()));

        } else if (item.property("type").toString() == "header") {
            formLayout->addRow(new QLabel(item.property("label").toString()));
        } else if (item.property("options").isArray()) {
            QComboBox* combo = new QComboBox();
            combo->setMinimumWidth(200);
            qint32 options_count = item.property("options").property("length").toInt32();
            for (qint32 i = 0; i < options_count; i++) {
                combo->addItem(item.property("options").property(i).toString());
            }
            _combos.push_back(combo);
            formLayout->addRow(new QLabel(item.property("label").toString()), combo);
        } else if (item.property("type").toString() == "checkbox") {
            QCheckBox* check = new QCheckBox();
            check->setTristate(false);
            check->setCheckState(item.property("value").toString() == "true" ? Qt::Checked : Qt::Unchecked);
            _checks.push_back(check);
            formLayout->addRow(new QLabel(item.property("label").toString()), check);
        } else {
            QLineEdit* edit = new QLineEdit(item.property("value").toString());
            edit->setMinimumWidth(200);
            int editIndex = _edits.size();
            _edits.push_back(edit);
            item.setProperty("editIndex", editIndex);
            formLayout->addRow(new QLabel(item.property("label").toString()), edit);
        }
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok 
        | (cancelButton ? QDialogButtonBox::Cancel : QDialogButtonBox::NoButton)
        );
    connect(buttons, SIGNAL(accepted()), editDialog, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), editDialog, SLOT(reject()));
    layout->addWidget(buttons);

    return editDialog;    
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
    if (fileInfo.isDir()) {
        fileInfo.setFile(directory, "__HIFI_INVALID_FILE__");
        path = fileInfo.filePath();
    }
    
    QFileDialog fileDialog(Application::getInstance()->getWindow(), title, path, nameFilter);
    fileDialog.setAcceptMode(acceptMode);
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

int WindowScriptingInterface::getX() {
    return Application::getInstance()->getWindow()->x();
}

int WindowScriptingInterface::getY() {
    return Application::getInstance()->getWindow()->y();
}
