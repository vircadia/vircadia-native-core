//
//  AudioFOA_avx2.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 10/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef __AVX2__

#include <stdint.h>
#include <assert.h>
#include <immintrin.h>

#define _mm256_permute4x64_ps(ymm, imm)     _mm256_castpd_ps(_mm256_permute4x64_pd(_mm256_castps_pd(ymm), imm));

#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#define ALIGN32 __declspec(align(32))
#elif defined(__GNUC__)
#define FORCEINLINE inline __attribute__((always_inline))
#define ALIGN32 __attribute__((aligned(32)))
#else
#define FORCEINLINE inline
#define ALIGN32
#endif

//
// Vectorized FFT based on radix 2/4/8 Stockham auto-sort
// AVX2 version
//
typedef struct { 
    float re;
    float im;
} complex_t;

static const float SQRT1_2 = 0.707106781f;  // 1/sqrt(2)

ALIGN32 static const complex_t w_fft_256_0[] = {
    { 1.000000000e+00f, 9.238795042e-01f }, 
    { 7.071067691e-01f, 3.826834261e-01f }, 
    { 0.000000000e+00f,-3.826834261e-01f }, 
    {-7.071067691e-01f,-9.238795042e-01f }, 
    { 0.000000000e+00f, 3.826834261e-01f }, 
    { 7.071067691e-01f, 9.238795042e-01f }, 
    { 1.000000000e+00f, 9.238795042e-01f }, 
    { 7.071067691e-01f, 3.826834261e-01f }, 
};

ALIGN32 static const complex_t w_fft_256_1[] = {
    { 1.000000000e+00f, 9.951847196e-01f }, { 9.807852507e-01f, 9.569403529e-01f }, { 9.238795042e-01f, 8.819212914e-01f }, 
    { 8.314695954e-01f, 7.730104327e-01f }, { 0.000000000e+00f, 9.801714122e-02f }, { 1.950903237e-01f, 2.902846634e-01f }, 
    { 3.826834261e-01f, 4.713967443e-01f }, { 5.555702448e-01f, 6.343932748e-01f }, { 1.000000000e+00f, 9.807852507e-01f }, 
    { 9.238795042e-01f, 8.314695954e-01f }, { 7.071067691e-01f, 5.555702448e-01f }, { 3.826834261e-01f, 1.950903237e-01f }, 
    { 0.000000000e+00f, 1.950903237e-01f }, { 3.826834261e-01f, 5.555702448e-01f }, { 7.071067691e-01f, 8.314695954e-01f }, 
    { 9.238795042e-01f, 9.807852507e-01f }, { 1.000000000e+00f, 9.569403529e-01f }, { 8.314695954e-01f, 6.343932748e-01f }, 
    { 3.826834261e-01f, 9.801714122e-02f }, {-1.950903237e-01f,-4.713967443e-01f }, { 0.000000000e+00f, 2.902846634e-01f }, 
    { 5.555702448e-01f, 7.730104327e-01f }, { 9.238795042e-01f, 9.951847196e-01f }, { 9.807852507e-01f, 8.819212914e-01f }, 
    { 7.071067691e-01f, 6.343932748e-01f }, { 5.555702448e-01f, 4.713967443e-01f }, { 3.826834261e-01f, 2.902846634e-01f }, 
    { 1.950903237e-01f, 9.801714122e-02f }, { 7.071067691e-01f, 7.730104327e-01f }, { 8.314695954e-01f, 8.819212914e-01f }, 
    { 9.238795042e-01f, 9.569403529e-01f }, { 9.807852507e-01f, 9.951847196e-01f }, { 0.000000000e+00f,-1.950903237e-01f }, 
    {-3.826834261e-01f,-5.555702448e-01f }, {-7.071067691e-01f,-8.314695954e-01f }, {-9.238795042e-01f,-9.807852507e-01f }, 
    { 1.000000000e+00f, 9.807852507e-01f }, { 9.238795042e-01f, 8.314695954e-01f }, { 7.071067691e-01f, 5.555702448e-01f }, 
    { 3.826834261e-01f, 1.950903237e-01f }, {-7.071067691e-01f,-8.819212914e-01f }, {-9.807852507e-01f,-9.951847196e-01f }, 
    {-9.238795042e-01f,-7.730104327e-01f }, {-5.555702448e-01f,-2.902846634e-01f }, { 7.071067691e-01f, 4.713967443e-01f }, 
    { 1.950903237e-01f,-9.801714122e-02f }, {-3.826834261e-01f,-6.343932748e-01f }, {-8.314695954e-01f,-9.569403529e-01f }, 
};

ALIGN32 static const complex_t w_fft_256_2[] = {
    { 1.000000000e+00f, 9.996988177e-01f }, { 9.987954497e-01f, 9.972904325e-01f }, { 9.951847196e-01f, 9.924795628e-01f }, 
    { 9.891765118e-01f, 9.852776527e-01f }, { 0.000000000e+00f, 2.454122901e-02f }, { 4.906767607e-02f, 7.356456667e-02f }, 
    { 9.801714122e-02f, 1.224106774e-01f }, { 1.467304677e-01f, 1.709618866e-01f }, { 1.000000000e+00f, 9.987954497e-01f }, 
    { 9.951847196e-01f, 9.891765118e-01f }, { 9.807852507e-01f, 9.700312614e-01f }, { 9.569403529e-01f, 9.415440559e-01f }, 
    { 0.000000000e+00f, 4.906767607e-02f }, { 9.801714122e-02f, 1.467304677e-01f }, { 1.950903237e-01f, 2.429801822e-01f }, 
    { 2.902846634e-01f, 3.368898630e-01f }, { 1.000000000e+00f, 9.972904325e-01f }, { 9.891765118e-01f, 9.757021070e-01f }, 
    { 9.569403529e-01f, 9.329928160e-01f }, { 9.039893150e-01f, 8.700869679e-01f }, { 0.000000000e+00f, 7.356456667e-02f }, 
    { 1.467304677e-01f, 2.191012353e-01f }, { 2.902846634e-01f, 3.598950505e-01f }, { 4.275550842e-01f, 4.928981960e-01f }, 
    { 9.807852507e-01f, 9.757021070e-01f }, { 9.700312614e-01f, 9.637760520e-01f }, { 9.569403529e-01f, 9.495281577e-01f }, 
    { 9.415440559e-01f, 9.329928160e-01f }, { 1.950903237e-01f, 2.191012353e-01f }, { 2.429801822e-01f, 2.667127550e-01f }, 
    { 2.902846634e-01f, 3.136817515e-01f }, { 3.368898630e-01f, 3.598950505e-01f }, { 9.238795042e-01f, 9.039893150e-01f }, 
    { 8.819212914e-01f, 8.577286005e-01f }, { 8.314695954e-01f, 8.032075167e-01f }, { 7.730104327e-01f, 7.409511209e-01f }, 
    { 3.826834261e-01f, 4.275550842e-01f }, { 4.713967443e-01f, 5.141027570e-01f }, { 5.555702448e-01f, 5.956993103e-01f }, 
    { 6.343932748e-01f, 6.715589762e-01f }, { 8.314695954e-01f, 7.883464098e-01f }, { 7.409511209e-01f, 6.895405650e-01f }, 
    { 6.343932748e-01f, 5.758081675e-01f }, { 5.141027570e-01f, 4.496113360e-01f }, { 5.555702448e-01f, 6.152315736e-01f }, 
    { 6.715589762e-01f, 7.242470980e-01f }, { 7.730104327e-01f, 8.175848126e-01f }, { 8.577286005e-01f, 8.932242990e-01f }, 
    { 9.238795042e-01f, 9.142097831e-01f }, { 9.039893150e-01f, 8.932242990e-01f }, { 8.819212914e-01f, 8.700869679e-01f }, 
    { 8.577286005e-01f, 8.448535800e-01f }, { 3.826834261e-01f, 4.052413106e-01f }, { 4.275550842e-01f, 4.496113360e-01f }, 
    { 4.713967443e-01f, 4.928981960e-01f }, { 5.141027570e-01f, 5.349976420e-01f }, { 7.071067691e-01f, 6.715589762e-01f }, 
    { 6.343932748e-01f, 5.956993103e-01f }, { 5.555702448e-01f, 5.141027570e-01f }, { 4.713967443e-01f, 4.275550842e-01f }, 
    { 7.071067691e-01f, 7.409511209e-01f }, { 7.730104327e-01f, 8.032075167e-01f }, { 8.314695954e-01f, 8.577286005e-01f }, 
    { 8.819212914e-01f, 9.039893150e-01f }, { 3.826834261e-01f, 3.136817515e-01f }, { 2.429801822e-01f, 1.709618866e-01f }, 
    { 9.801714122e-02f, 2.454122901e-02f }, {-4.906767607e-02f,-1.224106774e-01f }, { 9.238795042e-01f, 9.495281577e-01f }, 
    { 9.700312614e-01f, 9.852776527e-01f }, { 9.951847196e-01f, 9.996988177e-01f }, { 9.987954497e-01f, 9.924795628e-01f }, 
    { 8.314695954e-01f, 8.175848126e-01f }, { 8.032075167e-01f, 7.883464098e-01f }, { 7.730104327e-01f, 7.572088242e-01f }, 
    { 7.409511209e-01f, 7.242470980e-01f }, { 5.555702448e-01f, 5.758081675e-01f }, { 5.956993103e-01f, 6.152315736e-01f }, 
    { 6.343932748e-01f, 6.531728506e-01f }, { 6.715589762e-01f, 6.895405650e-01f }, { 3.826834261e-01f, 3.368898630e-01f }, 
    { 2.902846634e-01f, 2.429801822e-01f }, { 1.950903237e-01f, 1.467304677e-01f }, { 9.801714122e-02f, 4.906767607e-02f }, 
    { 9.238795042e-01f, 9.415440559e-01f }, { 9.569403529e-01f, 9.700312614e-01f }, { 9.807852507e-01f, 9.891765118e-01f }, 
    { 9.951847196e-01f, 9.987954497e-01f }, {-1.950903237e-01f,-2.667127550e-01f }, {-3.368898630e-01f,-4.052413106e-01f }, 
    {-4.713967443e-01f,-5.349976420e-01f }, {-5.956993103e-01f,-6.531728506e-01f }, { 9.807852507e-01f, 9.637760520e-01f }, 
    { 9.415440559e-01f, 9.142097831e-01f }, { 8.819212914e-01f, 8.448535800e-01f }, { 8.032075167e-01f, 7.572088242e-01f }, 
    { 7.071067691e-01f, 6.895405650e-01f }, { 6.715589762e-01f, 6.531728506e-01f }, { 6.343932748e-01f, 6.152315736e-01f }, 
    { 5.956993103e-01f, 5.758081675e-01f }, { 7.071067691e-01f, 7.242470980e-01f }, { 7.409511209e-01f, 7.572088242e-01f }, 
    { 7.730104327e-01f, 7.883464098e-01f }, { 8.032075167e-01f, 8.175848126e-01f }, { 0.000000000e+00f,-4.906767607e-02f }, 
    {-9.801714122e-02f,-1.467304677e-01f }, {-1.950903237e-01f,-2.429801822e-01f }, {-2.902846634e-01f,-3.368898630e-01f }, 
    { 1.000000000e+00f, 9.987954497e-01f }, { 9.951847196e-01f, 9.891765118e-01f }, { 9.807852507e-01f, 9.700312614e-01f }, 
    { 9.569403529e-01f, 9.415440559e-01f }, {-7.071067691e-01f,-7.572088242e-01f }, {-8.032075167e-01f,-8.448535800e-01f }, 
    {-8.819212914e-01f,-9.142097831e-01f }, {-9.415440559e-01f,-9.637760520e-01f }, { 7.071067691e-01f, 6.531728506e-01f }, 
    { 5.956993103e-01f, 5.349976420e-01f }, { 4.713967443e-01f, 4.052413106e-01f }, { 3.368898630e-01f, 2.667127550e-01f }, 
    { 5.555702448e-01f, 5.349976420e-01f }, { 5.141027570e-01f, 4.928981960e-01f }, { 4.713967443e-01f, 4.496113360e-01f }, 
    { 4.275550842e-01f, 4.052413106e-01f }, { 8.314695954e-01f, 8.448535800e-01f }, { 8.577286005e-01f, 8.700869679e-01f }, 
    { 8.819212914e-01f, 8.932242990e-01f }, { 9.039893150e-01f, 9.142097831e-01f }, {-3.826834261e-01f,-4.275550842e-01f }, 
    {-4.713967443e-01f,-5.141027570e-01f }, {-5.555702448e-01f,-5.956993103e-01f }, {-6.343932748e-01f,-6.715589762e-01f }, 
    { 9.238795042e-01f, 9.039893150e-01f }, { 8.819212914e-01f, 8.577286005e-01f }, { 8.314695954e-01f, 8.032075167e-01f }, 
    { 7.730104327e-01f, 7.409511209e-01f }, {-9.807852507e-01f,-9.924795628e-01f }, {-9.987954497e-01f,-9.996988177e-01f }, 
    {-9.951847196e-01f,-9.852776527e-01f }, {-9.700312614e-01f,-9.495281577e-01f }, { 1.950903237e-01f, 1.224106774e-01f }, 
    { 4.906767607e-02f,-2.454122901e-02f }, {-9.801714122e-02f,-1.709618866e-01f }, {-2.429801822e-01f,-3.136817515e-01f }, 
    { 3.826834261e-01f, 3.598950505e-01f }, { 3.368898630e-01f, 3.136817515e-01f }, { 2.902846634e-01f, 2.667127550e-01f }, 
    { 2.429801822e-01f, 2.191012353e-01f }, { 9.238795042e-01f, 9.329928160e-01f }, { 9.415440559e-01f, 9.495281577e-01f }, 
    { 9.569403529e-01f, 9.637760520e-01f }, { 9.700312614e-01f, 9.757021070e-01f }, {-7.071067691e-01f,-7.409511209e-01f }, 
    {-7.730104327e-01f,-8.032075167e-01f }, {-8.314695954e-01f,-8.577286005e-01f }, {-8.819212914e-01f,-9.039893150e-01f }, 
    { 7.071067691e-01f, 6.715589762e-01f }, { 6.343932748e-01f, 5.956993103e-01f }, { 5.555702448e-01f, 5.141027570e-01f }, 
    { 4.713967443e-01f, 4.275550842e-01f }, {-9.238795042e-01f,-8.932242990e-01f }, {-8.577286005e-01f,-8.175848126e-01f }, 
    {-7.730104327e-01f,-7.242470980e-01f }, {-6.715589762e-01f,-6.152315736e-01f }, {-3.826834261e-01f,-4.496113360e-01f }, 
    {-5.141027570e-01f,-5.758081675e-01f }, {-6.343932748e-01f,-6.895405650e-01f }, {-7.409511209e-01f,-7.883464098e-01f }, 
    { 1.950903237e-01f, 1.709618866e-01f }, { 1.467304677e-01f, 1.224106774e-01f }, { 9.801714122e-02f, 7.356456667e-02f }, 
    { 4.906767607e-02f, 2.454122901e-02f }, { 9.807852507e-01f, 9.852776527e-01f }, { 9.891765118e-01f, 9.924795628e-01f }, 
    { 9.951847196e-01f, 9.972904325e-01f }, { 9.987954497e-01f, 9.996988177e-01f }, {-9.238795042e-01f,-9.415440559e-01f }, 
    {-9.569403529e-01f,-9.700312614e-01f }, {-9.807852507e-01f,-9.891765118e-01f }, {-9.951847196e-01f,-9.987954497e-01f }, 
    { 3.826834261e-01f, 3.368898630e-01f }, { 2.902846634e-01f, 2.429801822e-01f }, { 1.950903237e-01f, 1.467304677e-01f }, 
    { 9.801714122e-02f, 4.906767607e-02f }, {-5.555702448e-01f,-4.928981960e-01f }, {-4.275550842e-01f,-3.598950505e-01f }, 
    {-2.902846634e-01f,-2.191012353e-01f }, {-1.467304677e-01f,-7.356456667e-02f }, {-8.314695954e-01f,-8.700869679e-01f }, 
    {-9.039893150e-01f,-9.329928160e-01f }, {-9.569403529e-01f,-9.757021070e-01f }, {-9.891765118e-01f,-9.972904325e-01f }, 
};

