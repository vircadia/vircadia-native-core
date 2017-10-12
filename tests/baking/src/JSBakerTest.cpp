//
//  JSBakerTest.cpp
//  tests/networking/src
//
//  Created by Utkarsh Gautam on 09/26/17.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSBakerTest.h"
QTEST_MAIN(JSBakerTest)

void JSBakerTest::setTestCases() {
    // Test cases contain a std::pair(input, desiredOutput)

    _testCases.emplace_back("var a=1;", "var a=1;");
    _testCases.emplace_back("var a=1;//single line comment\nvar b=2;", "var a=1;var b=2;");
    _testCases.emplace_back("a\rb", "a\nb");
    _testCases.emplace_back("a/*multi\n line \n comment*/ b", "ab");
    _testCases.emplace_back("a/b", "a/b");
    _testCases.emplace_back("var a =      1;", "var a=1;"); // Multiple spaces omitted
    _testCases.emplace_back("var a=\t\t\t1;", "var a=1;"); // Multiple tabs omitted

                                                              // Cases for space not omitted
    _testCases.emplace_back("var x", "var x");
    _testCases.emplace_back("a '", "a '");
    _testCases.emplace_back("a $", "a $");
    _testCases.emplace_back("a _", "a _");
    _testCases.emplace_back("a /", "a /");
    _testCases.emplace_back("a 1", "a 1");
    _testCases.emplace_back("1 a", "1 a");
    _testCases.emplace_back("$ a", "$ a");
    _testCases.emplace_back("_ a", "_ a");
    _testCases.emplace_back("/ a", "/ a");
    _testCases.emplace_back("$ $", "$ $");
    _testCases.emplace_back("_ _", "_ _");
    _testCases.emplace_back("/ /", "/ /");

    _testCases.emplace_back("a\n\n\n\nb", "a\nb"); // Skip multiple new lines
    _testCases.emplace_back("a\n\n b", "a\nb"); // Skip multiple new lines followed by whitespace
    _testCases.emplace_back("a\n\n  b", "a\nb"); // Skip multiple new lines followed by tab

                                                 //Cases for new line not omitted
    _testCases.emplace_back("a\nb", "a\nb");
    _testCases.emplace_back("a\n9", "a\n9");
    _testCases.emplace_back("9\na", "9\na");
    _testCases.emplace_back("a\n$", "a\n$");
    _testCases.emplace_back("a\n[", "a\n[");
    _testCases.emplace_back("a\n{", "a\n{");
    _testCases.emplace_back("a\n(", "a\n(");
    _testCases.emplace_back("a\n+", "a\n+");
    _testCases.emplace_back("a\n'", "a\n'");
    _testCases.emplace_back("a\n-", "a\n-");
    _testCases.emplace_back("$\na", "$\na");
    _testCases.emplace_back("$\na", "$\na");
    _testCases.emplace_back("_\na", "_\na");
    _testCases.emplace_back("]\na", "]\na");
    _testCases.emplace_back("}\na", "}\na");
    _testCases.emplace_back(")\na", ")\na");
    _testCases.emplace_back("+\na", "+\na");
    _testCases.emplace_back("-\na", "-\na");

    // Cases to check quoted strings are not modified
    _testCases.emplace_back("'abcd1234$%^&[](){}'\na", "'abcd1234$%^&[](){}'\na");
    _testCases.emplace_back("\"abcd1234$%^&[](){}\"\na", "\"abcd1234$%^&[](){}\"\na");
    _testCases.emplace_back("`abcd1234$%^&[](){}`\na", "`abcd1234$%^&[](){}`a");

    // Edge Cases

    //No semicolon to terminate an expression, instead a new line used for termination
    _testCases.emplace_back("var x=5\nvar y=6;", "var x=5\nvar y=6;");
    
     //a + ++b is minified as a+ ++b.
    _testCases.emplace_back("a + ++b", "a + ++b");

    //a - --b is minified as a- --b.
    _testCases.emplace_back("a - --b", "a - --b");
}

void JSBakerTest::testJSBaking() {

    for (int i = 0;i < _testCases.size();i++) {
        QByteArray output;
        auto input = _testCases.at(i).first;
        JSBaker::bakeJS(input, output);

        auto desiredOutput = _testCases.at(i).second;
        QCOMPARE(output, desiredOutput);
    }
}
