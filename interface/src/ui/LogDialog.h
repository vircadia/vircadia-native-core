//
//  LogDialog.h
//  interface
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__LogDialog__
#define __interface__LogDialog__

#include "InterfaceConfig.h"

#include <QDialog>
#include <QMutex>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSyntaxHighlighter>

#include "AbstractLoggerInterface.h"

class KeywordHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    KeywordHighlighter(QTextDocument *parent = 0);
    QString keyword;

protected:
    void highlightBlock(const QString &text);

private:
    QTextCharFormat keywordFormat;

};

class LogDialog : public QDialog {
    Q_OBJECT

public:
    LogDialog(QWidget*, AbstractLoggerInterface*);
    ~LogDialog();

public slots:
    void appendLogLine(QString logLine);

private slots:
    void handleSearchButton();
    void handleRevealButton();
    void handleExtraDebuggingCheckbox(const int);
    void handleSearchTextChanged(const QString);

protected:
    void resizeEvent(QResizeEvent*);
    void showEvent(QShowEvent*);

private:
    QPushButton* _searchButton;
    QLineEdit* _searchTextBox;
    QCheckBox* _extraDebuggingBox;
    QPushButton* _revealLogButton;
    QPlainTextEdit* _logTextBox;
    QString _searchTerm;
    KeywordHighlighter* _highlighter;

    AbstractLoggerInterface* _logger;

    void initControls();
    void showLogData();
};

#endif

