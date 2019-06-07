
//
//  Created by Anthony Thibault on May 9th 2019
//  Copyright 2013-2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DependencyManagerTests.h"

#include <QtTest/QtTest>
#include <QtGui/QDesktopServices>
#include <test-utils/QTestExtensions.h>

#include <DependencyManager.h>
#include <thread>

// x macro
#define LIST_OF_CLASSES  \
    CLASS(A)             \
    CLASS(B)             \
    CLASS(C)             \
    CLASS(D)             \
    CLASS(E)             \
    CLASS(F)             \
    CLASS(G)             \
    CLASS(H)             \
    CLASS(I)             \
    CLASS(J)             \
    CLASS(K)             \
    CLASS(L)             \
    CLASS(M)             \
    CLASS(N)             \
    CLASS(O)             \
    CLASS(P)             \
    CLASS(Q)             \
    CLASS(R)             \
    CLASS(S)             \
    CLASS(T)             \
    CLASS(U)             \
    CLASS(V)             \
    CLASS(W)             \
    CLASS(X)             \
    CLASS(Y)             \
    CLASS(Z)             \
    CLASS(AA)            \
    CLASS(AB)            \
    CLASS(AC)            \
    CLASS(AD)            \
    CLASS(AE)            \
    CLASS(AF)            \
    CLASS(AG)            \
    CLASS(AH)            \
    CLASS(AI)            \
    CLASS(AJ)            \
    CLASS(AK)            \
    CLASS(AL)            \
    CLASS(AM)            \
    CLASS(AN)            \
    CLASS(AO)            \
    CLASS(AP)            \
    CLASS(AQ)            \
    CLASS(AR)            \
    CLASS(AS)            \
    CLASS(AT)            \
    CLASS(AU)            \
    CLASS(AV)            \
    CLASS(AW)            \
    CLASS(AX)            \
    CLASS(AY)            \
    CLASS(AZ)            \
    CLASS(BA)            \
    CLASS(BB)            \
    CLASS(BC)            \
    CLASS(BD)            \
    CLASS(BE)            \
    CLASS(BF)            \
    CLASS(BG)            \
    CLASS(BH)            \
    CLASS(BI)            \
    CLASS(BJ)            \
    CLASS(BK)            \
    CLASS(BL)            \
    CLASS(BM)            \
    CLASS(BN)            \
    CLASS(BO)            \
    CLASS(BP)            \
    CLASS(BQ)            \
    CLASS(BR)            \
    CLASS(BS)            \
    CLASS(BT)            \
    CLASS(BU)            \
    CLASS(BV)            \
    CLASS(BW)            \
    CLASS(BX)            \
    CLASS(BY)            \
    CLASS(BZ)            \
    CLASS(CA)            \
    CLASS(CB)            \
    CLASS(CC)            \
    CLASS(CD)            \
    CLASS(CE)            \
    CLASS(CF)            \
    CLASS(CG)            \
    CLASS(CH)            \
    CLASS(CI)            \
    CLASS(CJ)            \
    CLASS(CK)            \
    CLASS(CL)            \
    CLASS(CM)            \
    CLASS(CN)            \
    CLASS(CO)            \
    CLASS(CP)            \
    CLASS(CQ)            \
    CLASS(CR)            \
    CLASS(CS)            \
    CLASS(CT)            \
    CLASS(CU)            \
    CLASS(CV)            \
    CLASS(CW)            \
    CLASS(CX)            \
    CLASS(CY)            \
    CLASS(CZ)            \
    CLASS(DA)            \
    CLASS(DB)            \
    CLASS(DC)            \
    CLASS(DD)            \
    CLASS(DE)            \
    CLASS(DF)            \
    CLASS(DG)            \
    CLASS(DH)            \
    CLASS(DI)            \
    CLASS(DJ)            \
    CLASS(DK)            \
    CLASS(DL)            \
    CLASS(DM)            \
    CLASS(DN)            \
    CLASS(DO)            \
    CLASS(DP)            \
    CLASS(DQ)            \
    CLASS(DR)            \
    CLASS(DS)            \
    CLASS(DT)            \
    CLASS(DU)            \
    CLASS(DV)            \
    CLASS(DW)            \
    CLASS(DX)            \
    CLASS(DY)            \
    CLASS(DZ)


QTEST_MAIN(DependencyManagerTests)

#define CLASS(NAME) class NAME : public Dependency {};
LIST_OF_CLASSES
#undef CLASS

void DependencyManagerTests::testDependencyManager() {
    QCOMPARE(DependencyManager::isSet<A>(), false);
    DependencyManager::set<A>();
    QCOMPARE(DependencyManager::isSet<A>(), true);
    DependencyManager::destroy<A>();
    QCOMPARE(DependencyManager::isSet<A>(), false);
}

static void addDeps() {

#define CLASS(NAME) DependencyManager::set<NAME>();
LIST_OF_CLASSES
#undef CLASS
}

static void removeDeps() {
#define CLASS(NAME) DependencyManager::destroy<NAME>();
LIST_OF_CLASSES
#undef CLASS
}


static void spamIsSet() {
    for (int i = 0; i < 1000; i++) {
#define CLASS(NAME) DependencyManager::isSet<NAME>();
LIST_OF_CLASSES
#undef CLASS
    }
}

static void spamGet() {
    for (int i = 0; i < 10; i++) {
#define CLASS(NAME) DependencyManager::get<NAME>();
LIST_OF_CLASSES
#undef CLASS
    }
}


void assertDeps(bool value) {
#define CLASS(NAME) QCOMPARE(DependencyManager::isSet<NAME>(), value);
LIST_OF_CLASSES
#undef CLASS
}

void DependencyManagerTests::testDependencyManagerMultiThreaded() {

    std::thread isSetThread1(spamIsSet);  // spawn new thread that checks of dpendencies are present by calling isSet()
    std::thread getThread1(spamGet);  // spawn new thread that checks of dpendencies are present by calling get()
    addDeps();
    isSetThread1.join();
    getThread1.join();
    assertDeps(true);

    std::thread isSetThread2(spamIsSet);  // spawn new thread that checks of dpendencies are present by calling isSet()
    std::thread getThread2(spamGet);    // spawn new thread that checks of dpendencies are present by calling get()
    removeDeps();
    isSetThread2.join();
    getThread2.join();
    assertDeps(false);
}