ALIGN32 static complex_t w_rfft_512[] = {
    {-4.999623597e-01f,-4.998494089e-01f }, {-4.990590513e-01f,-4.986452162e-01f }, 
    {-4.996611774e-01f,-4.993977249e-01f }, {-4.981563091e-01f,-4.975923598e-01f }, 
    { 4.938642383e-01f, 4.877293706e-01f }, { 4.693396389e-01f, 4.632177055e-01f }, 
    { 4.815963805e-01f, 4.754661620e-01f }, { 4.571013451e-01f, 4.509914219e-01f }, 
    {-4.969534874e-01f,-4.962397814e-01f }, {-4.936507046e-01f,-4.926388264e-01f }, 
    {-4.954513311e-01f,-4.945882559e-01f }, {-4.915527403e-01f,-4.903926253e-01f }, 
    { 4.448888898e-01f, 4.387946725e-01f }, { 4.205709100e-01f, 4.145190716e-01f }, 
    { 4.327096343e-01f, 4.266347587e-01f }, { 4.084800482e-01f, 4.024548531e-01f }, 
    {-4.891586900e-01f,-4.878510535e-01f }, {-4.834882319e-01f,-4.818880260e-01f }, 
    {-4.864699841e-01f,-4.850156307e-01f }, {-4.802152514e-01f,-4.784701765e-01f }, 
    { 3.964443207e-01f, 3.904493749e-01f }, { 3.725671768e-01f, 3.666436076e-01f }, 
    { 3.844709396e-01f, 3.785099089e-01f }, { 3.607401550e-01f, 3.548576832e-01f }, 
    {-4.766530097e-01f,-4.747640789e-01f }, {-4.686695039e-01f,-4.664964080e-01f }, 
    {-4.728036523e-01f,-4.707720280e-01f }, {-4.642530382e-01f,-4.619397521e-01f }, 
    { 3.489970267e-01f, 3.431591392e-01f }, { 3.257906437e-01f, 3.200524747e-01f }, 
    { 3.373448551e-01f, 3.315550685e-01f }, { 3.143413961e-01f, 3.086583018e-01f }, 
    {-4.595569372e-01f,-4.571048915e-01f }, {-4.493372440e-01f,-4.466121495e-01f }, 
    {-4.545840025e-01f,-4.519946575e-01f }, {-4.438198209e-01f,-4.409606457e-01f }, 
    { 3.030039668e-01f, 2.973793447e-01f }, { 2.806918621e-01f, 2.751943469e-01f }, 
    { 2.917852402e-01f, 2.862224579e-01f }, { 2.697306275e-01f, 2.643016279e-01f }, 
    {-4.380350411e-01f,-4.350434840e-01f }, {-4.256775975e-01f,-4.224267900e-01f }, 
    {-4.319864213e-01f,-4.288643003e-01f }, {-4.191123545e-01f,-4.157347977e-01f }, 
    { 2.589080930e-01f, 2.535508871e-01f }, { 2.377051711e-01f, 2.325011790e-01f }, 
    { 2.482308149e-01f, 2.429486215e-01f }, { 2.273375094e-01f, 2.222148776e-01f }, 
    {-4.122946560e-01f,-4.087924063e-01f }, {-3.979184628e-01f,-3.941732049e-01f }, 
    {-4.052285850e-01f,-4.016037583e-01f }, {-3.903686106e-01f,-3.865052164e-01f }, 
    { 2.171340883e-01f, 2.120959163e-01f }, { 1.972444654e-01f, 1.923842132e-01f }, 
    { 2.071010768e-01f, 2.021503448e-01f }, { 1.875702441e-01f, 1.828033626e-01f }, 
    {-3.825836182e-01f,-3.786044121e-01f }, {-3.663271368e-01f,-3.621235490e-01f }, 
    {-3.745681942e-01f,-3.704755604e-01f }, {-3.578654230e-01f,-3.535533845e-01f }, 
    { 1.780842245e-01f, 1.734135747e-01f }, { 1.596994996e-01f, 1.552297175e-01f }, 
    { 1.687920988e-01f, 1.642205119e-01f }, { 1.508118808e-01f, 1.464466155e-01f }, 
    {-3.491881192e-01f,-3.447702825e-01f }, {-3.312079012e-01f,-3.265864253e-01f }, 
    {-3.403005004e-01f,-3.357794881e-01f }, {-3.219157755e-01f,-3.171966374e-01f }, 
    { 1.421345770e-01f, 1.378764510e-01f }, { 1.254318058e-01f, 1.213955879e-01f }, 
    { 1.336728632e-01f, 1.295244396e-01f }, { 1.174163818e-01f, 1.134947836e-01f }, 
    {-3.124297559e-01f,-3.076157868e-01f }, {-2.928989232e-01f,-2.879040837e-01f }, 
    {-3.027555346e-01f,-2.978496552e-01f }, {-2.828659117e-01f,-2.777851224e-01f }, 
    { 1.096313894e-01f, 1.058267951e-01f }, { 9.477141500e-02f, 9.120759368e-02f }, 
    { 1.020815372e-01f, 9.839624166e-02f }, { 8.770534396e-02f, 8.426520228e-02f }, 
    {-2.726624906e-01f,-2.674988210e-01f }, {-2.517691851e-01f,-2.464490980e-01f }, 
    {-2.622948289e-01f,-2.570513785e-01f }, {-2.410918921e-01f,-2.356983721e-01f }, 
    { 8.088764548e-02f, 7.757321000e-02f }, { 6.801357865e-02f, 6.495651603e-02f }, 
    { 7.432240248e-02f, 7.113569975e-02f }, { 6.196495891e-02f, 5.903935432e-02f }, 
    {-2.302693576e-01f,-2.248056680e-01f }, {-2.082147747e-01f,-2.026206553e-01f }, 
    {-2.193081230e-01f,-2.137775421e-01f }, {-1.969960183e-01f,-1.913417131e-01f }, 
    { 5.618017912e-02f, 5.338785052e-02f }, { 4.541599751e-02f, 4.289510846e-02f }, 
    { 5.066275597e-02f, 4.800534248e-02f }, { 4.044306278e-02f, 3.806024790e-02f }, 
    {-1.856586039e-01f,-1.799475253e-01f }, {-1.626551449e-01f,-1.568408757e-01f }, 
    {-1.742093414e-01f,-1.684449315e-01f }, {-1.510029733e-01f,-1.451423317e-01f }, 
    { 3.574696183e-02f, 3.350359201e-02f }, { 2.719634771e-02f, 2.523592114e-02f }, 
    { 3.133049607e-02f, 2.922797203e-02f }, { 2.334699035e-02f, 2.152982354e-02f }, 
    {-1.392598450e-01f,-1.333563775e-01f }, {-1.155290529e-01f,-1.095506176e-01f }, 
    {-1.274328232e-01f,-1.214900911e-01f }, {-1.035556868e-01f,-9.754516184e-02f }, 
    { 1.978474855e-02f, 1.811197400e-02f }, { 1.353001595e-02f, 1.214894652e-02f }, 
    { 1.651176810e-02f, 1.498436928e-02f }, { 1.084131002e-02f, 9.607374668e-03f }, 
    {-9.151994437e-02f,-8.548094332e-02f }, {-6.729035079e-02f,-6.120533869e-02f }, 
    {-7.942907512e-02f,-7.336523384e-02f }, {-5.511110276e-02f,-4.900857061e-02f }, 
    { 8.447259665e-03f, 7.361173630e-03f }, { 4.548668861e-03f, 3.760218620e-03f }, 
    { 6.349295378e-03f, 5.411744118e-03f }, { 3.046512604e-03f, 2.407640219e-03f }, 
    {-4.289865494e-02f,-3.678228334e-02f }, {-1.840361208e-02f,-1.227061450e-02f }, 
    {-3.066036850e-02f,-2.453383803e-02f }, {-6.135769188e-03f,-0.000000000e+00f }, 
    { 1.843690872e-03f, 1.354783773e-03f }, { 3.388226032e-04f, 1.505911350e-04f }, 
    { 9.409487247e-04f, 6.022751331e-04f }, { 3.764033318e-05f, 0.000000000e+00f }, 
};

