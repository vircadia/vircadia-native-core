//
//  LogDialog.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 12/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LogDialog_h
#define hifi_LogDialog_h

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
    void highlightBlock(const QString &text) override;

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
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;

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

#endif // hifi_LogDialog_h
