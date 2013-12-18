//
//  LogDialog.h
//  interface
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__LogDialog__
#define __interface__LogDialog__

#include <QDialog>
#include <QPlainTextEdit>

class LogDialog : public QDialog {
    Q_OBJECT

public:
    LogDialog(QWidget* parent);
    ~LogDialog();

//signals:
//    void closed();

public slots:    
//    void reject();
    void appendLogLine(QString logLine);

protected:
    // Emits a 'closed' signal when this dialog is closed.
//    void closeEvent(QCloseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void showEvent(QShowEvent* e);

private:
    QPlainTextEdit* _logTextBox;
    pthread_mutex_t _mutex;

};

#endif

