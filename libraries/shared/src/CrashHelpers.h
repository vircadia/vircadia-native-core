//
//  CrashHelpers.h
//  libraries/shared/src
//
//  Created by Ryan Huffman on 11 April 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_CrashHelpers_h
#define hifi_CrashHelpers_h

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
    virtual void virtualFunction() { }
};

A::~A() {
    _b->virtualFunction();
}

void pureVirtualCall() {
    qDebug() << "About to make a pure virtual call";
    B b;
}
    
void doubleFree() {
    qDebug() << "About to double delete memory";
    int* blah = new int(200);
    delete blah;
    delete blah;
}

void nullDeref() {
    qDebug() << "About to dereference a null pointer";
    int* p = nullptr;
    *p = 1;
}

void doAbort() {
    qDebug() << "About to abort";
    abort();
}

void outOfBoundsVectorCrash() {
    qDebug() << "std::vector out of bounds crash!";
    std::vector<int> v;
    v[0] = 42;
}

void newFault() {
    qDebug() << "About to crash inside new fault";
    // Force crash with large allocation
    int* data = new int[std::numeric_limits<uint64_t>::max()];
    // Use variable to suppress warning
    data[0] = 0;
}

}

#endif // hifi_CrashHelpers_h
