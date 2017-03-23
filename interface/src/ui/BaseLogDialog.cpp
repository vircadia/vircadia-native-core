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
#include <QTextCursor>
#include <QPushButton>
#include <QSyntaxHighlighter>

#include <PathUtils.h>

const int TOP_BAR_HEIGHT = 46;
const int INITIAL_WIDTH = 720;
const int INITIAL_HEIGHT = 480;
const int MINIMAL_WIDTH = 700;
const int SEARCH_BUTTON_LEFT = 25;
const int SEARCH_BUTTON_WIDTH = 20;
const int SEARCH_TOGGLE_BUTTON_WIDTH = 50;
const int SEARCH_TEXT_WIDTH = 240;
const int TIME_STAMP_LENGTH = 16;
const int FONT_WEIGHT = 75;
const QColor HIGHLIGHT_COLOR = QColor("#3366CC");
const QColor BOLD_COLOR = QColor("#445c8c");
const QString BOLD_PATTERN = "\\[\\d*\\/.*:\\d*:\\d*\\]";

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
    _leftPad += SEARCH_TEXT_WIDTH + BUTTON_MARGIN;
    _searchTextBox->show();
    connect(_searchTextBox, SIGNAL(textChanged(QString)), SLOT(handleSearchTextChanged(QString)));
    connect(_searchTextBox, SIGNAL(returnPressed()), SLOT(toggleSearchNext()));

    _searchPrevButton = new QPushButton(this);
    _searchPrevButton->setObjectName("searchPrevButton");
    _searchPrevButton->setGeometry(_leftPad, ELEMENT_MARGIN, SEARCH_TOGGLE_BUTTON_WIDTH, ELEMENT_HEIGHT);
    _searchPrevButton->setText("Prev");
    _leftPad += SEARCH_TOGGLE_BUTTON_WIDTH + BUTTON_MARGIN;
    _searchPrevButton->show();
    connect(_searchPrevButton, SIGNAL(clicked()), SLOT(toggleSearchPrev()));

    _searchNextButton = new QPushButton(this);
    _searchNextButton->setObjectName("searchNextButton");
    _searchNextButton->setGeometry(_leftPad, ELEMENT_MARGIN, SEARCH_TOGGLE_BUTTON_WIDTH, ELEMENT_HEIGHT);
    _searchNextButton->setText("Next");
    _leftPad += SEARCH_TOGGLE_BUTTON_WIDTH + CHECKBOX_MARGIN;
    _searchNextButton->show();
    connect(_searchNextButton, SIGNAL(clicked()), SLOT(toggleSearchNext()));

    _logTextBox = new QPlainTextEdit(this);
    _logTextBox->setReadOnly(true);
    _logTextBox->show();
    _highlighter = new Highlighter(_logTextBox->document());
    connect(_logTextBox, SIGNAL(selectionChanged()), SLOT(updateSelection()));
}

void BaseLogDialog::showEvent(QShowEvent* event)  {
    showLogData();
}

void BaseLogDialog::resizeEvent(QResizeEvent* event) {
    _logTextBox->setGeometry(0, TOP_BAR_HEIGHT, width(), height() - TOP_BAR_HEIGHT);
}

void BaseLogDialog::appendLogLine(QString logLine) {
    if (logLine.contains(_searchTerm, Qt::CaseInsensitive)) {
        int indexToBold = _logTextBox->document()->characterCount();
        _logTextBox->appendPlainText(logLine.trimmed());
        _highlighter->setBold(indexToBold);
    }
}

void BaseLogDialog::handleSearchButton() {
    _searchTextBox->setFocus();
}

void BaseLogDialog::handleSearchTextChanged(QString searchText) {
    if (searchText.isEmpty()) {
        return;
    }

    QTextCursor cursor = _logTextBox->textCursor();
    if (cursor.hasSelection()) {
        QString selectedTerm = cursor.selectedText();
        if (selectedTerm == searchText) {
          return;
        }
    }

    cursor.setPosition(0, QTextCursor::MoveAnchor);
    _logTextBox->setTextCursor(cursor);
    bool foundTerm = _logTextBox->find(searchText);

    if (!foundTerm) {
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        _logTextBox->setTextCursor(cursor);
    }

    _searchTerm = searchText;
    _highlighter->keyword = searchText;
    _highlighter->rehighlight();
}

void BaseLogDialog::toggleSearchPrev() {
    QTextCursor searchCursor = _logTextBox->textCursor();
    if (searchCursor.hasSelection()) {
        QString selectedTerm = searchCursor.selectedText();
        _logTextBox->find(selectedTerm, QTextDocument::FindBackward);
    } else {
        handleSearchTextChanged(_searchTextBox->text());
    }
}

void BaseLogDialog::toggleSearchNext() {
    QTextCursor searchCursor = _logTextBox->textCursor();
    if (searchCursor.hasSelection()) {
        QString selectedTerm = searchCursor.selectedText();
        _logTextBox->find(selectedTerm);
    } else {
        handleSearchTextChanged(_searchTextBox->text());
    }
}

void BaseLogDialog::showLogData() {
    _logTextBox->clear();
    _logTextBox->appendPlainText(getCurrentLog());
    _logTextBox->ensureCursorVisible();
    _highlighter->rehighlight();
}

void BaseLogDialog::updateSelection() {
    QTextCursor cursor = _logTextBox->textCursor();
    if (cursor.hasSelection()) {
        QString selectionTerm = cursor.selectedText();
        if (QString::compare(selectionTerm, _searchTextBox->text(), Qt::CaseInsensitive) != 0) {
            _searchTextBox->setText(selectionTerm);
        }
    }
}

Highlighter::Highlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    boldFormat.setFontWeight(FONT_WEIGHT);
    boldFormat.setForeground(BOLD_COLOR);
    keywordFormat.setForeground(HIGHLIGHT_COLOR);
}

void Highlighter::highlightBlock(const QString& text) {
    QRegExp expression(BOLD_PATTERN);

    int index = text.indexOf(expression, 0);

    while (index >= 0) {
        int length = expression.matchedLength();
        setFormat(index, length, boldFormat);
        index = text.indexOf(expression, index + length);
    }

    if (keyword.isNull() || keyword.isEmpty()) {
        return;
    }

    index = text.indexOf(keyword, 0, Qt::CaseInsensitive);
    int length = keyword.length();

    while (index >= 0) {
        setFormat(index, length, keywordFormat);
        index = text.indexOf(keyword, index + length, Qt::CaseInsensitive);
    }
}

void Highlighter::setBold(int indexToBold) {
    setFormat(indexToBold, TIME_STAMP_LENGTH, boldFormat);
}