// x[n] in SIMD8 order
// y[n] in SIMD8 order
// n >= 16
FORCEINLINE static void fft_radix2_pass(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/2;
    assert(t >= 8); // SIMD8
    assert(p >= 8); // SIMD8

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];

    for (size_t i = 0; i < t; i += 8) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 2*(i-k) + k; // output index

        __m256 ar0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 ai0 = _mm256_loadu_ps(&xp0[i+4].re);
        __m256 ar1 = _mm256_loadu_ps(&xp1[i+0].re);
        __m256 ai1 = _mm256_loadu_ps(&xp1[i+4].re);

        __m256 wr1 = _mm256_loadu_ps(&w[k+0].re);
        __m256 wi1 = _mm256_loadu_ps(&w[k+4].re);

        __m256 br1 = _mm256_mul_ps(ar1, wr1);
        __m256 bi1 = _mm256_mul_ps(ai1, wr1);

        br1 = _mm256_fmadd_ps(ai1, wi1, br1);
        bi1 = _mm256_fnmadd_ps(ar1, wi1, bi1);

        __m256 cr0 = _mm256_add_ps(ar0, br1);
        __m256 ci0 = _mm256_add_ps(ai0, bi1);
        __m256 cr1 = _mm256_sub_ps(ar0, br1);
        __m256 ci1 = _mm256_sub_ps(ai0, bi1);

        _mm256_storeu_ps(&yp0[j+0].re, cr0);
        _mm256_storeu_ps(&yp0[j+4].re, ci0);
        _mm256_storeu_ps(&yp1[j+0].re, cr1);
        _mm256_storeu_ps(&yp1[j+4].re, ci1);
    }
}  

// x[n] in SIMD8 order
// y[n] in SIMD8 order
// n >= 32
FORCEINLINE static void fft_radix4_pass(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/4;
    assert(t >= 8); // SIMD8
    assert(p >= 8); // SIMD8

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];
    complex_t* yp2 = &y[2*p];
    complex_t* yp3 = &y[3*p];

    for (size_t i = 0; i < t; i += 8) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 4*(i-k) + k; // output index

        __m256 ar1 = _mm256_loadu_ps(&xp1[i+0].re);
        __m256 ai1 = _mm256_loadu_ps(&xp1[i+4].re);
        __m256 ar2 = _mm256_loadu_ps(&xp2[i+0].re);
        __m256 ai2 = _mm256_loadu_ps(&xp2[i+4].re);
        __m256 ar3 = _mm256_loadu_ps(&xp3[i+0].re);
        __m256 ai3 = _mm256_loadu_ps(&xp3[i+4].re);

        __m256 wr1 = _mm256_loadu_ps(&w[3*k+0].re);
        __m256 wi1 = _mm256_loadu_ps(&w[3*k+4].re);
        __m256 wr2 = _mm256_loadu_ps(&w[3*k+8].re);
        __m256 wi2 = _mm256_loadu_ps(&w[3*k+12].re);
        __m256 wr3 = _mm256_loadu_ps(&w[3*k+16].re);
        __m256 wi3 = _mm256_loadu_ps(&w[3*k+20].re);

        __m256 br1 = _mm256_mul_ps(ar1, wr1);
        __m256 bi1 = _mm256_mul_ps(ai1, wr1);
        __m256 br2 = _mm256_mul_ps(ar2, wr2);
        __m256 bi2 = _mm256_mul_ps(ai2, wr2);
        __m256 br3 = _mm256_mul_ps(ar3, wr3);
        __m256 bi3 = _mm256_mul_ps(ai3, wr3);

        br1 = _mm256_fmadd_ps(ai1, wi1, br1);
        bi1 = _mm256_fnmadd_ps(ar1, wi1, bi1);
        br2 = _mm256_fmadd_ps(ai2, wi2, br2);
        bi2 = _mm256_fnmadd_ps(ar2, wi2, bi2);
        br3 = _mm256_fmadd_ps(ai3, wi3, br3);
        bi3 = _mm256_fnmadd_ps(ar3, wi3, bi3);

        __m256 ar0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 ai0 = _mm256_loadu_ps(&xp0[i+4].re);

        __m256 cr0 = _mm256_add_ps(ar0, br2);
        __m256 ci0 = _mm256_add_ps(ai0, bi2);
        __m256 cr2 = _mm256_sub_ps(ar0, br2);
        __m256 ci2 = _mm256_sub_ps(ai0, bi2);
        __m256 cr1 = _mm256_add_ps(br1, br3);
        __m256 ci1 = _mm256_add_ps(bi1, bi3);
        __m256 cr3 = _mm256_sub_ps(br1, br3);
        __m256 ci3 = _mm256_sub_ps(bi1, bi3);

        __m256 dr0 = _mm256_add_ps(cr0, cr1);
        __m256 di0 = _mm256_add_ps(ci0, ci1);
        __m256 dr1 = _mm256_add_ps(cr2, ci3);
        __m256 di1 = _mm256_sub_ps(ci2, cr3);
        __m256 dr2 = _mm256_sub_ps(cr0, cr1);
        __m256 di2 = _mm256_sub_ps(ci0, ci1);
        __m256 dr3 = _mm256_sub_ps(cr2, ci3);
        __m256 di3 = _mm256_add_ps(ci2, cr3);

        _mm256_storeu_ps(&yp0[j+0].re, dr0);
        _mm256_storeu_ps(&yp0[j+4].re, di0);
        _mm256_storeu_ps(&yp1[j+0].re, dr1);
        _mm256_storeu_ps(&yp1[j+4].re, di1);
        _mm256_storeu_ps(&yp2[j+0].re, dr2);
        _mm256_storeu_ps(&yp2[j+4].re, di2);
        _mm256_storeu_ps(&yp3[j+0].re, dr3);
        _mm256_storeu_ps(&yp3[j+4].re, di3);
    }
}

