//
//  CrashHelpers.cpp
//  libraries/shared/src
//
//  Created by Clement Brisset on 4/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CrashHelpers.h"

#ifdef NDEBUG
// undefine NDEBUG so doAssert() works for all builds
#undef NDEBUG
#include <assert.h>
#define NDEBUG
#else
#include <assert.h>
#endif
#include <stdexcept>

namespace crash {

class B;
class A {
public:
    A(B* b) : _b(b) { }
    ~A();
    virtual void virtualFunction() = 0;

private:
    B* _b;
};

class B : public A {
public:
    B() : A(this) { }
    virtual void virtualFunction() override { }
};

A::~A() {
    _b->virtualFunction();
}

// only use doAssert() for debug purposes
void doAssert(bool value) {
    assert(value);
}

void pureVirtualCall() {
    qCDebug(shared) << "About to make a pure virtual call";
    B b;
}

void doubleFree() {
    qCDebug(shared) << "About to double delete memory";
    int* blah = new int(200);
    delete blah;
    delete blah;
}

void nullDeref() {
    qCDebug(shared) << "About to dereference a null pointer";
    int* p = nullptr;
    *p = 1;
}

void doAbort() {
    qCDebug(shared) << "About to abort";
    abort();
}

void outOfBoundsVectorCrash() {
    qCDebug(shared) << "std::vector out of bounds crash!";
    std::vector<int> v;
    v[0] = 42;
}

void newFault() {
    qCDebug(shared) << "About to crash inside new fault";

    // Force crash with multiple large allocations
    while (true) {
        const size_t GIGABYTE = 1024 * 1024 * 1024;
        new char[GIGABYTE];
    }
}

void throwException() {
    qCDebug(shared) << "About to throw an exception";
    throw std::runtime_error("unexpected exception");
}

}
