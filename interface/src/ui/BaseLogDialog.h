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
#include <QSyntaxHighlighter>

const int ELEMENT_MARGIN = 7;
const int ELEMENT_HEIGHT = 32;
const int CHECKBOX_MARGIN = 12;
const int CHECKBOX_WIDTH = 110;
const int COMBOBOX_WIDTH = 160;
const int BUTTON_MARGIN = 8;

class QPushButton;
class QLineEdit;
class QPlainTextEdit;

class Highlighter : public QSyntaxHighlighter {
public:
    Highlighter(QTextDocument* parent = nullptr);
    void setBold(int indexToBold);
    QString keyword;

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat boldFormat;
    QTextCharFormat keywordFormat;

};

class BaseLogDialog : public QDialog {
    Q_OBJECT

public:
    BaseLogDialog(QWidget* parent);
    ~BaseLogDialog();

public slots:
    virtual void appendLogLine(QString logLine);

private slots:
    void updateSelection();
    void handleSearchButton();
    void handleSearchTextChanged(QString text);
    void toggleSearchPrev();
    void toggleSearchNext();

protected:
    int _leftPad{ 0 };
    QString _searchTerm;
    QPlainTextEdit* _logTextBox{ nullptr };
    Highlighter* _highlighter{ nullptr };

    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    virtual QString getCurrentLog() = 0;
    void clearSearch();

private:
    QPushButton* _searchButton{ nullptr };
    QLineEdit* _searchTextBox{ nullptr };
    QPushButton* _searchPrevButton{ nullptr };
    QPushButton* _searchNextButton{ nullptr };

    void initControls();
    void showLogData();
};


#endif // hifi_BaseLogDialog_h