// x[n] in SIMD8 order
// y[n] in interleaved order
// n >= 32
FORCEINLINE static void fft_radix4_last(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/4;
    assert(t >= 8); // SIMD8
    assert(p >= 8); // SIMD8

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];
    complex_t* yp2 = &y[2*p];
    complex_t* yp3 = &y[3*p];

    for (size_t i = 0; i < t; i += 8) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 4*(i-k) + k; // output index

        __m256 ar1 = _mm256_loadu_ps(&xp1[i+0].re);
        __m256 ai1 = _mm256_loadu_ps(&xp1[i+4].re);
        __m256 ar2 = _mm256_loadu_ps(&xp2[i+0].re);
        __m256 ai2 = _mm256_loadu_ps(&xp2[i+4].re);
        __m256 ar3 = _mm256_loadu_ps(&xp3[i+0].re);
        __m256 ai3 = _mm256_loadu_ps(&xp3[i+4].re);

        __m256 wr1 = _mm256_loadu_ps(&w[3*k+0].re);
        __m256 wi1 = _mm256_loadu_ps(&w[3*k+4].re);
        __m256 wr2 = _mm256_loadu_ps(&w[3*k+8].re);
        __m256 wi2 = _mm256_loadu_ps(&w[3*k+12].re);
        __m256 wr3 = _mm256_loadu_ps(&w[3*k+16].re);
        __m256 wi3 = _mm256_loadu_ps(&w[3*k+20].re);

        __m256 br1 = _mm256_mul_ps(ar1, wr1);
        __m256 bi1 = _mm256_mul_ps(ai1, wr1);
        __m256 br2 = _mm256_mul_ps(ar2, wr2);
        __m256 bi2 = _mm256_mul_ps(ai2, wr2);
        __m256 br3 = _mm256_mul_ps(ar3, wr3);
        __m256 bi3 = _mm256_mul_ps(ai3, wr3);

        br1 = _mm256_fmadd_ps(ai1, wi1, br1);
        bi1 = _mm256_fnmadd_ps(ar1, wi1, bi1);
        br2 = _mm256_fmadd_ps(ai2, wi2, br2);
        bi2 = _mm256_fnmadd_ps(ar2, wi2, bi2);
        br3 = _mm256_fmadd_ps(ai3, wi3, br3);
        bi3 = _mm256_fnmadd_ps(ar3, wi3, bi3);

        __m256 ar0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 ai0 = _mm256_loadu_ps(&xp0[i+4].re);

        __m256 cr0 = _mm256_add_ps(ar0, br2);
        __m256 ci0 = _mm256_add_ps(ai0, bi2);
        __m256 cr2 = _mm256_sub_ps(ar0, br2);
        __m256 ci2 = _mm256_sub_ps(ai0, bi2);
        __m256 cr1 = _mm256_add_ps(br1, br3);
        __m256 ci1 = _mm256_add_ps(bi1, bi3);
        __m256 cr3 = _mm256_sub_ps(br1, br3);
        __m256 ci3 = _mm256_sub_ps(bi1, bi3);

        __m256 dr0 = _mm256_add_ps(cr0, cr1);
        __m256 di0 = _mm256_add_ps(ci0, ci1);
        __m256 dr1 = _mm256_add_ps(cr2, ci3);
        __m256 di1 = _mm256_sub_ps(ci2, cr3);
        __m256 dr2 = _mm256_sub_ps(cr0, cr1);
        __m256 di2 = _mm256_sub_ps(ci0, ci1);
        __m256 dr3 = _mm256_sub_ps(cr2, ci3);
        __m256 di3 = _mm256_add_ps(ci2, cr3);

        // interleave re,im
        __m256 t0 = _mm256_unpacklo_ps(dr0, di0);
        __m256 t1 = _mm256_unpackhi_ps(dr0, di0);
        __m256 t2 = _mm256_unpacklo_ps(dr1, di1);
        __m256 t3 = _mm256_unpackhi_ps(dr1, di1);
        __m256 t4 = _mm256_unpacklo_ps(dr2, di2);
        __m256 t5 = _mm256_unpackhi_ps(dr2, di2);
        __m256 t6 = _mm256_unpacklo_ps(dr3, di3);
        __m256 t7 = _mm256_unpackhi_ps(dr3, di3);

        __m256 z0 = _mm256_permute2f128_ps(t0, t1, 0x20);
        __m256 z1 = _mm256_permute2f128_ps(t0, t1, 0x31);
        __m256 z2 = _mm256_permute2f128_ps(t2, t3, 0x20);
        __m256 z3 = _mm256_permute2f128_ps(t2, t3, 0x31);
        __m256 z4 = _mm256_permute2f128_ps(t4, t5, 0x20);
        __m256 z5 = _mm256_permute2f128_ps(t4, t5, 0x31);
        __m256 z6 = _mm256_permute2f128_ps(t6, t7, 0x20);
        __m256 z7 = _mm256_permute2f128_ps(t6, t7, 0x31);

        _mm256_storeu_ps(&yp0[j+0].re, z0);
        _mm256_storeu_ps(&yp0[j+4].re, z1);
        _mm256_storeu_ps(&yp1[j+0].re, z2);
        _mm256_storeu_ps(&yp1[j+4].re, z3);
        _mm256_storeu_ps(&yp2[j+0].re, z4);
        _mm256_storeu_ps(&yp2[j+4].re, z5);
        _mm256_storeu_ps(&yp3[j+0].re, z6);
        _mm256_storeu_ps(&yp3[j+4].re, z7);
    }
}

// x[n] in SIMD8 order
// y[n] in interleaved order
// n >= 32
FORCEINLINE static void ifft_radix4_last(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/4;
    assert(t >= 8); // SIMD8
    assert(p >= 8); // SIMD8

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];
    complex_t* yp2 = &y[2*p];
    complex_t* yp3 = &y[3*p];

    for (size_t i = 0; i < t; i += 8) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 4*(i-k) + k; // output index

        __m256 ar1 = _mm256_loadu_ps(&xp1[i+0].re);
        __m256 ai1 = _mm256_loadu_ps(&xp1[i+4].re);
        __m256 ar2 = _mm256_loadu_ps(&xp2[i+0].re);
        __m256 ai2 = _mm256_loadu_ps(&xp2[i+4].re);
        __m256 ar3 = _mm256_loadu_ps(&xp3[i+0].re);
        __m256 ai3 = _mm256_loadu_ps(&xp3[i+4].re);

        __m256 wr1 = _mm256_loadu_ps(&w[3*k+0].re);
        __m256 wi1 = _mm256_loadu_ps(&w[3*k+4].re);
        __m256 wr2 = _mm256_loadu_ps(&w[3*k+8].re);
        __m256 wi2 = _mm256_loadu_ps(&w[3*k+12].re);
        __m256 wr3 = _mm256_loadu_ps(&w[3*k+16].re);
        __m256 wi3 = _mm256_loadu_ps(&w[3*k+20].re);

        __m256 br1 = _mm256_mul_ps(ar1, wr1);
        __m256 bi1 = _mm256_mul_ps(ai1, wr1);
        __m256 br2 = _mm256_mul_ps(ar2, wr2);
        __m256 bi2 = _mm256_mul_ps(ai2, wr2);
        __m256 br3 = _mm256_mul_ps(ar3, wr3);
        __m256 bi3 = _mm256_mul_ps(ai3, wr3);

        br1 = _mm256_fmadd_ps(ai1, wi1, br1);
        bi1 = _mm256_fnmadd_ps(ar1, wi1, bi1);
        br2 = _mm256_fmadd_ps(ai2, wi2, br2);
        bi2 = _mm256_fnmadd_ps(ar2, wi2, bi2);
        br3 = _mm256_fmadd_ps(ai3, wi3, br3);
        bi3 = _mm256_fnmadd_ps(ar3, wi3, bi3);

        __m256 ar0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 ai0 = _mm256_loadu_ps(&xp0[i+4].re);

        __m256 cr0 = _mm256_add_ps(ar0, br2);
        __m256 ci0 = _mm256_add_ps(ai0, bi2);
        __m256 cr2 = _mm256_sub_ps(ar0, br2);
        __m256 ci2 = _mm256_sub_ps(ai0, bi2);
        __m256 cr1 = _mm256_add_ps(br1, br3);
        __m256 ci1 = _mm256_add_ps(bi1, bi3);
        __m256 cr3 = _mm256_sub_ps(br1, br3);
        __m256 ci3 = _mm256_sub_ps(bi1, bi3);

        __m256 dr0 = _mm256_add_ps(cr0, cr1);
        __m256 di0 = _mm256_add_ps(ci0, ci1);
        __m256 dr1 = _mm256_add_ps(cr2, ci3);
        __m256 di1 = _mm256_sub_ps(ci2, cr3);
        __m256 dr2 = _mm256_sub_ps(cr0, cr1);
        __m256 di2 = _mm256_sub_ps(ci0, ci1);
        __m256 dr3 = _mm256_sub_ps(cr2, ci3);
        __m256 di3 = _mm256_add_ps(ci2, cr3);

        __m256 scale = _mm256_set1_ps(1.0f/n);  // scaled by 1/n for ifft
        dr0 = _mm256_mul_ps(dr0, scale);
        di0 = _mm256_mul_ps(di0, scale);
        dr1 = _mm256_mul_ps(dr1, scale);
        di1 = _mm256_mul_ps(di1, scale);
        dr2 = _mm256_mul_ps(dr2, scale);
        di2 = _mm256_mul_ps(di2, scale);
        dr3 = _mm256_mul_ps(dr3, scale);
        di3 = _mm256_mul_ps(di3, scale);

        // interleave re,im
        __m256 t0 = _mm256_unpacklo_ps(di0, dr0);   // swapped re,im for ifft
        __m256 t1 = _mm256_unpackhi_ps(di0, dr0);
        __m256 t2 = _mm256_unpacklo_ps(di1, dr1);
        __m256 t3 = _mm256_unpackhi_ps(di1, dr1);
        __m256 t4 = _mm256_unpacklo_ps(di2, dr2);
        __m256 t5 = _mm256_unpackhi_ps(di2, dr2);
        __m256 t6 = _mm256_unpacklo_ps(di3, dr3);
        __m256 t7 = _mm256_unpackhi_ps(di3, dr3);

        __m256 z0 = _mm256_permute2f128_ps(t0, t1, 0x20);
        __m256 z1 = _mm256_permute2f128_ps(t0, t1, 0x31);
        __m256 z2 = _mm256_permute2f128_ps(t2, t3, 0x20);
        __m256 z3 = _mm256_permute2f128_ps(t2, t3, 0x31);
        __m256 z4 = _mm256_permute2f128_ps(t4, t5, 0x20);
        __m256 z5 = _mm256_permute2f128_ps(t4, t5, 0x31);
        __m256 z6 = _mm256_permute2f128_ps(t6, t7, 0x20);
        __m256 z7 = _mm256_permute2f128_ps(t6, t7, 0x31);

        _mm256_storeu_ps(&yp0[j+0].re, z0);
        _mm256_storeu_ps(&yp0[j+4].re, z1);
        _mm256_storeu_ps(&yp1[j+0].re, z2);
        _mm256_storeu_ps(&yp1[j+4].re, z3);
        _mm256_storeu_ps(&yp2[j+0].re, z4);
        _mm256_storeu_ps(&yp2[j+4].re, z5);
        _mm256_storeu_ps(&yp3[j+0].re, z6);
        _mm256_storeu_ps(&yp3[j+4].re, z7);
    }
}

