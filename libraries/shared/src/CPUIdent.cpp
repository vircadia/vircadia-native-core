//
//  CPUIdent.cpp
//
//  Created by Ryan Huffman on 3/25/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CPUIdent.h"

#ifdef Q_OS_WIN

const CPUIdent::CPUIdent_Internal CPUIdent::CPU_Rep;

std::vector<CPUIdent::Feature> CPUIdent::getAllFeatures() {
    std::vector<CPUIdent::Feature> features;

    features.push_back({ "3DNOW", CPUIdent::_3DNOW() });
    features.push_back({ "3DNOWEXT", CPUIdent::_3DNOWEXT() });
    features.push_back({ "ABM", CPUIdent::ABM() });
    features.push_back({ "ADX", CPUIdent::ADX() });
    features.push_back({ "AES", CPUIdent::AES() });
    features.push_back({ "AVX", CPUIdent::AVX() });
    features.push_back({ "AVX2", CPUIdent::AVX2() });
    features.push_back({ "AVX512CD", CPUIdent::AVX512CD() });
    features.push_back({ "AVX512ER", CPUIdent::AVX512ER() });
    features.push_back({ "AVX512F", CPUIdent::AVX512F() });
    features.push_back({ "AVX512PF", CPUIdent::AVX512PF() });
    features.push_back({ "BMI1", CPUIdent::BMI1() });
    features.push_back({ "BMI2", CPUIdent::BMI2() });
    features.push_back({ "CLFSH", CPUIdent::CLFSH() });
    features.push_back({ "CMPXCHG16B", CPUIdent::CMPXCHG16B() });
    features.push_back({ "CX8", CPUIdent::CX8() });
    features.push_back({ "ERMS", CPUIdent::ERMS() });
    features.push_back({ "F16C", CPUIdent::F16C() });
    features.push_back({ "FMA", CPUIdent::FMA() });
    features.push_back({ "FSGSBASE", CPUIdent::FSGSBASE() });
    features.push_back({ "FXSR", CPUIdent::FXSR() });
    features.push_back({ "HLE", CPUIdent::HLE() });
    features.push_back({ "INVPCID", CPUIdent::INVPCID() });
    features.push_back({ "LAHF", CPUIdent::LAHF() });
    features.push_back({ "LZCNT", CPUIdent::LZCNT() });
    features.push_back({ "MMX", CPUIdent::MMX() });
    features.push_back({ "MMXEXT", CPUIdent::MMXEXT() });
    features.push_back({ "MONITOR", CPUIdent::MONITOR() });
    features.push_back({ "MOVBE", CPUIdent::MOVBE() });
    features.push_back({ "MSR", CPUIdent::MSR() });
    features.push_back({ "OSXSAVE", CPUIdent::OSXSAVE() });
    features.push_back({ "PCLMULQDQ", CPUIdent::PCLMULQDQ() });
    features.push_back({ "POPCNT", CPUIdent::POPCNT() });
    features.push_back({ "PREFETCHWT1", CPUIdent::PREFETCHWT1() });
    features.push_back({ "RDRAND", CPUIdent::RDRAND() });
    features.push_back({ "RDSEED", CPUIdent::RDSEED() });
    features.push_back({ "RDTSCP", CPUIdent::RDTSCP() });
    features.push_back({ "RTM", CPUIdent::RTM() });
    features.push_back({ "SEP", CPUIdent::SEP() });
    features.push_back({ "SHA", CPUIdent::SHA() });
    features.push_back({ "SSE", CPUIdent::SSE() });
    features.push_back({ "SSE2", CPUIdent::SSE2() });
    features.push_back({ "SSE3", CPUIdent::SSE3() });
    features.push_back({ "SSE4.1", CPUIdent::SSE41() });
    features.push_back({ "SSE4.2", CPUIdent::SSE42() });
    features.push_back({ "SSE4a", CPUIdent::SSE4a() });
    features.push_back({ "SSSE3", CPUIdent::SSSE3() });
    features.push_back({ "SYSCALL", CPUIdent::SYSCALL() });
    features.push_back({ "TBM", CPUIdent::TBM() });
    features.push_back({ "XOP", CPUIdent::XOP() });
    features.push_back({ "XSAVE", CPUIdent::XSAVE() });

    return features;
};

#endif
