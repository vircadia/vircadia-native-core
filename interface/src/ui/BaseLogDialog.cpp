//
//  BaseLogDialog.cpp
//  interface/src/ui
//
//  Created by Clement Brisset on 1/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BaseLogDialog.h"

#include <QDir>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSyntaxHighlighter>

#include <PathUtils.h>

const int TOP_BAR_HEIGHT = 46;
const int INITIAL_WIDTH = 720;
const int INITIAL_HEIGHT = 480;
const int MINIMAL_WIDTH = 570;
const int SEARCH_BUTTON_LEFT = 25;
const int SEARCH_BUTTON_WIDTH = 20;
const int SEARCH_TEXT_WIDTH = 240;
const QColor HIGHLIGHT_COLOR = QColor("#3366CC");

class KeywordHighlighter : public QSyntaxHighlighter {
public:
    KeywordHighlighter(QTextDocument* parent = nullptr);
    QString keyword;

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat keywordFormat;
    
};

BaseLogDialog::BaseLogDialog(QWidget* parent) : QDialog(parent, Qt::Window) {
    setWindowTitle("Base Log");
    setAttribute(Qt::WA_DeleteOnClose);

    QFile styleSheet(PathUtils::resourcesPath() + "styles/log_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        QDir::setCurrent(PathUtils::resourcesPath());
        setStyleSheet(styleSheet.readAll());
    }

    initControls();

    resize(INITIAL_WIDTH, INITIAL_HEIGHT);
    setMinimumWidth(MINIMAL_WIDTH);
}

BaseLogDialog::~BaseLogDialog() {
    deleteLater();
}

void BaseLogDialog::initControls() {
    _searchButton = new QPushButton(this);
    // set object name for css styling
    _searchButton->setObjectName("searchButton");
    _leftPad = SEARCH_BUTTON_LEFT;
    _searchButton->setGeometry(_leftPad, ELEMENT_MARGIN, SEARCH_BUTTON_WIDTH, ELEMENT_HEIGHT);
    _leftPad += SEARCH_BUTTON_WIDTH;
    _searchButton->show();
    connect(_searchButton, SIGNAL(clicked()), SLOT(handleSearchButton()));

    _searchTextBox = new QLineEdit(this);
    // disable blue outline in Mac
    _searchTextBox->setAttribute(Qt::WA_MacShowFocusRect, false);
    _searchTextBox->setGeometry(_leftPad, ELEMENT_MARGIN, SEARCH_TEXT_WIDTH, ELEMENT_HEIGHT);
    _leftPad += SEARCH_TEXT_WIDTH + CHECKBOX_MARGIN;
    _searchTextBox->show();
    connect(_searchTextBox, SIGNAL(textChanged(QString)), SLOT(handleSearchTextChanged(QString)));

    _logTextBox = new QPlainTextEdit(this);
    _logTextBox->setReadOnly(true);
    _logTextBox->show();
    _highlighter = new KeywordHighlighter(_logTextBox->document());

}

void BaseLogDialog::showEvent(QShowEvent* event)  {
    showLogData();
}

void BaseLogDialog::resizeEvent(QResizeEvent* event) {
    _logTextBox->setGeometry(0, TOP_BAR_HEIGHT, width(), height() - TOP_BAR_HEIGHT);
}

void BaseLogDialog::appendLogLine(QString logLine) {
    if (logLine.contains(_searchTerm, Qt::CaseInsensitive)) {
        _logTextBox->appendPlainText(logLine.trimmed());
    }
}

void BaseLogDialog::handleSearchButton() {
    _searchTextBox->setFocus();
}

void BaseLogDialog::handleSearchTextChanged(QString searchText) {
    _searchTerm = searchText;
    _highlighter->keyword = searchText;
    _highlighter->rehighlight();
}

void BaseLogDialog::showLogData() {
    _logTextBox->clear();
    _logTextBox->appendPlainText(getCurrentLog());
    _logTextBox->ensureCursorVisible();
}

KeywordHighlighter::KeywordHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    keywordFormat.setForeground(HIGHLIGHT_COLOR);
}

void KeywordHighlighter::highlightBlock(const QString& text) {
    if (keyword.isNull() || keyword.isEmpty()) {
        return;
    }

    int index = text.indexOf(keyword, 0, Qt::CaseInsensitive);
    int length = keyword.length();

    while (index >= 0) {
        setFormat(index, length, keywordFormat);
        index = text.indexOf(keyword, index + length, Qt::CaseInsensitive);
    }
}