// x[n] in interleaved order
// y[n] in SIMD8 order
// n >= 64
FORCEINLINE static void fft_radix8_first(complex_t* x, complex_t* y, int n, int p) {

    size_t t = n/8;
    assert(t >= 8); // SIMD8
    assert(p == 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];
    complex_t* xp4 = &x[4*t];
    complex_t* xp5 = &x[5*t];
    complex_t* xp6 = &x[6*t];
    complex_t* xp7 = &x[7*t];

    for (size_t i = 0; i < t; i += 8) {

        //
        // even
        //
        __m256 z0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 z1 = _mm256_loadu_ps(&xp0[i+4].re);
        __m256 z2 = _mm256_loadu_ps(&xp2[i+0].re);
        __m256 z3 = _mm256_loadu_ps(&xp2[i+4].re);
        __m256 z4 = _mm256_loadu_ps(&xp4[i+0].re);
        __m256 z5 = _mm256_loadu_ps(&xp4[i+4].re);
        __m256 z6 = _mm256_loadu_ps(&xp6[i+0].re);
        __m256 z7 = _mm256_loadu_ps(&xp6[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar0 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ai0 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1));
        __m256 ar2 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(2,0,2,0));
        __m256 ai2 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(3,1,3,1));
        __m256 ar4 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(2,0,2,0));
        __m256 ai4 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(3,1,3,1));
        __m256 ar6 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(2,0,2,0));
        __m256 ai6 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(3,1,3,1));

        __m256 cr0 = _mm256_add_ps(ar0, ar4);
        __m256 ci0 = _mm256_add_ps(ai0, ai4);
        __m256 cr2 = _mm256_add_ps(ar2, ar6);
        __m256 ci2 = _mm256_add_ps(ai2, ai6);
        __m256 cr4 = _mm256_sub_ps(ar0, ar4);
        __m256 ci4 = _mm256_sub_ps(ai0, ai4);
        __m256 cr6 = _mm256_sub_ps(ar2, ar6);
        __m256 ci6 = _mm256_sub_ps(ai2, ai6);

        __m256 dr0 = _mm256_add_ps(cr0, cr2);
        __m256 di0 = _mm256_add_ps(ci0, ci2);
        __m256 dr2 = _mm256_sub_ps(cr0, cr2);
        __m256 di2 = _mm256_sub_ps(ci0, ci2);
        __m256 dr4 = _mm256_add_ps(cr4, ci6);
        __m256 di4 = _mm256_sub_ps(ci4, cr6);
        __m256 dr6 = _mm256_sub_ps(cr4, ci6);
        __m256 di6 = _mm256_add_ps(ci4, cr6);

        //
        // odd
        //
        z0 = _mm256_loadu_ps(&xp1[i+0].re);
        z1 = _mm256_loadu_ps(&xp1[i+4].re);
        z2 = _mm256_loadu_ps(&xp3[i+0].re);
        z3 = _mm256_loadu_ps(&xp3[i+4].re);
        z4 = _mm256_loadu_ps(&xp5[i+0].re);
        z5 = _mm256_loadu_ps(&xp5[i+4].re);
        z6 = _mm256_loadu_ps(&xp7[i+0].re);
        z7 = _mm256_loadu_ps(&xp7[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar1 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ai1 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1));
        __m256 ar3 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(2,0,2,0));
        __m256 ai3 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(3,1,3,1));
        __m256 ar5 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(2,0,2,0));
        __m256 ai5 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(3,1,3,1));
        __m256 ar7 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(2,0,2,0));
        __m256 ai7 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(3,1,3,1));

        __m256 br1 = _mm256_add_ps(ar1, ar5);
        __m256 bi1 = _mm256_add_ps(ai1, ai5);
        __m256 br3 = _mm256_add_ps(ar3, ar7);
        __m256 bi3 = _mm256_add_ps(ai3, ai7);
        __m256 br5 = _mm256_sub_ps(ar1, ar5);
        __m256 bi5 = _mm256_sub_ps(ai1, ai5);
        __m256 br7 = _mm256_sub_ps(ar3, ar7);
        __m256 bi7 = _mm256_sub_ps(ai3, ai7);

        __m256 cr1 = br1;
        __m256 ci1 = bi1;
        __m256 cr3 = br3;
        __m256 ci3 = bi3;
        __m256 cr5 = _mm256_add_ps(bi5, br5);
        __m256 ci5 = _mm256_sub_ps(bi5, br5);
        __m256 cr7 = _mm256_sub_ps(bi7, br7);
        __m256 ci7 = _mm256_add_ps(bi7, br7);

        cr5 = _mm256_mul_ps(cr5, _mm256_set1_ps(SQRT1_2));
        ci5 = _mm256_mul_ps(ci5, _mm256_set1_ps(SQRT1_2));
        cr7 = _mm256_mul_ps(cr7, _mm256_set1_ps(SQRT1_2));
        ci7 = _mm256_mul_ps(ci7, _mm256_set1_ps(SQRT1_2));

        __m256 dr1 = _mm256_add_ps(cr1, cr3);
        __m256 di1 = _mm256_add_ps(ci1, ci3);
        __m256 dr3 = _mm256_sub_ps(cr1, cr3);
        __m256 di3 = _mm256_sub_ps(ci1, ci3);
        __m256 dr5 = _mm256_add_ps(cr5, cr7);
        __m256 di5 = _mm256_sub_ps(ci5, ci7);
        __m256 dr7 = _mm256_sub_ps(cr5, cr7);
        __m256 di7 = _mm256_add_ps(ci5, ci7);

        //
        // merge
        //
        __m256 er0 = _mm256_add_ps(dr0, dr1);
        __m256 ei0 = _mm256_add_ps(di0, di1);
        __m256 er1 = _mm256_add_ps(dr4, dr5);
        __m256 ei1 = _mm256_add_ps(di4, di5);
        __m256 er2 = _mm256_add_ps(dr2, di3);
        __m256 ei2 = _mm256_sub_ps(di2, dr3);
        __m256 er3 = _mm256_add_ps(dr6, di7);
        __m256 ei3 = _mm256_sub_ps(di6, dr7);
        __m256 er4 = _mm256_sub_ps(dr0, dr1);
        __m256 ei4 = _mm256_sub_ps(di0, di1);
        __m256 er5 = _mm256_sub_ps(dr4, dr5);
        __m256 ei5 = _mm256_sub_ps(di4, di5);
        __m256 er6 = _mm256_sub_ps(dr2, di3);
        __m256 ei6 = _mm256_add_ps(di2, dr3);
        __m256 er7 = _mm256_sub_ps(dr6, di7);
        __m256 ei7 = _mm256_add_ps(di6, dr7);

        //
        // 8x8 transpose
        //
        __m256 tr0 = _mm256_unpacklo_ps(er0, er1);
        __m256 tr1 = _mm256_unpackhi_ps(er0, er1);
        __m256 tr2 = _mm256_unpacklo_ps(er2, er3);
        __m256 tr3 = _mm256_unpackhi_ps(er2, er3);
        __m256 tr4 = _mm256_unpacklo_ps(er4, er5);
        __m256 tr5 = _mm256_unpackhi_ps(er4, er5);
        __m256 tr6 = _mm256_unpacklo_ps(er6, er7);
        __m256 tr7 = _mm256_unpackhi_ps(er6, er7);

        __m256 ti0 = _mm256_unpacklo_ps(ei0, ei1);
        __m256 ti1 = _mm256_unpackhi_ps(ei0, ei1);
        __m256 ti2 = _mm256_unpacklo_ps(ei2, ei3);
        __m256 ti3 = _mm256_unpackhi_ps(ei2, ei3);
        __m256 ti4 = _mm256_unpacklo_ps(ei4, ei5);
        __m256 ti5 = _mm256_unpackhi_ps(ei4, ei5);
        __m256 ti6 = _mm256_unpacklo_ps(ei6, ei7);
        __m256 ti7 = _mm256_unpackhi_ps(ei6, ei7);

        __m256 ur0 = _mm256_shuffle_ps(tr0, tr2, _MM_SHUFFLE(1,0,1,0));
        __m256 ur1 = _mm256_shuffle_ps(tr0, tr2, _MM_SHUFFLE(3,2,3,2));
        __m256 ur2 = _mm256_shuffle_ps(tr1, tr3, _MM_SHUFFLE(1,0,1,0));
        __m256 ur3 = _mm256_shuffle_ps(tr1, tr3, _MM_SHUFFLE(3,2,3,2));
        __m256 ur4 = _mm256_shuffle_ps(tr4, tr6, _MM_SHUFFLE(1,0,1,0));
        __m256 ur5 = _mm256_shuffle_ps(tr4, tr6, _MM_SHUFFLE(3,2,3,2));
        __m256 ur6 = _mm256_shuffle_ps(tr5, tr7, _MM_SHUFFLE(1,0,1,0));
        __m256 ur7 = _mm256_shuffle_ps(tr5, tr7, _MM_SHUFFLE(3,2,3,2));

        __m256 ui0 = _mm256_shuffle_ps(ti0, ti2, _MM_SHUFFLE(1,0,1,0));
        __m256 ui1 = _mm256_shuffle_ps(ti0, ti2, _MM_SHUFFLE(3,2,3,2));
        __m256 ui2 = _mm256_shuffle_ps(ti1, ti3, _MM_SHUFFLE(1,0,1,0));
        __m256 ui3 = _mm256_shuffle_ps(ti1, ti3, _MM_SHUFFLE(3,2,3,2));
        __m256 ui4 = _mm256_shuffle_ps(ti4, ti6, _MM_SHUFFLE(1,0,1,0));
        __m256 ui5 = _mm256_shuffle_ps(ti4, ti6, _MM_SHUFFLE(3,2,3,2));
        __m256 ui6 = _mm256_shuffle_ps(ti5, ti7, _MM_SHUFFLE(1,0,1,0));
        __m256 ui7 = _mm256_shuffle_ps(ti5, ti7, _MM_SHUFFLE(3,2,3,2));

        __m256 vr0 = _mm256_permute2f128_ps(ur0, ur4, 0x20);
        __m256 vr1 = _mm256_permute2f128_ps(ur1, ur5, 0x20);
        __m256 vr2 = _mm256_permute2f128_ps(ur0, ur4, 0x31);
        __m256 vr3 = _mm256_permute2f128_ps(ur1, ur5, 0x31);
        __m256 vr4 = _mm256_permute2f128_ps(ur2, ur6, 0x20);
        __m256 vr5 = _mm256_permute2f128_ps(ur3, ur7, 0x20);
        __m256 vr6 = _mm256_permute2f128_ps(ur2, ur6, 0x31);
        __m256 vr7 = _mm256_permute2f128_ps(ur3, ur7, 0x31);

        __m256 vi0 = _mm256_permute2f128_ps(ui0, ui4, 0x20);
        __m256 vi1 = _mm256_permute2f128_ps(ui1, ui5, 0x20);
        __m256 vi2 = _mm256_permute2f128_ps(ui0, ui4, 0x31);
        __m256 vi3 = _mm256_permute2f128_ps(ui1, ui5, 0x31);
        __m256 vi4 = _mm256_permute2f128_ps(ui2, ui6, 0x20);
        __m256 vi5 = _mm256_permute2f128_ps(ui3, ui7, 0x20);
        __m256 vi6 = _mm256_permute2f128_ps(ui2, ui6, 0x31);
        __m256 vi7 = _mm256_permute2f128_ps(ui3, ui7, 0x31);

        _mm256_storeu_ps(&y[8*i+ 0].re, vr0);
        _mm256_storeu_ps(&y[8*i+ 4].re, vi0);
        _mm256_storeu_ps(&y[8*i+ 8].re, vr1);
        _mm256_storeu_ps(&y[8*i+12].re, vi1);
        _mm256_storeu_ps(&y[8*i+16].re, vr2);
        _mm256_storeu_ps(&y[8*i+20].re, vi2);
        _mm256_storeu_ps(&y[8*i+24].re, vr3);
        _mm256_storeu_ps(&y[8*i+28].re, vi3);
        _mm256_storeu_ps(&y[8*i+32].re, vr4);
        _mm256_storeu_ps(&y[8*i+36].re, vi4);
        _mm256_storeu_ps(&y[8*i+40].re, vr5);
        _mm256_storeu_ps(&y[8*i+44].re, vi5);
        _mm256_storeu_ps(&y[8*i+48].re, vr6);
        _mm256_storeu_ps(&y[8*i+52].re, vi6);
        _mm256_storeu_ps(&y[8*i+56].re, vr7);
        _mm256_storeu_ps(&y[8*i+60].re, vi7);
    }
}

