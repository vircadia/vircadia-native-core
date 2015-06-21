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
// of float and custom data types), and some extension mechanisms to provide other new
// functionality as needed.


// Generic function that reimplements the debugging output of a QCOMPARE failure via QFAIL.
// Use this to implement your own QCOMPARE-ish macros (see QEXPLICIT_FUZZY_COMPARE for
// more info).
template <typename T>
void QTest_failWithMessage (const T & actual, const T & expected, const char * actual_expr, const char * expected_expr, int line, const char * file)
{
    
}

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
    
    QString pad1 = QString(")").rightJustified(pad1_, ' ');
    QString pad2 = QString(")").rightJustified(pad2_, ' ');
    
    QString msg;
    QTextStream stream (&msg);
    stream << failMessage << "\n\t"
    "Actual:   (" << actual_expr   << pad1 << ": " << actual   << "\n\t"
    "Expected: (" << expected_expr << pad2 << ": " << expected;
    return msg;
}

template <typename T, typename V>
inline bool QTest_fuzzyCompare(const T & actual, const T & expected, const char * actual_expr, const char * expected_expr, int line, const char * file, const V & epsilon)
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

// Note: this generates a message that looks something like the following:
//  FAIL!  : BulletUtilTests::fooTest() Compared values are not the same (fuzzy compare)
//      Actual    (foo * 3):   glm::vec3 { 1, 0, 3 }
//      Expected  (bar + baz): glm::vec3 { 2, 0, 5 }
//      Error Tolerance: 2.23607 > 1
//  Loc: [/Users/semery/hifi/tests/physics/src/BulletUtilTests.cpp(68)]
//
// The last line (and the FAIL! message up to "Compared values...") are generated automatically by
// QFAIL. It is possible to generate these manually via __FILE__ and __LINE__, but QFAIL does this
// already so there's no point in reimplementing it. However, since we are using QFAIL to generate
// our line number for us, it's important that it's actually invoked on the same line as the thing
// that calls it -- hence the elaborate macro(s) above (since the result is *technically* one line)
//

#endif
