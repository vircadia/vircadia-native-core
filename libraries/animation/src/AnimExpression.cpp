//
//  AnimExpression.cpp
//
//  Created by Anthony J. Thibault on 11/1/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <StreamUtils.h>
#include <QRegExp>

#include "AnimExpression.h"
#include "AnimationLogging.h"

AnimExpression::AnimExpression(const QString& str) :
    _expression(str) {
    parseExpression(_expression);
}

bool AnimExpression::parseExpression(const QString& str) {
    Token token(Token::End);
    auto iter = str.begin();
    do {
        token = consumeToken(str, iter);
        switch(token.type) {
        case Token::Error:
        case Token::End:
            return false;
        }
    } while(iter != str.end());
}

AnimExpression::Token AnimExpression::consumeToken(const QString& str, QString::const_iterator& iter) const {
    while (iter != str.end()) {
        if (iter->isSpace()) {
            ++iter;
        } else if (iter->isLetter()) {
            return consumeIdentifier(str, iter);
        } else if (iter->isDigit()) {
            return consumeNumber(str, iter);
        } else {
            switch (iter->unicode()) {
            case '&': return consumeAnd(str, iter);
            case '|': return consumeOr(str, iter);
            case '>': return consumeGreaterThan(str, iter);
            case '<': return consumeLessThan(str, iter);
            case '(': ++iter; return Token(Token::LeftParen);
            case ')': ++iter; return Token(Token::RightParen);
            case '!': return consumeNot(str, iter);
            case '-': ++iter; return Token(Token::Minus);
            case '+': ++iter; return Token(Token::Plus);
            case '*': ++iter; return Token(Token::Multiply);
            case '%': ++iter; return Token(Token::Modulus);
            case ',': ++iter; return Token(Token::Comma);
            default:
                qCCritical(animation) << "AnimExpression: unexpected char" << *iter << "at index " << (int)(iter - str.begin());
                return Token(Token::Error);
            }
        }
    }
    return Token(Token::End);
}

AnimExpression::Token AnimExpression::consumeIdentifier(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->isLetter());
    auto begin = iter;
    while (iter->isLetter() && iter != str.end()) {
        ++iter;
    }
    int pos = (int)(begin - str.begin());
    int len = (int)(iter - begin);
    return Token(QStringRef(const_cast<const QString*>(&str), pos, len));
}

// TODO: not very efficient or accruate, but it's close enough for now.
static float computeFractionalPart(int fractionalPart)
{
    float frac = (float)fractionalPart;
    while (fractionalPart) {
        fractionalPart /= 10;
        frac /= 10.0f;
    }
    return frac;
}

static float computeFloat(int whole, int fraction) {
    return (float)whole + computeFractionalPart(fraction);
}

AnimExpression::Token AnimExpression::consumeNumber(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->isDigit());
    auto begin = iter;
    while (iter->isDigit() && iter != str.end()) {
        ++iter;
    }

    // parse whole integer part
    int pos = (int)(begin - str.begin());
    int len = (int)(iter - begin);
    QString sub = QStringRef(const_cast<const QString*>(&str), pos, len).toString();
    int whole = sub.toInt();

    // parse optional fractional part
    if (iter->unicode() == '.') {
        iter++;
        auto begin = iter;
        while (iter->isDigit() && iter != str.end()) {
            ++iter;
        }

        int pos = (int)(begin - str.begin());
        int len = (int)(iter - begin);
        QString sub = QStringRef(const_cast<const QString*>(&str), pos, len).toString();
        int fraction = sub.toInt();

        return Token(computeFloat(whole, fraction));

    } else {
        return Token(whole);
    }
}

AnimExpression::Token AnimExpression::consumeAnd(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->unicode() == '&');
    iter++;
    if (iter->unicode() == '&') {
        iter++;
        return Token(Token::And);
    } else {
        qCCritical(animation) << "AnimExpression: unexpected char" << *iter << "at index " << (int)(iter - str.begin());
        return Token(Token::Error);
    }
}

AnimExpression::Token AnimExpression::consumeOr(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->unicode() == '|');
    iter++;
    if (iter->unicode() == '|') {
        iter++;
        return Token(Token::Or);
    } else {
        qCCritical(animation) << "AnimExpression: unexpected char" << *iter << "at index " << (int)(iter - str.begin());
        return Token(Token::Error);
    }
}

AnimExpression::Token AnimExpression::consumeGreaterThan(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->unicode() == '>');
    iter++;
    if (iter->unicode() == '=') {
        iter++;
        return Token(Token::GreaterThanEqual);
    } else {
        return Token(Token::GreaterThan);
    }
}

AnimExpression::Token AnimExpression::consumeLessThan(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->unicode() == '<');
    iter++;
    if (iter->unicode() == '=') {
        iter++;
        return Token(Token::LessThanEqual);
    } else {
        return Token(Token::LessThan);
    }
}

AnimExpression::Token AnimExpression::consumeNot(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->unicode() == '!');
    iter++;
    if (iter->unicode() == '=') {
        iter++;
        return Token(Token::NotEqual);
    } else {
        return Token(Token::Not);
    }
}