// x[n] in interleaved order
// y[n] in SIMD8 order
// n >= 64
FORCEINLINE static void ifft_radix8_first(complex_t* x, complex_t* y, int n, int p) {

    size_t t = n/8;
    assert(t >= 8); // SIMD8
    assert(p == 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];
    complex_t* xp4 = &x[4*t];
    complex_t* xp5 = &x[5*t];
    complex_t* xp6 = &x[6*t];
    complex_t* xp7 = &x[7*t];

    for (size_t i = 0; i < t; i += 8) {

        //
        // even
        //
        __m256 z0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 z1 = _mm256_loadu_ps(&xp0[i+4].re);
        __m256 z2 = _mm256_loadu_ps(&xp2[i+0].re);
        __m256 z3 = _mm256_loadu_ps(&xp2[i+4].re);
        __m256 z4 = _mm256_loadu_ps(&xp4[i+0].re);
        __m256 z5 = _mm256_loadu_ps(&xp4[i+4].re);
        __m256 z6 = _mm256_loadu_ps(&xp6[i+0].re);
        __m256 z7 = _mm256_loadu_ps(&xp6[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar0 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1)); // swapped re,im for ifft
        __m256 ai0 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ar2 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(3,1,3,1));
        __m256 ai2 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(2,0,2,0));
        __m256 ar4 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(3,1,3,1));
        __m256 ai4 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(2,0,2,0));
        __m256 ar6 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(3,1,3,1));
        __m256 ai6 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(2,0,2,0));

        __m256 cr0 = _mm256_add_ps(ar0, ar4);
        __m256 ci0 = _mm256_add_ps(ai0, ai4);
        __m256 cr2 = _mm256_add_ps(ar2, ar6);
        __m256 ci2 = _mm256_add_ps(ai2, ai6);
        __m256 cr4 = _mm256_sub_ps(ar0, ar4);
        __m256 ci4 = _mm256_sub_ps(ai0, ai4);
        __m256 cr6 = _mm256_sub_ps(ar2, ar6);
        __m256 ci6 = _mm256_sub_ps(ai2, ai6);

        __m256 dr0 = _mm256_add_ps(cr0, cr2);
        __m256 di0 = _mm256_add_ps(ci0, ci2);
        __m256 dr2 = _mm256_sub_ps(cr0, cr2);
        __m256 di2 = _mm256_sub_ps(ci0, ci2);
        __m256 dr4 = _mm256_add_ps(cr4, ci6);
        __m256 di4 = _mm256_sub_ps(ci4, cr6);
        __m256 dr6 = _mm256_sub_ps(cr4, ci6);
        __m256 di6 = _mm256_add_ps(ci4, cr6);

        //
        // odd
        //
        z0 = _mm256_loadu_ps(&xp1[i+0].re);
        z1 = _mm256_loadu_ps(&xp1[i+4].re);
        z2 = _mm256_loadu_ps(&xp3[i+0].re);
        z3 = _mm256_loadu_ps(&xp3[i+4].re);
        z4 = _mm256_loadu_ps(&xp5[i+0].re);
        z5 = _mm256_loadu_ps(&xp5[i+4].re);
        z6 = _mm256_loadu_ps(&xp7[i+0].re);
        z7 = _mm256_loadu_ps(&xp7[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar1 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1)); // swapped re,im for ifft
        __m256 ai1 = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ar3 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(3,1,3,1));
        __m256 ai3 = _mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(2,0,2,0));
        __m256 ar5 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(3,1,3,1));
        __m256 ai5 = _mm256_shuffle_ps(z4, z5, _MM_SHUFFLE(2,0,2,0));
        __m256 ar7 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(3,1,3,1));
        __m256 ai7 = _mm256_shuffle_ps(z6, z7, _MM_SHUFFLE(2,0,2,0));

        __m256 br1 = _mm256_add_ps(ar1, ar5);
        __m256 bi1 = _mm256_add_ps(ai1, ai5);
        __m256 br3 = _mm256_add_ps(ar3, ar7);
        __m256 bi3 = _mm256_add_ps(ai3, ai7);
        __m256 br5 = _mm256_sub_ps(ar1, ar5);
        __m256 bi5 = _mm256_sub_ps(ai1, ai5);
        __m256 br7 = _mm256_sub_ps(ar3, ar7);
        __m256 bi7 = _mm256_sub_ps(ai3, ai7);

        __m256 cr1 = br1;
        __m256 ci1 = bi1;
        __m256 cr3 = br3;
        __m256 ci3 = bi3;
        __m256 cr5 = _mm256_add_ps(bi5, br5);
        __m256 ci5 = _mm256_sub_ps(bi5, br5);
        __m256 cr7 = _mm256_sub_ps(bi7, br7);
        __m256 ci7 = _mm256_add_ps(bi7, br7);

        cr5 = _mm256_mul_ps(cr5, _mm256_set1_ps(SQRT1_2));
        ci5 = _mm256_mul_ps(ci5, _mm256_set1_ps(SQRT1_2));
        cr7 = _mm256_mul_ps(cr7, _mm256_set1_ps(SQRT1_2));
        ci7 = _mm256_mul_ps(ci7, _mm256_set1_ps(SQRT1_2));

        __m256 dr1 = _mm256_add_ps(cr1, cr3);
        __m256 di1 = _mm256_add_ps(ci1, ci3);
        __m256 dr3 = _mm256_sub_ps(cr1, cr3);
        __m256 di3 = _mm256_sub_ps(ci1, ci3);
        __m256 dr5 = _mm256_add_ps(cr5, cr7);
        __m256 di5 = _mm256_sub_ps(ci5, ci7);
        __m256 dr7 = _mm256_sub_ps(cr5, cr7);
        __m256 di7 = _mm256_add_ps(ci5, ci7);

        //
        // merge
        //
        __m256 er0 = _mm256_add_ps(dr0, dr1);
        __m256 ei0 = _mm256_add_ps(di0, di1);
        __m256 er1 = _mm256_add_ps(dr4, dr5);
        __m256 ei1 = _mm256_add_ps(di4, di5);
        __m256 er2 = _mm256_add_ps(dr2, di3);
        __m256 ei2 = _mm256_sub_ps(di2, dr3);
        __m256 er3 = _mm256_add_ps(dr6, di7);
        __m256 ei3 = _mm256_sub_ps(di6, dr7);
        __m256 er4 = _mm256_sub_ps(dr0, dr1);
        __m256 ei4 = _mm256_sub_ps(di0, di1);
        __m256 er5 = _mm256_sub_ps(dr4, dr5);
        __m256 ei5 = _mm256_sub_ps(di4, di5);
        __m256 er6 = _mm256_sub_ps(dr2, di3);
        __m256 ei6 = _mm256_add_ps(di2, dr3);
        __m256 er7 = _mm256_sub_ps(dr6, di7);
        __m256 ei7 = _mm256_add_ps(di6, dr7);

        //
        // 8x8 transpose
        //
        __m256 tr0 = _mm256_unpacklo_ps(er0, er1);
        __m256 tr1 = _mm256_unpackhi_ps(er0, er1);
        __m256 tr2 = _mm256_unpacklo_ps(er2, er3);
        __m256 tr3 = _mm256_unpackhi_ps(er2, er3);
        __m256 tr4 = _mm256_unpacklo_ps(er4, er5);
        __m256 tr5 = _mm256_unpackhi_ps(er4, er5);
        __m256 tr6 = _mm256_unpacklo_ps(er6, er7);
        __m256 tr7 = _mm256_unpackhi_ps(er6, er7);

        __m256 ti0 = _mm256_unpacklo_ps(ei0, ei1);
        __m256 ti1 = _mm256_unpackhi_ps(ei0, ei1);
        __m256 ti2 = _mm256_unpacklo_ps(ei2, ei3);
        __m256 ti3 = _mm256_unpackhi_ps(ei2, ei3);
        __m256 ti4 = _mm256_unpacklo_ps(ei4, ei5);
        __m256 ti5 = _mm256_unpackhi_ps(ei4, ei5);
        __m256 ti6 = _mm256_unpacklo_ps(ei6, ei7);
        __m256 ti7 = _mm256_unpackhi_ps(ei6, ei7);

        __m256 ur0 = _mm256_shuffle_ps(tr0, tr2, _MM_SHUFFLE(1,0,1,0));
        __m256 ur1 = _mm256_shuffle_ps(tr0, tr2, _MM_SHUFFLE(3,2,3,2));
        __m256 ur2 = _mm256_shuffle_ps(tr1, tr3, _MM_SHUFFLE(1,0,1,0));
        __m256 ur3 = _mm256_shuffle_ps(tr1, tr3, _MM_SHUFFLE(3,2,3,2));
        __m256 ur4 = _mm256_shuffle_ps(tr4, tr6, _MM_SHUFFLE(1,0,1,0));
        __m256 ur5 = _mm256_shuffle_ps(tr4, tr6, _MM_SHUFFLE(3,2,3,2));
        __m256 ur6 = _mm256_shuffle_ps(tr5, tr7, _MM_SHUFFLE(1,0,1,0));
        __m256 ur7 = _mm256_shuffle_ps(tr5, tr7, _MM_SHUFFLE(3,2,3,2));

        __m256 ui0 = _mm256_shuffle_ps(ti0, ti2, _MM_SHUFFLE(1,0,1,0));
        __m256 ui1 = _mm256_shuffle_ps(ti0, ti2, _MM_SHUFFLE(3,2,3,2));
        __m256 ui2 = _mm256_shuffle_ps(ti1, ti3, _MM_SHUFFLE(1,0,1,0));
        __m256 ui3 = _mm256_shuffle_ps(ti1, ti3, _MM_SHUFFLE(3,2,3,2));
        __m256 ui4 = _mm256_shuffle_ps(ti4, ti6, _MM_SHUFFLE(1,0,1,0));
        __m256 ui5 = _mm256_shuffle_ps(ti4, ti6, _MM_SHUFFLE(3,2,3,2));
        __m256 ui6 = _mm256_shuffle_ps(ti5, ti7, _MM_SHUFFLE(1,0,1,0));
        __m256 ui7 = _mm256_shuffle_ps(ti5, ti7, _MM_SHUFFLE(3,2,3,2));

        __m256 vr0 = _mm256_permute2f128_ps(ur0, ur4, 0x20);
        __m256 vr1 = _mm256_permute2f128_ps(ur1, ur5, 0x20);
        __m256 vr2 = _mm256_permute2f128_ps(ur0, ur4, 0x31);
        __m256 vr3 = _mm256_permute2f128_ps(ur1, ur5, 0x31);
        __m256 vr4 = _mm256_permute2f128_ps(ur2, ur6, 0x20);
        __m256 vr5 = _mm256_permute2f128_ps(ur3, ur7, 0x20);
        __m256 vr6 = _mm256_permute2f128_ps(ur2, ur6, 0x31);
        __m256 vr7 = _mm256_permute2f128_ps(ur3, ur7, 0x31);

        __m256 vi0 = _mm256_permute2f128_ps(ui0, ui4, 0x20);
        __m256 vi1 = _mm256_permute2f128_ps(ui1, ui5, 0x20);
        __m256 vi2 = _mm256_permute2f128_ps(ui0, ui4, 0x31);
        __m256 vi3 = _mm256_permute2f128_ps(ui1, ui5, 0x31);
        __m256 vi4 = _mm256_permute2f128_ps(ui2, ui6, 0x20);
        __m256 vi5 = _mm256_permute2f128_ps(ui3, ui7, 0x20);
        __m256 vi6 = _mm256_permute2f128_ps(ui2, ui6, 0x31);
        __m256 vi7 = _mm256_permute2f128_ps(ui3, ui7, 0x31);

        _mm256_storeu_ps(&y[8*i+ 0].re, vr0);
        _mm256_storeu_ps(&y[8*i+ 4].re, vi0);
        _mm256_storeu_ps(&y[8*i+ 8].re, vr1);
        _mm256_storeu_ps(&y[8*i+12].re, vi1);
        _mm256_storeu_ps(&y[8*i+16].re, vr2);
        _mm256_storeu_ps(&y[8*i+20].re, vi2);
        _mm256_storeu_ps(&y[8*i+24].re, vr3);
        _mm256_storeu_ps(&y[8*i+28].re, vi3);
        _mm256_storeu_ps(&y[8*i+32].re, vr4);
        _mm256_storeu_ps(&y[8*i+36].re, vi4);
        _mm256_storeu_ps(&y[8*i+40].re, vr5);
        _mm256_storeu_ps(&y[8*i+44].re, vi5);
        _mm256_storeu_ps(&y[8*i+48].re, vr6);
        _mm256_storeu_ps(&y[8*i+52].re, vi6);
        _mm256_storeu_ps(&y[8*i+56].re, vr7);
        _mm256_storeu_ps(&y[8*i+60].re, vi7);
    }
}

