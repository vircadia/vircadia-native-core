//
//  BaseLogDialog.h
//  interface/src/ui
//
//  Created by Clement Brisset on 1/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BaseLogDialog_h
#define hifi_BaseLogDialog_h

#include <QDialog>

const int ELEMENT_MARGIN = 7;
const int ELEMENT_HEIGHT = 32;
const int CHECKBOX_MARGIN = 12;
const int CHECKBOX_WIDTH = 140;

class QPushButton;
class QLineEdit;
class QPlainTextEdit;
class KeywordHighlighter;

class BaseLogDialog : public QDialog {
    Q_OBJECT

public:
    BaseLogDialog(QWidget* parent);
    ~BaseLogDialog();

public slots:
    void appendLogLine(QString logLine);

private slots:
    void handleSearchButton();
    void handleSearchTextChanged(QString text);

protected:
    int _leftPad { 0 };

    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    virtual QString getCurrentLog() = 0;

private:
    QPushButton* _searchButton { nullptr };
    QLineEdit* _searchTextBox { nullptr };
    QPlainTextEdit* _logTextBox { nullptr };
    QString _searchTerm;
    KeywordHighlighter* _highlighter { nullptr };
    
    void initControls();
    void showLogData();
};


#endif // hifi_BaseLogDialog_h
