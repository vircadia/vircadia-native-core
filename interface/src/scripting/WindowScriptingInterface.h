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

#ifndef hifi_WindowScriptingInterface_h
#define hifi_WindowScriptingInterface_h

#include <QObject>
#include <QScriptValue>
#include <QString>
#include <QFileDialog>
#include <QComboBox>
#include <QLineEdit>

#include "WebWindowClass.h"

class WindowScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(int innerWidth READ getInnerWidth)
    Q_PROPERTY(int innerHeight READ getInnerHeight)
    Q_PROPERTY(int x READ getX)
    Q_PROPERTY(int y READ getY)
public:
    static WindowScriptingInterface* getInstance();
    int getInnerWidth();
    int getInnerHeight();
    int getX();
    int getY();

public slots:
    QScriptValue getCursorPositionX();
    QScriptValue getCursorPositionY();
    void setCursorPosition(int x, int y);
    void setCursorVisible(bool visible);
    QScriptValue hasFocus();
    QScriptValue alert(const QString& message = "");
    QScriptValue confirm(const QString& message = "");
    QScriptValue form(const QString& title, QScriptValue array);
    QScriptValue prompt(const QString& message = "", const QString& defaultText = "");
    QScriptValue browse(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QScriptValue save(const QString& title = "", const QString& directory = "",  const QString& nameFilter = "");
    QScriptValue s3Browse(const QString& nameFilter = "");

    void nonBlockingForm(const QString& title, QScriptValue array);
    void reloadNonBlockingForm(QScriptValue array);
    QScriptValue getNonBlockingFormResult(QScriptValue array);
    QScriptValue peekNonBlockingFormResult(QScriptValue array);

signals:
    void inlineButtonClicked(const QString& name);
    void nonBlockingFormClosed();

private slots:
    QScriptValue showAlert(const QString& message);
    QScriptValue showConfirm(const QString& message);
    QScriptValue showForm(const QString& title, QScriptValue form);
    QScriptValue showPrompt(const QString& message, const QString& defaultText);
    QScriptValue showBrowse(const QString& title, const QString& directory, const QString& nameFilter,
                            QFileDialog::AcceptMode acceptMode = QFileDialog::AcceptOpen);
    QScriptValue showS3Browse(const QString& nameFilter);

    void showNonBlockingForm(const QString& title, QScriptValue array);
    void doReloadNonBlockingForm(QScriptValue array);
    bool nonBlockingFormActive();
    QScriptValue doGetNonBlockingFormResult(QScriptValue array);
    QScriptValue doPeekNonBlockingFormResult(QScriptValue array);

    void chooseDirectory();
    void inlineButtonClicked();

    void nonBlockingFormAccepted() { _nonBlockingFormActive = false; _formResult = QDialog::Accepted; emit nonBlockingFormClosed(); }
    void nonBlockingFormRejected() { _nonBlockingFormActive = false; _formResult = QDialog::Rejected; emit nonBlockingFormClosed(); }

    WebWindowClass* doCreateWebWindow(const QString& title, const QString& url, int width, int height);
    
private:
    WindowScriptingInterface();
    QString jsRegExp2QtRegExp(QString string);
    QDialog* createForm(const QString& title, QScriptValue form);
    
    QDialog* _editDialog;
    QScriptValue _form;
    bool _nonBlockingFormActive;
    int _formResult;
    QVector<QComboBox*> _combos;
    QVector<QLineEdit*> _edits;
    QVector<QPushButton*> _directories;
};

#endif // hifi_WindowScriptingInterface_h