// x[n] in interleaved order
// n >= 32
static void rfft_post(complex_t* x, const complex_t* w, int n) {

    size_t t = n/4;
    assert(n/4 >= 8);   // SIMD8

    // NOTE: x[n/2].re is packed into x[0].im
    float tr = x[0].re;
    float ti = x[0].im;
    x[0].re = tr + ti;
    x[0].im = tr - ti;

    complex_t* xp0 = &x[1];
    complex_t* xp1 = &x[n/2 - 8];

    for (size_t i = 0; i < t; i += 8) {

        __m256 z0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 z1 = _mm256_loadu_ps(&xp0[i+4].re);
        __m256 z2 = _mm256_loadu_ps(&xp1[0-i].re);
        __m256 z3 = _mm256_loadu_ps(&xp1[4-i].re);

        __m256 wr = _mm256_loadu_ps(&w[i+0].re);
        __m256 wi = _mm256_loadu_ps(&w[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ai = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1));
        __m256 br = _mm256_permute4x64_ps(_mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(0,2,0,2)), _MM_SHUFFLE(0,1,2,3));   // reversed
        __m256 bi = _mm256_permute4x64_ps(_mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(1,3,1,3)), _MM_SHUFFLE(0,1,2,3));   // reversed

        __m256 cr = _mm256_sub_ps(ar, br);
        __m256 ci = _mm256_add_ps(ai, bi);

        __m256 dr = _mm256_mul_ps(cr, wr);
        __m256 di = _mm256_mul_ps(ci, wr);

        dr = _mm256_fmadd_ps(ci, wi, dr);
        di = _mm256_fnmadd_ps(cr, wi, di);

        ar = _mm256_add_ps(ar, di);
        ai = _mm256_sub_ps(dr, ai);

        br = _mm256_sub_ps(br, di);
        bi = _mm256_sub_ps(dr, bi);

        // interleave re,im (input permuted 0,2,1,3)
        z0 = _mm256_unpacklo_ps(br, bi);
        z1 = _mm256_unpackhi_ps(br, bi);
        z2 = _mm256_permute4x64_ps(_mm256_unpackhi_ps(ar, ai), _MM_SHUFFLE(0,1,2,3));   // reversed
        z3 = _mm256_permute4x64_ps(_mm256_unpacklo_ps(ar, ai), _MM_SHUFFLE(0,1,2,3));   // reversed

        _mm256_storeu_ps(&xp0[i+0].re, z0);
        _mm256_storeu_ps(&xp0[i+4].re, z1);
        _mm256_storeu_ps(&xp1[0-i].re, z2);
        _mm256_storeu_ps(&xp1[4-i].re, z3);
    }
}

// x[n] in interleaved order
// n >= 32
static void rifft_pre(complex_t* x, const complex_t* w, int n) {

    size_t t = n/4;
    assert(n/4 >= 8);   // SIMD8

    // NOTE: x[n/2].re is packed into x[0].im
    float tr = x[0].re;
    float ti = x[0].im;
    x[0].re = 0.5f * (tr + ti); // halved for ifft
    x[0].im = 0.5f * (tr - ti); // halved for ifft

    complex_t* xp0 = &x[1];
    complex_t* xp1 = &x[n/2 - 8];

    for (size_t i = 0; i < t; i += 8) {

        __m256 z0 = _mm256_loadu_ps(&xp0[i+0].re);
        __m256 z1 = _mm256_loadu_ps(&xp0[i+4].re);
        __m256 z2 = _mm256_loadu_ps(&xp1[0-i].re);
        __m256 z3 = _mm256_loadu_ps(&xp1[4-i].re);

        __m256 wr = _mm256_loadu_ps(&w[i+0].re);    // negated for ifft
        __m256 wi = _mm256_loadu_ps(&w[i+4].re);

        // deinterleave re,im (output permuted 0,2,1,3)
        __m256 ar = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(2,0,2,0));
        __m256 ai = _mm256_shuffle_ps(z0, z1, _MM_SHUFFLE(3,1,3,1));
        __m256 br = _mm256_permute4x64_ps(_mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(0,2,0,2)), _MM_SHUFFLE(0,1,2,3));   // reversed
        __m256 bi = _mm256_permute4x64_ps(_mm256_shuffle_ps(z2, z3, _MM_SHUFFLE(1,3,1,3)), _MM_SHUFFLE(0,1,2,3));   // reversed

        __m256 cr = _mm256_sub_ps(br, ar);
        __m256 ci = _mm256_add_ps(ai, bi);

        __m256 dr = _mm256_mul_ps(cr, wr);
        __m256 di = _mm256_mul_ps(cr, wi);

        dr = _mm256_fmadd_ps(ci, wi, dr);
        di = _mm256_fnmadd_ps(ci, wr, di);

        ar = _mm256_add_ps(ar, di);
        ai = _mm256_sub_ps(dr, ai);

        br = _mm256_sub_ps(br, di);
        bi = _mm256_sub_ps(dr, bi);

        // interleave re,im (input permuted 0,2,1,3)
        z0 = _mm256_unpacklo_ps(br, bi);
        z1 = _mm256_unpackhi_ps(br, bi);
        z2 = _mm256_permute4x64_ps(_mm256_unpackhi_ps(ar, ai), _MM_SHUFFLE(0,1,2,3));   // reversed
        z3 = _mm256_permute4x64_ps(_mm256_unpacklo_ps(ar, ai), _MM_SHUFFLE(0,1,2,3));   // reversed

        _mm256_storeu_ps(&xp0[i+0].re, z0);
        _mm256_storeu_ps(&xp0[i+4].re, z1);
        _mm256_storeu_ps(&xp1[0-i].re, z2);
        _mm256_storeu_ps(&xp1[4-i].re, z3);
    }
}

// in-place complex FFT
// buf[n] in interleaved order
void fft256_AVX2(float buf[512]) {

    complex_t* x = (complex_t*)buf;
    ALIGN32 complex_t y[256];

    fft_radix8_first(x, y, 256, 1);
    fft_radix2_pass(y, x, w_fft_256_0, 256, 8);
    fft_radix4_pass(x, y, w_fft_256_1, 256, 16);
    fft_radix4_last(y, x, w_fft_256_2, 256, 64);

    _mm256_zeroupper();
}

// in-place complex IFFT
// buf[n] in interleaved order
void ifft256_AVX2(float buf[512]) {

    complex_t* x = (complex_t*)buf;
    ALIGN32 complex_t y[256];

    ifft_radix8_first(x, y, 256, 1);
    fft_radix2_pass(y, x, w_fft_256_0, 256, 8);
    fft_radix4_pass(x, y, w_fft_256_1, 256, 16);
    ifft_radix4_last(y, x, w_fft_256_2, 256, 64);

    _mm256_zeroupper();
}

// in-place real FFT
// buf[n] in interleaved order
void rfft512_AVX2(float buf[512]) {

    fft256_AVX2(buf);   // half size complex
    rfft_post((complex_t*)buf, w_rfft_512, 512);

    _mm256_zeroupper();
}

// in-place real IFFT
// buf[n] in interleaved order
void rifft512_AVX2(float buf[512]) {

    rifft_pre((complex_t*)buf, w_rfft_512, 512);
    ifft256_AVX2(buf);  // half size complex

    _mm256_zeroupper();
}

