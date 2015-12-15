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
    auto iter = str.begin();
    parseExpr(_expression, iter);
    while(!_tokenStack.empty()) {
        _tokenStack.pop();
    }
}

//
// Tokenizer
//

void AnimExpression::unconsumeToken(const Token& token) {
    _tokenStack.push(token);
}

AnimExpression::Token AnimExpression::consumeToken(const QString& str, QString::const_iterator& iter) const {
    if (!_tokenStack.empty()) {
        Token top = _tokenStack.top();
        _tokenStack.pop();
        return top;
    } else {
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
                case '/': ++iter; return Token(Token::Divide);
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
}

AnimExpression::Token AnimExpression::consumeIdentifier(const QString& str, QString::const_iterator& iter) const {
    assert(iter != str.end());
    assert(iter->isLetter());
    auto begin = iter;
    while ((iter->isLetter() || iter->isDigit()) && iter != str.end()) {
        ++iter;
    }
    int pos = (int)(begin - str.begin());
    int len = (int)(iter - begin);

    QStringRef stringRef(const_cast<const QString*>(&str), pos, len);
    if (stringRef == "true") {
        return Token(true);
    } else if (stringRef == "false") {
        return Token(false);
    } else {
        return Token(stringRef);
    }
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

//
// Parser
//

/*
Expr   → Term Expr'
Expr'  → '||' Term Expr'
       | ε
Term   → Unary Term'
Term'  → '&&' Unary Term'
       | ε
Unary  → '!' Unary
       | Factor
Factor → INT
       | BOOL
       | FLOAT
       | IDENTIFIER
       | '(' Expr ')'
*/

// Expr → Term Expr'
bool AnimExpression::parseExpr(const QString& str, QString::const_iterator& iter) {
    if (!parseTerm(str, iter)) {
        return false;
    }
    if (!parseExprPrime(str, iter)) {
        return false;
    }
    return true;
}

// Expr' → '||' Term Expr' | ε
bool AnimExpression::parseExprPrime(const QString& str, QString::const_iterator& iter) {
    auto token = consumeToken(str, iter);
    if (token.type == Token::Or) {
        if (!parseTerm(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        if (!parseExprPrime(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        _opCodes.push_back(OpCode {OpCode::Or});
        return true;
    } else {
        unconsumeToken(token);
        return true;
    }
}

// Term → Unary Term'
bool AnimExpression::parseTerm(const QString& str, QString::const_iterator& iter) {
    if (!parseUnary(str, iter)) {
        return false;
    }
    if (!parseTermPrime(str, iter)) {
        return false;
    }
    return true;
}

// Term' → '&&' Unary Term' | ε
bool AnimExpression::parseTermPrime(const QString& str, QString::const_iterator& iter) {
    auto token = consumeToken(str, iter);
    if (token.type == Token::And) {
        if (!parseUnary(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        if (!parseTermPrime(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        _opCodes.push_back(OpCode {OpCode::And});
        return true;
    } else {
        unconsumeToken(token);
        return true;
    }
}

// Unary → '!' Unary | Factor
bool AnimExpression::parseUnary(const QString& str, QString::const_iterator& iter) {

    auto token = consumeToken(str, iter);
    if (token.type == Token::Not) {
        if (!parseUnary(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        _opCodes.push_back(OpCode {OpCode::Not});
        return true;
    }
    unconsumeToken(token);

    return parseFactor(str, iter);
}


// Factor → INT | BOOL | FLOAT | IDENTIFIER | '(' Expr ')'
bool AnimExpression::parseFactor(const QString& str, QString::const_iterator& iter) {
    auto token = consumeToken(str, iter);
    if (token.type == Token::Int) {
        _opCodes.push_back(OpCode {token.intVal});
        return true;
    } else if (token.type == Token::Bool) {
        _opCodes.push_back(OpCode {(bool)token.intVal});
        return true;
    } else if (token.type == Token::Float) {
        _opCodes.push_back(OpCode {token.floatVal});
        return true;
    } else if (token.type == Token::Identifier) {
        _opCodes.push_back(OpCode {token.strVal});
        return true;
    } else if (token.type == Token::LeftParen) {
        if (!parseExpr(str, iter)) {
            unconsumeToken(token);
            return false;
        }
        auto nextToken = consumeToken(str, iter);
        if (nextToken.type != Token::RightParen) {
            unconsumeToken(nextToken);
            unconsumeToken(token);
            return false;
        }
        return true;
    } else {
        unconsumeToken(token);
        return false;
    }
}

//
// Evaluator
//

AnimExpression::OpCode AnimExpression::evaluate(const AnimVariantMap& map) const {
    std::stack<OpCode> stack;
    for (auto& opCode : _opCodes) {
        switch (opCode.type) {
        case OpCode::Identifier:
        case OpCode::Int:
        case OpCode::Float:
        case OpCode::Bool:
            stack.push(opCode);
            break;
        case OpCode::And: evalAnd(map, stack); break;
        case OpCode::Or: evalOr(map, stack); break;
        case OpCode::GreaterThan: evalGreaterThan(map, stack); break;
        case OpCode::GreaterThanEqual: evalGreaterThanEqual(map, stack); break;
        case OpCode::LessThan: evalLessThan(map, stack); break;
        case OpCode::LessThanEqual: evalLessThanEqual(map, stack); break;
        case OpCode::Equal: evalEqual(map, stack); break;
        case OpCode::NotEqual: evalNotEqual(map, stack); break;
        case OpCode::Not: evalNot(map, stack); break;
        case OpCode::Subtract: evalSubtract(map, stack); break;
        case OpCode::Add: evalAdd(map, stack); break;
        case OpCode::Multiply: evalMultiply(map, stack); break;
        case OpCode::Divide: evalDivide(map, stack); break;
        case OpCode::Modulus: evalModulus(map, stack); break;
        case OpCode::UnaryMinus: evalUnaryMinus(map, stack); break;
        }
    }
    return coerseToValue(map, stack.top());
}

#define POP_BOOL(NAME)                           \
    const OpCode& NAME##_temp = stack.top();     \
    bool NAME = NAME##_temp.coerceBool(map);     \
    stack.pop()

#define PUSH(EXPR)                              \
    stack.push(OpCode {(EXPR)})

void AnimExpression::evalAnd(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    POP_BOOL(lhs);
    POP_BOOL(rhs);
    PUSH(lhs && rhs);
}

void AnimExpression::evalOr(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    POP_BOOL(lhs);
    POP_BOOL(rhs);
    PUSH(lhs || rhs);
}

void AnimExpression::evalGreaterThan(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalGreaterThanEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalLessThan(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalLessThanEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalNotEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(false);
}

void AnimExpression::evalNot(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    POP_BOOL(rhs);
    PUSH(!rhs);
}

void AnimExpression::evalSubtract(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(0.0f);
}

void AnimExpression::add(int lhs, const OpCode& rhs, std::stack<OpCode>& stack) const {
    switch (rhs.type) {
    case OpCode::Bool:
    case OpCode::Int:
        PUSH(lhs + rhs.intVal);
        break;
    case OpCode::Float:
        PUSH((float)lhs + rhs.floatVal);
        break;
    default:
        PUSH(lhs);
    }
}

void AnimExpression::add(float lhs, const OpCode& rhs, std::stack<OpCode>& stack) const {
    switch (rhs.type) {
    case OpCode::Bool:
    case OpCode::Int:
        PUSH(lhs + (float)rhs.intVal);
        break;
    case OpCode::Float:
        PUSH(lhs + rhs.floatVal);
        break;
    default:
        PUSH(lhs);
    }
}

void AnimExpression::evalAdd(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = coerseToValue(map, stack.top());
    stack.pop();
    OpCode rhs = coerseToValue(map, stack.top());
    stack.pop();

    switch (lhs.type) {
    case OpCode::Bool:
        add(lhs.intVal, rhs, stack);
        break;
    case OpCode::Int:
        add(lhs.intVal, rhs, stack);
        break;
    case OpCode::Float:
        add(lhs.floatVal, rhs, stack);
        break;
    default:
        add(0, rhs, stack);
        break;
    }
}

void AnimExpression::evalMultiply(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = coerseToValue(map, stack.top());
    stack.pop();
    OpCode rhs = coerseToValue(map, stack.top());
    stack.pop();

    switch(lhs.type) {
    case OpCode::Bool:
        mul(lhs.intVal, rhs, stack);
        break;
    case OpCode::Int:
        mul(lhs.intVal, rhs, stack);
        break;
    case OpCode::Float:
        mul(lhs.floatVal, rhs, stack);
        break;
    default:
        mul(0, rhs, stack);
        break;
    }
}

void AnimExpression::mul(int lhs, const OpCode& rhs, std::stack<OpCode>& stack) const {
    switch (rhs.type) {
    case OpCode::Bool:
    case OpCode::Int:
        PUSH(lhs * rhs.intVal);
        break;
    case OpCode::Float:
        PUSH((float)lhs * rhs.floatVal);
        break;
    default:
        PUSH(lhs);
    }
}

void AnimExpression::mul(float lhs, const OpCode& rhs, std::stack<OpCode>& stack) const {
    switch (rhs.type) {
    case OpCode::Bool:
    case OpCode::Int:
        PUSH(lhs * (float)rhs.intVal);
        break;
    case OpCode::Float:
        PUSH(lhs * rhs.floatVal);
        break;
    default:
        PUSH(lhs);
    }
}

void AnimExpression::evalDivide(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH(0.0f);
}

void AnimExpression::evalModulus(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode lhs = stack.top(); stack.pop();
    OpCode rhs = stack.top(); stack.pop();

    // TODO:
    PUSH((int)0);
}

void AnimExpression::evalUnaryMinus(const AnimVariantMap& map, std::stack<OpCode>& stack) const {
    OpCode rhs = stack.top(); stack.pop();

    switch (rhs.type) {
    case OpCode::Identifier: {
        const AnimVariant& var = map.get(rhs.strVal);
        switch (var.getType()) {
        case AnimVariant::Type::Bool:
            qCWarning(animation) << "AnimExpression: type missmatch for unary minus, expected a number not a bool";
            // interpret this as boolean not.
            PUSH(!var.getBool());
            break;
        case AnimVariant::Type::Int:
            PUSH(-var.getInt());
            break;
        case AnimVariant::Type::Float:
            PUSH(-var.getFloat());
            break;
        default:
            // TODO: Vec3, Quat are unsupported
            assert(false);
            PUSH(false);
            break;
        }
    }
    case OpCode::Int:
        PUSH(-rhs.intVal);
        break;
    case OpCode::Float:
        PUSH(-rhs.floatVal);
        break;
    case OpCode::Bool:
        qCWarning(animation) << "AnimExpression: type missmatch for unary minus, expected a number not a bool";
        // interpret this as boolean not.
        PUSH(!rhs.coerceBool(map));
        break;
    default:
        qCCritical(animation) << "AnimExpression: ERRROR for unary minus, expected a number, type = " << rhs.type;
        assert(false);
        PUSH(false);
        break;
    }
}

AnimExpression::OpCode AnimExpression::coerseToValue(const AnimVariantMap& map, const OpCode& opCode) const {
    switch (opCode.type) {
    case OpCode::Identifier:
        {
            const AnimVariant& var = map.get(opCode.strVal);
            switch (var.getType()) {
            case AnimVariant::Type::Bool:
                return OpCode((bool)var.getBool());
                break;
            case AnimVariant::Type::Int:
                return OpCode(var.getInt());
                break;
            case AnimVariant::Type::Float:
                return OpCode(var.getFloat());
                break;
            default:
                // TODO: Vec3, Quat are unsupported
                assert(false);
                return OpCode(0);
                break;
            }
        }
        break;
    case OpCode::Bool:
    case OpCode::Int:
    case OpCode::Float:
        return opCode;
    default:
        qCCritical(animation) << "AnimExpression: ERROR expected a number, type = " << opCode.type;
        assert(false);
        return OpCode(0);
        break;
    }
}

#ifndef NDEBUG
void AnimExpression::dumpOpCodes() const {
    QString tmp;
    for (auto& op : _opCodes) {
        switch (op.type) {
        case OpCode::Identifier: tmp += QString(" %1").arg(op.strVal); break;
        case OpCode::Bool: tmp += QString(" %1").arg(op.intVal ? "true" : "false"); break;
        case OpCode::Int: tmp += QString(" %1").arg(op.intVal); break;
        case OpCode::Float: tmp += QString(" %1").arg(op.floatVal); break;
        case OpCode::And: tmp += " &&"; break;
        case OpCode::Or: tmp += " ||"; break;
        case OpCode::GreaterThan: tmp += " >"; break;
        case OpCode::GreaterThanEqual: tmp += " >="; break;
        case OpCode::LessThan: tmp += " <"; break;
        case OpCode::LessThanEqual: tmp += " <="; break;
        case OpCode::Equal: tmp += " =="; break;
        case OpCode::NotEqual: tmp += " !="; break;
        case OpCode::Not: tmp += " !"; break;
        case OpCode::Subtract: tmp += " -"; break;
        case OpCode::Add: tmp += " +"; break;
        case OpCode::Multiply: tmp += " *"; break;
        case OpCode::Divide: tmp += " /"; break;
        case OpCode::Modulus: tmp += " %"; break;
        case OpCode::UnaryMinus: tmp += " unary-"; break;
        default: tmp += " ???"; break;
        }
    }
    qCDebug(animation).nospace().noquote() << "opCodes =" << tmp;
    qCDebug(animation).resetFormat();
}
#endif
