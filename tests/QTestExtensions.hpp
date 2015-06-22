//
//  QTestExtensions.hpp
//  tests/
//
//  Created by Seiji Emery on 6/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_QTestExtensions_hpp
#define hifi_QTestExtensions_hpp

#include <QtTest/QtTest>
#include <functional>

// Adds some additional functionality to QtTest (eg. explicitely defined fuzzy comparison
// of float and custom data types), and some extension mechanisms to provide other
// test functionality as needed.

// QFUZZY_COMPARE (actual_expr, expected_expr, epsilon / error tolerance):
// Requires that you have two functions defined:
//
//      V fuzzyCompare (const T & a, const T & b)
//      QTextStream & operator << (const T & v)
//
//  fuzzyCompare should take a data type, T, and return the difference between two
//  such values / objects in terms of a second type, V (which should match the error
//  value type). For glm::vec3, T = glm::vec3, V = float, for example
//

// Generic function that reimplements the debugging output of a QCOMPARE failure via QFAIL.
// Use this to implement your own QCOMPARE-ish macros (see QEXPLICIT_FUZZY_COMPARE for
// more info).
// This version provides a callback to write additional messages.
// If the messages span more than one line, wrap them with '\n\t' to get proper indentation.
template <typename T>
inline QString QTest_generateCompareFailureMessage (const char * failMessage, const T & actual, const T & expected, const char * actual_expr, const char * expected_expr, std::function<QTextStream & (QTextStream &)> writeAdditionalMessages)
{
    QString s1 = actual_expr, s2 = expected_expr;
    int pad1_ = qMax(s2.length() - s1.length(), 0);
    int pad2_ = qMax(s1.length() - s2.length(), 0);
    
    QString pad1 = QString(")").rightJustified(pad1_, ' ');
    QString pad2 = QString(")").rightJustified(pad2_, ' ');
    
    QString msg;
    QTextStream stream (&msg);
    stream << failMessage << "\n\t"
        "Actual:   (" << actual_expr   << pad1 << ": " << actual   << "\n\t"
        "Expected: (" << expected_expr << pad2 << ": " << expected << "\n\t";
    writeAdditionalMessages(stream);
    return msg;
}

template <typename T>
inline QString QTest_generateCompareFailureMessage (const char * failMessage, const T & actual, const T & expected, const char * actual_expr, const char * expected_expr)
{
    QString s1 = actual_expr, s2 = expected_expr;
    int pad1_ = qMax(s2.length() - s1.length(), 0);
    int pad2_ = qMax(s1.length() - s2.length(), 0);
    
    QString pad1 = QString("): ").rightJustified(pad1_, ' ');
    QString pad2 = QString("): ").rightJustified(pad2_, ' ');
    
    QString msg;
    QTextStream stream (&msg);
    stream << failMessage << "\n\t"
        "Actual:   (" << actual_expr   << pad1 << actual   << "\n\t"
        "Expected: (" << expected_expr << pad2 << expected;
    return msg;
}

// Generates a QCOMPARE style failure message with custom arguments.
// This is expected to be wrapped in a macro (see QFUZZY_COMPARE), and it must
// actually return on failure (unless other functionality is desired).
template <typename T>
inline void QTest_failWithMessage(const char * failMessage, const T & actual, const T & expected, const char * actualExpr, const char * expectedExpr, int line, const char * file)
{
    QTest::qFail(qPrintable(QTest_generateCompareFailureMessage(failMessage, actual, expected, actualExpr, expectedExpr)), file, line);
}

// Generates a QCOMPARE style failure message with custom arguments.
// Writing additional lines (eg:)
//      Actual (<expr>): <actual value>
//      Expected (<expr>): <expected value>
//      <Your additional messages here...>
//      Loc: [<filepath...>(<line num>)]
// is provided via a lamdbda / closure that can write to the textstream.
// Be aware that newlines are actually "\n\t" (with this impl), so use that to get
// proper indenting (and add extra '\t's to get additional indentation).
template <typename T>
inline void QTest_failWithMessage(const char * failMessage, const T & actual, const T & expected, const char * actualExpr, const char * expectedExpr, int line, const char * file, std::function<QTextStream &(QTextStream&)> writeAdditionalMessageLines) {
    QTest::qFail(qPrintable(QTest_generateCompareFailureMessage(failMessage, actual, expected, actualExpr, expectedExpr, writeAdditionalMessageLines)), file, line);
}

template <typename T, typename V>
inline auto QTest_fuzzyCompare(const T & actual, const T & expected, const char * actual_expr, const char * expected_expr, int line, const char * file, const V & epsilon) -> decltype(fuzzyCompare(actual, expected))
{
    if (fuzzyCompare(actual, expected) > epsilon) {
        QTest::qFail(qPrintable(QTest_generateCompareFailureMessage("Compared values are not the same (fuzzy compare)", actual, expected, actual_expr, expected_expr,
            [&] (QTextStream & stream) -> QTextStream & {
                return stream << "Err tolerance: " << fuzzyCompare((actual), (expected)) << " > " << epsilon;
            })), file, line);
        return false;
    }
    return true;
}

#define QFUZZY_COMPARE(actual, expected, epsilon) \
do { \
    if (!QTest_fuzzyCompare(actual, expected, #actual, #expected, __LINE__, __FILE__, epsilon)) \
        return; \
} while(0)

#endif
