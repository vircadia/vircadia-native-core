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
#include <QVector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class AnimExpression {
public:
    friend class AnimTests;
    AnimExpression(const QString& str);
protected:
    struct Token {
        enum Type {
            End = 0,
            Identifier,
            LiteralInt,
            LiteralFloat,
            LiteralVec3,
            LiteralVec4,
            LiteralQuat,
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
            Modulus,
            Comma,
            Error
        };
        Token(Type type) : type(type) {}
        Token(const QStringRef& strRef) : type(Type::Identifier), strVal(strRef.toString()) {}
        Token(int val) : type(Type::LiteralInt), intVal(val) {}
        Token(float val) : type(Type::LiteralFloat), floatVal(val) {}
        Type type = End;
        QString strVal;
        int intVal;
        float floatVal;
        glm::vec3 vec3Val;
        glm::vec4 vec4Val;
        glm::quat quatVal;
    };
    bool parseExpression(const QString& str);
    Token consumeToken(const QString& str, QString::const_iterator& iter) const;
    Token consumeIdentifier(const QString& str, QString::const_iterator& iter) const;
    Token consumeNumber(const QString& str, QString::const_iterator& iter) const;
    Token consumeAnd(const QString& str, QString::const_iterator& iter) const;
    Token consumeOr(const QString& str, QString::const_iterator& iter) const;
    Token consumeGreaterThan(const QString& str, QString::const_iterator& iter) const;
    Token consumeLessThan(const QString& str, QString::const_iterator& iter) const;
    Token consumeNot(const QString& str, QString::const_iterator& iter) const;

    QString _expression;
};

#endif