// fft-domain complex multiply-add, for packed complex-conjugate symmetric
// 1 channel input, 2 channel output
void rfft512_cmadd_1X2_AVX2(const float src[512], const float coef0[512], const float coef1[512], float dst0[512], float dst1[512]) {

    // NOTE: x[n/2].re is packed into x[0].im
    float t00 = dst0[0] + src[0] * coef0[0];    // first bin is real
    float t01 = dst0[1] + src[1] * coef0[1];    // last bin is real

    float t10 = dst1[0] + src[0] * coef1[0];    // first bin is real
    float t11 = dst1[1] + src[1] * coef1[1];    // last bin is real

    for (int i = 0; i < 512; i += 8) {

        __m256 arr = _mm256_moveldup_ps(_mm256_loadu_ps(&src[i]));      // [ ar1 ar1 ar0 ar0 ]
        __m256 aii = _mm256_movehdup_ps(_mm256_loadu_ps(&src[i]));      // [ ai1 ai1 ai0 ai0 ]

        __m256 bri = _mm256_loadu_ps(&coef0[i]);                        // [ bi1 br1 bi0 br0 ]
        __m256 bir = _mm256_shuffle_ps(bri, bri, _MM_SHUFFLE(2,3,0,1)); // [ br1 bi1 br0 bi0 ]

        __m256 cri = _mm256_loadu_ps(&coef1[i]);                        // [ ci1 cr1 ci0 cr0 ]
        __m256 cir = _mm256_shuffle_ps(cri, cri, _MM_SHUFFLE(2,3,0,1)); // [ cr1 ci1 cr0 ci0 ]

        __m256 t0 = _mm256_mul_ps(aii, bir);
        __m256 t1 = _mm256_mul_ps(aii, cir);

        t0 = _mm256_fmaddsub_ps(arr, bri, t0);
        t1 = _mm256_fmaddsub_ps(arr, cri, t1);

        t0 = _mm256_add_ps(t0, _mm256_loadu_ps(&dst0[i]));
        t1 = _mm256_add_ps(t1, _mm256_loadu_ps(&dst1[i]));

        _mm256_storeu_ps(&dst0[i], t0);
        _mm256_storeu_ps(&dst1[i], t1);
    }

    // fix the real values
    dst0[0] = t00;
    dst0[1] = t01;

    dst1[0] = t10;
    dst1[1] = t11;

    _mm256_zeroupper();
}

#ifdef FOA_INPUT_FUMA   // input is FuMa (B-format) channel order and normalization

// convert to deinterleaved float (B-format)
void convertInput_AVX2(int16_t* src, float *dst[4], float gain, int numFrames) {

    __m256 scale = _mm256_set1_ps(gain * (1/32768.0f));

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        // sign-extend
        __m256i a0 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+0]));
        __m256i a1 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+8]));
        __m256i a2 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+16]));
        __m256i a3 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+24]));

        // scale
        __m256 x0 = _mm256_mul_ps(_mm256_cvtepi32_ps(a0), scale);
        __m256 x1 = _mm256_mul_ps(_mm256_cvtepi32_ps(a1), scale);
        __m256 x2 = _mm256_mul_ps(_mm256_cvtepi32_ps(a2), scale);
        __m256 x3 = _mm256_mul_ps(_mm256_cvtepi32_ps(a3), scale);

        // deinterleave (4x4 matrix transpose)
        __m256 t0 = _mm256_permute2f128_ps(x0, x2, 0x20);
        __m256 t1 = _mm256_permute2f128_ps(x0, x2, 0x31);
        __m256 t2 = _mm256_permute2f128_ps(x1, x3, 0x20);
        __m256 t3 = _mm256_permute2f128_ps(x1, x3, 0x31);

        x0 = _mm256_shuffle_ps(t0, t1, _MM_SHUFFLE(1,0,1,0));
        x1 = _mm256_shuffle_ps(t0, t1, _MM_SHUFFLE(3,2,3,2));
        x2 = _mm256_shuffle_ps(t2, t3, _MM_SHUFFLE(1,0,1,0));
        x3 = _mm256_shuffle_ps(t2, t3, _MM_SHUFFLE(3,2,3,2));

        t0 = _mm256_shuffle_ps(x0, x2, _MM_SHUFFLE(2,0,2,0));
        t1 = _mm256_shuffle_ps(x0, x2, _MM_SHUFFLE(3,1,3,1));
        t2 = _mm256_shuffle_ps(x1, x3, _MM_SHUFFLE(2,0,2,0));
        t3 = _mm256_shuffle_ps(x1, x3, _MM_SHUFFLE(3,1,3,1));

        _mm256_storeu_ps(&dst[0][i], t0);   // W
        _mm256_storeu_ps(&dst[1][i], t1);   // X
        _mm256_storeu_ps(&dst[2][i], t2);   // Y
        _mm256_storeu_ps(&dst[3][i], t3);   // Z
    }

    _mm256_zeroupper();
}

#else   // input is ambiX (ACN/SN3D) channel order and normalization

// convert to deinterleaved float (B-format)
void convertInput_AVX2(int16_t* src, float *dst[4], float gain, int numFrames) {

    __m256 scale = _mm256_setr_ps(gain * (1/32768.0f) * SQRT1_2,    // -3dB
                                  gain * (1/32768.0f),
                                  gain * (1/32768.0f),
                                  gain * (1/32768.0f),
                                  gain * (1/32768.0f) * SQRT1_2,    // -3dB
                                  gain * (1/32768.0f),
                                  gain * (1/32768.0f),
                                  gain * (1/32768.0f));

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        // sign-extend
        __m256i a0 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+0]));
        __m256i a1 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+8]));
        __m256i a2 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+16]));
        __m256i a3 = _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*)&src[4*i+24]));

        // scale
        __m256 x0 = _mm256_mul_ps(_mm256_cvtepi32_ps(a0), scale);
        __m256 x1 = _mm256_mul_ps(_mm256_cvtepi32_ps(a1), scale);
        __m256 x2 = _mm256_mul_ps(_mm256_cvtepi32_ps(a2), scale);
        __m256 x3 = _mm256_mul_ps(_mm256_cvtepi32_ps(a3), scale);

        // deinterleave (4x4 matrix transpose)
        __m256 t0 = _mm256_permute2f128_ps(x0, x2, 0x20);
        __m256 t1 = _mm256_permute2f128_ps(x0, x2, 0x31);
        __m256 t2 = _mm256_permute2f128_ps(x1, x3, 0x20);
        __m256 t3 = _mm256_permute2f128_ps(x1, x3, 0x31);

        x0 = _mm256_shuffle_ps(t0, t1, _MM_SHUFFLE(1,0,1,0));
        x1 = _mm256_shuffle_ps(t0, t1, _MM_SHUFFLE(3,2,3,2));
        x2 = _mm256_shuffle_ps(t2, t3, _MM_SHUFFLE(1,0,1,0));
        x3 = _mm256_shuffle_ps(t2, t3, _MM_SHUFFLE(3,2,3,2));

        t0 = _mm256_shuffle_ps(x0, x2, _MM_SHUFFLE(2,0,2,0));
        t1 = _mm256_shuffle_ps(x0, x2, _MM_SHUFFLE(3,1,3,1));
        t2 = _mm256_shuffle_ps(x1, x3, _MM_SHUFFLE(2,0,2,0));
        t3 = _mm256_shuffle_ps(x1, x3, _MM_SHUFFLE(3,1,3,1));

        _mm256_storeu_ps(&dst[0][i], t0);   // W
        _mm256_storeu_ps(&dst[2][i], t1);   // y
        _mm256_storeu_ps(&dst[3][i], t2);   // Z
        _mm256_storeu_ps(&dst[1][i], t3);   // X
    }

    _mm256_zeroupper();
}

#endif

// in-place rotation of the soundfield
// crossfade between old and new rotation, to prevent artifacts
void rotate_3x3_AVX2(float* buf[4], const float m0[3][3], const float m1[3][3], const float* win, int numFrames) {

    const float md[3][3] = { 
        { m0[0][0] - m1[0][0], m0[0][1] - m1[0][1], m0[0][2] - m1[0][2] },
        { m0[1][0] - m1[1][0], m0[1][1] - m1[1][1], m0[1][2] - m1[1][2] },
        { m0[2][0] - m1[2][0], m0[2][1] - m1[2][1], m0[2][2] - m1[2][2] },
    };

    assert(numFrames % 8 == 0);

    for (int i = 0; i < numFrames; i += 8) {

        __m256 frac = _mm256_loadu_ps(&win[i]);

        // interpolate the matrix
        __m256 m00 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[0][0]), _mm256_broadcast_ss(&m1[0][0]));
        __m256 m10 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[1][0]), _mm256_broadcast_ss(&m1[1][0]));
        __m256 m20 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[2][0]), _mm256_broadcast_ss(&m1[2][0]));

        __m256 m01 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[0][1]), _mm256_broadcast_ss(&m1[0][1]));
        __m256 m11 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[1][1]), _mm256_broadcast_ss(&m1[1][1]));
        __m256 m21 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[2][1]), _mm256_broadcast_ss(&m1[2][1]));

        __m256 m02 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[0][2]), _mm256_broadcast_ss(&m1[0][2]));
        __m256 m12 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[1][2]), _mm256_broadcast_ss(&m1[1][2]));
        __m256 m22 = _mm256_fmadd_ps(frac, _mm256_broadcast_ss(&md[2][2]), _mm256_broadcast_ss(&m1[2][2]));

        // matrix multiply
        __m256 x = _mm256_mul_ps(m00, _mm256_loadu_ps(&buf[1][i]));
        __m256 y = _mm256_mul_ps(m10, _mm256_loadu_ps(&buf[1][i]));
        __m256 z = _mm256_mul_ps(m20, _mm256_loadu_ps(&buf[1][i]));

        x = _mm256_fmadd_ps(m01, _mm256_loadu_ps(&buf[2][i]), x);
        y = _mm256_fmadd_ps(m11, _mm256_loadu_ps(&buf[2][i]), y);
        z = _mm256_fmadd_ps(m21, _mm256_loadu_ps(&buf[2][i]), z);

        x = _mm256_fmadd_ps(m02, _mm256_loadu_ps(&buf[3][i]), x);
        y = _mm256_fmadd_ps(m12, _mm256_loadu_ps(&buf[3][i]), y);
        z = _mm256_fmadd_ps(m22, _mm256_loadu_ps(&buf[3][i]), z);

        _mm256_storeu_ps(&buf[1][i], x);
        _mm256_storeu_ps(&buf[2][i], y);
        _mm256_storeu_ps(&buf[3][i], z);
    }

    _mm256_zeroupper();
}

#endif
