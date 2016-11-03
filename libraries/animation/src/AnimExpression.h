//
//  AnimExpression.h
//
//  Created by Anthony J. Thibault on 11/1/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimExpression
#define hifi_AnimExpression

#include <QString>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stack>
#include <vector>
#include "AnimVariant.h"

class AnimExpression {
public:
    friend class AnimTests;
    explicit AnimExpression(const QString& str);
protected:
    struct Token {
        enum Type {
            End = 0,
            Identifier,
            Bool,
            Int,
            Float,
            And,
            Or,
            GreaterThan,
            GreaterThanEqual,
            LessThan,
            LessThanEqual,
            Equal,
            NotEqual,
            LeftParen,
            RightParen,
            Not,
            Minus,
            Plus,
            Multiply,
            Divide,
            Modulus,
            Comma,
            Error
        };
        explicit Token(Type type) : type {type} {}
        explicit Token(const QStringRef& strRef) : type {Type::Identifier}, strVal {strRef.toString()} {}
        explicit Token(int val) : type {Type::Int}, intVal {val} {}
        explicit Token(bool val) : type {Type::Bool}, intVal {val} {}
        explicit Token(float val) : type {Type::Float}, floatVal {val} {}
        Type type {End};
        QString strVal;
        int intVal {0};
        float floatVal {0.0f};
    };

    struct OpCode {
        enum Type {
            Identifier,
            Bool,
            Int,
            Float,
            And,
            Or,
            GreaterThan,
            GreaterThanEqual,
            LessThan,
            LessThanEqual,
            Equal,
            NotEqual,
            Not,
            Subtract,
            Add,
            Multiply,
            Divide,
            Modulus,
            UnaryMinus
        };
        explicit OpCode(Type type) : type {type} {}
        explicit OpCode(const QStringRef& strRef) : type {Type::Identifier}, strVal {strRef.toString()} {}
        explicit OpCode(const QString& str) : type {Type::Identifier}, strVal {str} {}
        explicit OpCode(int val) : type {Type::Int}, intVal {val} {}
        explicit OpCode(bool val) : type {Type::Bool}, intVal {(int)val} {}
        explicit OpCode(float val) : type {Type::Float}, floatVal {val} {}

        bool coerceBool(const AnimVariantMap& map) const {
            if (type == Int || type == Bool) {
                return intVal != 0;
            } else if (type == Identifier) {
                return map.lookup(strVal, false);
            } else {
                return true;
            }
        }

        Type type {Int};
        QString strVal;
        int intVal {0};
        float floatVal {0.0f};
    };

    void unconsumeToken(const Token& token);
    Token consumeToken(const QString& str, QString::const_iterator& iter) const;
    Token consumeIdentifier(const QString& str, QString::const_iterator& iter) const;
    Token consumeNumber(const QString& str, QString::const_iterator& iter) const;
    Token consumeAnd(const QString& str, QString::const_iterator& iter) const;
    Token consumeOr(const QString& str, QString::const_iterator& iter) const;
    Token consumeGreaterThan(const QString& str, QString::const_iterator& iter) const;
    Token consumeLessThan(const QString& str, QString::const_iterator& iter) const;
    Token consumeNot(const QString& str, QString::const_iterator& iter) const;

    bool parseExpr(const QString& str, QString::const_iterator& iter);
    bool parseExprPrime(const QString& str, QString::const_iterator& iter);
    bool parseTerm(const QString& str, QString::const_iterator& iter);
    bool parseTermPrime(const QString& str, QString::const_iterator& iter);
    bool parseUnary(const QString& str, QString::const_iterator& iter);
    bool parseFactor(const QString& str, QString::const_iterator& iter);

    OpCode evaluate(const AnimVariantMap& map) const;
    void evalAnd(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalOr(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalGreaterThan(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalGreaterThanEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalLessThan(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalLessThanEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalNotEqual(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalNot(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalSubtract(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalAdd(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void add(int lhs, const OpCode& rhs, std::stack<OpCode>& stack) const;
    void add(float lhs, const OpCode& rhs, std::stack<OpCode>& stack) const;
    void evalMultiply(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void mul(int lhs, const OpCode& rhs, std::stack<OpCode>& stack) const;
    void mul(float lhs, const OpCode& rhs, std::stack<OpCode>& stack) const;
    void evalDivide(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalModulus(const AnimVariantMap& map, std::stack<OpCode>& stack) const;
    void evalUnaryMinus(const AnimVariantMap& map, std::stack<OpCode>& stack) const;

    OpCode coerseToValue(const AnimVariantMap& map, const OpCode& opCode) const;

    QString _expression;
    mutable std::stack<Token> _tokenStack;    // TODO: remove, only needed during parsing
    std::vector<OpCode> _opCodes;

#ifndef NDEBUG
    void dumpOpCodes() const;
#endif
};

#endif

