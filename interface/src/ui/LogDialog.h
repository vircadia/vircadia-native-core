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
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>

class LogDialog : public QDialog {
    Q_OBJECT

public:
    LogDialog(QWidget* parent);
    ~LogDialog();

public slots:
    void appendLogLine(QString logLine);

private slots:
    void handleSearchButton();
    void handleRevealButton();
    void handleExtraDebuggingCheckbox(const int);
    void handleSeachTextChanged(QString);

protected:
    void resizeEvent(QResizeEvent*);
    void showEvent(QShowEvent*);

private:
    QPushButton* _searchButton;
    QLineEdit* _searchTextBox;
    QCheckBox* _extraDebuggingBox;
    QPushButton* _revealLogButton;
    QPlainTextEdit* _logTextBox;
    pthread_mutex_t _mutex;

    void initControls();
};

#endif

