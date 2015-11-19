//
//  ScriptHighlighting.cpp
//  interface/src
//
//  Created by Thijs Wenker on 4/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptHighlighting.h"
#include <QTextDocument>

ScriptHighlighting::ScriptHighlighting(QTextDocument* parent) :
    QSyntaxHighlighter(parent)
{
    _keywordRegex = QRegExp("\\b(break|case|catch|continue|debugger|default|delete|do|else|finally|for|function|if|in|instanceof|new|return|switch|this|throw|try|typeof|var|void|while|with)\\b");
    _quotedTextRegex = QRegExp("(\"[^\"]*(\"){0,1}|\'[^\']*(\'){0,1})");
    _multiLineCommentBegin = QRegExp("/\\*");
    _multiLineCommentEnd = QRegExp("\\*/");
    _numberRegex = QRegExp("[0-9]+(\\.[0-9]+){0,1}");
    _singleLineComment = QRegExp("//[^\n]*");
    _truefalseRegex = QRegExp("\\b(true|false)\\b");
    _alphacharRegex = QRegExp("[A-Za-z]");
}

void ScriptHighlighting::highlightBlock(const QString& text) {
    this->highlightKeywords(text);
    this->formatNumbers(text);
    this->formatTrueFalse(text);
    this->formatQuotedText(text);
    this->formatComments(text);
}

void ScriptHighlighting::highlightKeywords(const QString& text) {
    int index = _keywordRegex.indexIn(text);
    while (index >= 0) {
        int length = _keywordRegex.matchedLength();
        setFormat(index, length, Qt::blue);
        index = _keywordRegex.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatComments(const QString& text) {

    setCurrentBlockState(BlockStateClean);

    int start = (previousBlockState() != BlockStateInMultiComment) ? text.indexOf(_multiLineCommentBegin) : 0;

    while (start > -1) {
        int end = text.indexOf(_multiLineCommentEnd, start);
        int length = (end == -1 ? text.length() : (end + _multiLineCommentEnd.matchedLength())) - start;
        setFormat(start, length, Qt::lightGray);
        start = text.indexOf(_multiLineCommentBegin, start + length);
        if (end == -1) {
            setCurrentBlockState(BlockStateInMultiComment);
        }
    }

    int index = _singleLineComment.indexIn(text);
    while (index >= 0) {
        int length = _singleLineComment.matchedLength();
        int quoted_index = _quotedTextRegex.indexIn(text);
        bool valid = true;
        while (quoted_index >= 0 && valid) {
            int quoted_length = _quotedTextRegex.matchedLength();
            if (quoted_index <= index && index <= (quoted_index + quoted_length)) {
                valid = false;
            }
            quoted_index = _quotedTextRegex.indexIn(text, quoted_index + quoted_length);
        }

        if (valid) {
            setFormat(index, length, Qt::lightGray);
        }
        index = _singleLineComment.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatQuotedText(const QString& text){
    int index = _quotedTextRegex.indexIn(text);
    while (index >= 0) {
        int length = _quotedTextRegex.matchedLength();
        setFormat(index, length, Qt::red);
        index = _quotedTextRegex.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatNumbers(const QString& text){
    int index = _numberRegex.indexIn(text);
    while (index >= 0) {
        int length = _numberRegex.matchedLength();
        if (index == 0 || _alphacharRegex.indexIn(text, index - 1) != (index - 1)) {
            setFormat(index, length, Qt::green);
        }
        index = _numberRegex.indexIn(text, index + length);
    }
}

void ScriptHighlighting::formatTrueFalse(const QString& text){
    int index = _truefalseRegex.indexIn(text);
    while (index >= 0) {
        int length = _truefalseRegex.matchedLength();
        QFont* font = new QFont(this->document()->defaultFont());
        font->setBold(true);
        setFormat(index, length, *font);
        index = _truefalseRegex.indexIn(text, index + length);
    }
}
