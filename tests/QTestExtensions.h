//
//  QTestExtensions.h
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

#include "GLMTestUtils.h"

// Implements several extensions to QtTest.
//
// Problems with QtTest:
// - QCOMPARE can compare float values (using a fuzzy compare), but uses an internal threshold
// that cannot be set explicitely (and we need explicit, adjustable error thresholds for our physics
// and math test code).
// - QFAIL takes a const char * failure message, and writing custom messages to it is complicated.
//
// To solve this, we have:
// - QCOMPARE_WITH_ABS_ERROR (compares floats, or *any other type* using explicitely defined error thresholds.
// To use it, you need to have a compareWithAbsError function ((T, T) -> V), and operator << for QTextStream).
// - QFAIL_WITH_MESSAGE("some " << streamed << " message"), which builds, writes to, and stringifies
// a QTextStream using black magic.
// - QCOMPARE_WITH_LAMBDA / QCOMPARE_WITH_FUNCTION, which implements QCOMPARE, but with a user-defined
// test function ((T, T) -> bool).
// - A simple framework to write additional custom test macros as needed (QCOMPARE is reimplemented
// from scratch using QTest::qFail, for example).
//


float getErrorDifference(const float& a, const float& b) {
    return fabsf(a - b);
}

// Generates a QCOMPARE-style failure message that can be passed to QTest::qFail.
//
// Formatting looks like this:
//  <qFail message> <failMessage....>
//      Actual:   (<stringified actual expr>)  : <actual value>
//      Expected: (<stringified expected expr>): <expected value>
//      < additional messages (should be separated by "\n\t" for indent formatting)>
//      Loc: [<file path....>(<linenum>)]
//
// Additional messages (after actual/expected) can be written using the std::function callback.
// If these messages span more than one line, wrap them with "\n\t" to get proper indentation / formatting)
//
template <typename T> inline
QString QTest_generateCompareFailureMessage (
    const char* failMessage,
    const T& actual, const T& expected,
    const char* actual_expr, const char* expected_expr,
    std::function<QTextStream& (QTextStream&)> writeAdditionalMessages
) {
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

// Generates a QCOMPARE-style failure message that can be passed to QTest::qFail.
//
// Formatting looks like this:
//  <qFail message> <failMessage....>
//      Actual:   (<stringified actual expr>)  : <actual value>
//      Expected: (<stringified expected expr>): <expected value>
//      Loc: [<file path....>(<linenum>)]
// (no message callback)
//
template <typename T> inline
QString QTest_generateCompareFailureMessage (
    const char* failMessage,
    const T& actual, const T& expected,
    const char* actual_expr, const char* expected_expr
) {
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

// Hacky function that can assemble a QString from a QTextStream via a callback
// (ie. stream operations w/out qDebug())
inline
QString makeMessageFromStream (std::function<void(QTextStream&)> writeMessage) {
    QString msg;
    QTextStream stream(&msg);
    writeMessage(stream);
    return msg;
}

inline
void QTest_failWithCustomMessage (
    std::function<void(QTextStream&)> writeMessage, int line, const char* file
) {
    QTest::qFail(qPrintable(makeMessageFromStream(writeMessage)), file, line);
}

// Equivalent to QFAIL, but takes a message that can be formatted using stream operators.
// Writes to a QTextStream internally, and calls QTest::qFail (the internal impl of QFAIL,
// with the current file and line number)
//
// example:
//  inline void foo () {
//      int thing = 2;
//      QFAIL_WITH_MESSAGE("Message " << thing << ";");
//  }
//
#define QFAIL_WITH_MESSAGE(...) \
do { \
    QTest_failWithCustomMessage([&](QTextStream& stream) { stream << __VA_ARGS__; }, __LINE__, __FILE__); \
    return; \
} while(0)

// Calls qFail using QTest_generateCompareFailureMessage.
// This is (usually) wrapped in macros, but if you call this directly you should return immediately to get QFAIL semantics.
template <typename T> inline
void QTest_failWithMessage(
    const char* failMessage,
    const T& actual, const T& expected,
    const char* actualExpr, const char* expectedExpr,
    int line, const char* file
) {
    QTest::qFail(qPrintable(QTest_generateCompareFailureMessage(
        failMessage, actual, expected, actualExpr, expectedExpr)), file, line);
}

// Calls qFail using QTest_generateCompareFailureMessage.
// This is (usually) wrapped in macros, but if you call this directly you should return immediately to get QFAIL semantics.
template <typename T> inline
void QTest_failWithMessage(
    const char* failMessage,
    const T& actual, const T& expected,
    const char* actualExpr, const char* expectedExpr,
    int line, const char* file,
    std::function<QTextStream& (QTextStream&)> writeAdditionalMessageLines
) {
    QTest::qFail(qPrintable(QTest_generateCompareFailureMessage(
        failMessage, actual, expected, actualExpr, expectedExpr, writeAdditionalMessageLines)), file, line);
}

// Implements QCOMPARE_WITH_ABS_ERROR
template <typename T, typename V> inline
bool QTest_compareWithAbsError(
    const T& actual, const T& expected,
    const char* actual_expr, const char* expected_expr,
    int line, const char* file,
    const V& epsilon
) {
    if (fabsf(getErrorDifference(actual, expected)) > fabsf(epsilon)) {
        QTest_failWithMessage(
            "Compared values are not the same (fuzzy compare)",
            actual, expected, actual_expr, expected_expr, line, file,
            [&] (QTextStream& stream) -> QTextStream& {
                return stream << "Err tolerance: " << getErrorDifference((actual), (expected)) << " > " << epsilon;
            });
        return false;
    }
    return true;
}

// Implements a fuzzy QCOMPARE using an explicit epsilon error value.
// If you use this, you must have the following functions defined for the types you're using:
//  <T, V>  V compareWithAbsError (const T& a, const T& b)         (should return the absolute, max difference between a and b)
//  <T>     QTextStream & operator << (QTextStream& stream, const T& value)
//
// Here's an implementation for glm::vec3:
//  inline  float  compareWithAbsError (const glm::vec3 & a, const glm::vec3 & b) {    // returns
//      return glm::distance(a, b);
//  }
//  inline QTextStream & operator << (QTextStream & stream, const T & v) {
//      return stream << "glm::vec3 { " << v.x << ", " << v.y << ", " << v.z << " }"
//  }
//
#define QCOMPARE_WITH_ABS_ERROR(actual, expected, epsilon) \
do { \
    if (!QTest_compareWithAbsError((actual), (expected), #actual, #expected, __LINE__, __FILE__, epsilon)) \
        return; \
} while(0)

// Implements QCOMPARE using an explicit, externally defined test function.
// The advantage of this (over a manual check or what have you) is that the values of actual and
// expected are printed in the event that the test fails.
//
//  testFunc(const T & actual, const T & expected) -> bool: true (test succeeds) | false (test fails)
//
#define QCOMPARE_WITH_FUNCTION(actual, expected, testFunc) \
do { \
    if (!(testFunc((actual), (expected)))) { \
        QTest_failWithMessage("Compared values are not the same", (actual), (expected), #actual, #expected, __LINE__, __FILE__); \
        return; \
    } \
} while (0)

// Implements QCOMPARE using an explicit, externally defined test function.
// Unlike QCOMPARE_WITH_FUNCTION, this func / closure takes no arguments (which is much more convenient
// if you're using a c++11 closure / lambda).
//
// usage:
//   QCOMPARE_WITH_LAMBDA(foo, expectedFoo, [&foo, &expectedFoo] () {
//      return foo->isFooish() && foo->fooishness() >= expectedFoo->fooishness();
//   });
// (fails if foo is not as fooish as expectedFoo)
//
#define QCOMPARE_WITH_LAMBDA(actual, expected, testClosure) \
do { \
    if (!(testClosure())) { \
        QTest_failWithMessage("Compared values are not the same", (actual), (expected), #actual, #expected, __LINE__, __FILE__); \
        return; \
    } \
} while (0)

// Same as QCOMPARE_WITH_FUNCTION, but with a custom fail message
#define QCOMPARE_WITH_FUNCTION_AND_MESSAGE(actual, expected, testfunc, failMessage) \
do { \
    if (!(testFunc((actual), (expected)))) { \
        QTest_failWithMessage((failMessage), (actual), (expected), #actual, #expected, __LINE__, __FILE__); \
        return; \
    } \
} while (0)

// Same as QCOMPARE_WITH_FUNCTION, but with a custom fail message
#define QCOMPARE_WITH_LAMBDA_AND_MESSAGE(actual, expected, testClosure, failMessage) \
do { \
    if (!(testClosure())) { \
        QTest_failWithMessage((failMessage), (actual), (expected), #actual, #expected, __LINE__, __FILE__); \
        return; \
    } \
} while (0)

#endif

#define QCOMPARE_WITH_EXPR(actual, expected, testExpr) \
    do { \
        if (!(testExpr)) { \
            QTest_failWithMessage("Compared values are not the same", (actual), (expected), #actual, #expected, __LINE__, __FILE__); \
        } \
    } while(0)


struct ByteData {
    ByteData (const char* data, size_t length)
        : data(data), length(length) {}
    const char* data;
    size_t length;
};

QTextStream & operator << (QTextStream& stream, const ByteData & wrapper) {
    // Print bytes as hex
    stream << QByteArray::fromRawData(wrapper.data, (int)wrapper.length).toHex();

    return stream;
}

bool compareData (const char* data, const char* expectedData, size_t length) {
    return memcmp(data, expectedData, length) == 0;
}

#define COMPARE_DATA(actual, expected, length) \
    QCOMPARE_WITH_EXPR((ByteData ( actual, length )), (ByteData ( expected, length )), compareData(actual, expected, length))


// Produces a relative error test for float usable QCOMPARE_WITH_LAMBDA.
inline auto errorTest (float actual, float expected, float acceptableRelativeError)
-> std::function<bool ()> {
    return [actual, expected, acceptableRelativeError] () {
        if (fabsf(expected) <= acceptableRelativeError) {
            return fabsf(actual - expected) < fabsf(acceptableRelativeError);
        }
        return fabsf((actual - expected) / expected) < fabsf(acceptableRelativeError);
    };
}

#define QCOMPARE_WITH_RELATIVE_ERROR(actual, expected, relativeError) \
    QCOMPARE_WITH_LAMBDA(actual, expected, errorTest(actual, expected, relativeError))


