//
//  ScriptHighlighting.h
//  interface/src
//
//  Created by Thijs Wenker on 4/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptHighlighting_h
#define hifi_ScriptHighlighting_h

#include <QSyntaxHighlighter>

class ScriptHighlighting : public QSyntaxHighlighter {
    Q_OBJECT

public:
    ScriptHighlighting(QTextDocument* parent = 0);

    enum BlockState {
        BlockStateClean,
        BlockStateInMultiComment
    };

protected:
    void highlightBlock(const QString& text);
    void highlightKeywords(const QString& text);
    void formatComments(const QString& text);
    void formatQoutedText(const QString& text);
    void formatNumbers(const QString& text);
    void formatTrueFalse(const QString text);

private:
    QRegExp keywordRegex;
    QRegExp qoutedTextRegex;
    QRegExp multiLineCommentBegin;
    QRegExp multiLineCommentEnd;
    QRegExp numberRegex;
    QRegExp singleLineComment;
    QRegExp truefalseRegex;
};

#endif // hifi_ScriptHighlighting_h
