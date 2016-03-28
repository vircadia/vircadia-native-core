//
//  CPUID.cpp
//
//  Created by Ryan Huffman on 3/25/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CPUID.h"

#ifdef Q_OS_WIN

const CPUID::CPUID_Internal CPUID::CPU_Rep;

std::vector<CPUID::Feature> CPUID::getAllFeatures() {
    std::vector<CPUID::Feature> features;

    features.push_back({ "3DNOW", CPUID::_3DNOW() });
    features.push_back({ "3DNOWEXT", CPUID::_3DNOWEXT() });
    features.push_back({ "ABM", CPUID::ABM() });
    features.push_back({ "ADX", CPUID::ADX() });
    features.push_back({ "AES", CPUID::AES() });
    features.push_back({ "AVX", CPUID::AVX() });
    features.push_back({ "AVX2", CPUID::AVX2() });
    features.push_back({ "AVX512CD", CPUID::AVX512CD() });
    features.push_back({ "AVX512ER", CPUID::AVX512ER() });
    features.push_back({ "AVX512F", CPUID::AVX512F() });
    features.push_back({ "AVX512PF", CPUID::AVX512PF() });
    features.push_back({ "BMI1", CPUID::BMI1() });
    features.push_back({ "BMI2", CPUID::BMI2() });
    features.push_back({ "CLFSH", CPUID::CLFSH() });
    features.push_back({ "CMPXCHG16B", CPUID::CMPXCHG16B() });
    features.push_back({ "CX8", CPUID::CX8() });
    features.push_back({ "ERMS", CPUID::ERMS() });
    features.push_back({ "F16C", CPUID::F16C() });
    features.push_back({ "FMA", CPUID::FMA() });
    features.push_back({ "FSGSBASE", CPUID::FSGSBASE() });
    features.push_back({ "FXSR", CPUID::FXSR() });
    features.push_back({ "HLE", CPUID::HLE() });
    features.push_back({ "INVPCID", CPUID::INVPCID() });
    features.push_back({ "LAHF", CPUID::LAHF() });
    features.push_back({ "LZCNT", CPUID::LZCNT() });
    features.push_back({ "MMX", CPUID::MMX() });
    features.push_back({ "MMXEXT", CPUID::MMXEXT() });
    features.push_back({ "MONITOR", CPUID::MONITOR() });
    features.push_back({ "MOVBE", CPUID::MOVBE() });
    features.push_back({ "MSR", CPUID::MSR() });
    features.push_back({ "OSXSAVE", CPUID::OSXSAVE() });
    features.push_back({ "PCLMULQDQ", CPUID::PCLMULQDQ() });
    features.push_back({ "POPCNT", CPUID::POPCNT() });
    features.push_back({ "PREFETCHWT1", CPUID::PREFETCHWT1() });
    features.push_back({ "RDRAND", CPUID::RDRAND() });
    features.push_back({ "RDSEED", CPUID::RDSEED() });
    features.push_back({ "RDTSCP", CPUID::RDTSCP() });
    features.push_back({ "RTM", CPUID::RTM() });
    features.push_back({ "SEP", CPUID::SEP() });
    features.push_back({ "SHA", CPUID::SHA() });
    features.push_back({ "SSE", CPUID::SSE() });
    features.push_back({ "SSE2", CPUID::SSE2() });
    features.push_back({ "SSE3", CPUID::SSE3() });
    features.push_back({ "SSE4.1", CPUID::SSE41() });
    features.push_back({ "SSE4.2", CPUID::SSE42() });
    features.push_back({ "SSE4a", CPUID::SSE4a() });
    features.push_back({ "SSSE3", CPUID::SSSE3() });
    features.push_back({ "SYSCALL", CPUID::SYSCALL() });
    features.push_back({ "TBM", CPUID::TBM() });
    features.push_back({ "XOP", CPUID::XOP() });
    features.push_back({ "XSAVE", CPUID::XSAVE() });

    return features;
};

#endif