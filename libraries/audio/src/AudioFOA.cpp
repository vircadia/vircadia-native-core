//
//  AudioFOA.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 10/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <string.h>
#include <assert.h>

#include "AudioFOA.h"
#include "AudioFOAData.h"

#if defined(_MSC_VER)
#define ALIGN32 __declspec(align(32))
#elif defined(__GNUC__)
#define ALIGN32 __attribute__((aligned(32)))
#else
#define ALIGN32
#endif

//
// Vectorized FFT based on radix 2/4/8 Stockham auto-sort
// scalar reference version
//
typedef struct { 
    float re;
    float im;
} complex_t;

static const float SQRT1_2 = 0.707106781f;  // 1/sqrt(2)

static const complex_t w_fft_256_0[] = {
    { 1.000000000e+00f, 0.000000000e+00f }, 
    { 9.238795042e-01f, 3.826834261e-01f }, 
    { 7.071067691e-01f, 7.071067691e-01f }, 
    { 3.826834261e-01f, 9.238795042e-01f }, 
    { 0.000000000e+00f, 1.000000000e+00f }, 
    {-3.826834261e-01f, 9.238795042e-01f }, 
    {-7.071067691e-01f, 7.071067691e-01f }, 
    {-9.238795042e-01f, 3.826834261e-01f }, 
};

static const complex_t w_fft_256_1[] = {
    { 1.000000000e+00f, 0.000000000e+00f }, { 1.000000000e+00f, 0.000000000e+00f }, { 1.000000000e+00f, 0.000000000e+00f }, 
    { 9.951847196e-01f, 9.801714122e-02f }, { 9.807852507e-01f, 1.950903237e-01f }, { 9.569403529e-01f, 2.902846634e-01f }, 
    { 9.807852507e-01f, 1.950903237e-01f }, { 9.238795042e-01f, 3.826834261e-01f }, { 8.314695954e-01f, 5.555702448e-01f }, 
    { 9.569403529e-01f, 2.902846634e-01f }, { 8.314695954e-01f, 5.555702448e-01f }, { 6.343932748e-01f, 7.730104327e-01f }, 
    { 9.238795042e-01f, 3.826834261e-01f }, { 7.071067691e-01f, 7.071067691e-01f }, { 3.826834261e-01f, 9.238795042e-01f }, 
    { 8.819212914e-01f, 4.713967443e-01f }, { 5.555702448e-01f, 8.314695954e-01f }, { 9.801714122e-02f, 9.951847196e-01f }, 
    { 8.314695954e-01f, 5.555702448e-01f }, { 3.826834261e-01f, 9.238795042e-01f }, {-1.950903237e-01f, 9.807852507e-01f }, 
    { 7.730104327e-01f, 6.343932748e-01f }, { 1.950903237e-01f, 9.807852507e-01f }, {-4.713967443e-01f, 8.819212914e-01f }, 
    { 7.071067691e-01f, 7.071067691e-01f }, { 0.000000000e+00f, 1.000000000e+00f }, {-7.071067691e-01f, 7.071067691e-01f }, 
    { 6.343932748e-01f, 7.730104327e-01f }, {-1.950903237e-01f, 9.807852507e-01f }, {-8.819212914e-01f, 4.713967443e-01f }, 
    { 5.555702448e-01f, 8.314695954e-01f }, {-3.826834261e-01f, 9.238795042e-01f }, {-9.807852507e-01f, 1.950903237e-01f }, 
    { 4.713967443e-01f, 8.819212914e-01f }, {-5.555702448e-01f, 8.314695954e-01f }, {-9.951847196e-01f,-9.801714122e-02f }, 
    { 3.826834261e-01f, 9.238795042e-01f }, {-7.071067691e-01f, 7.071067691e-01f }, {-9.238795042e-01f,-3.826834261e-01f }, 
    { 2.902846634e-01f, 9.569403529e-01f }, {-8.314695954e-01f, 5.555702448e-01f }, {-7.730104327e-01f,-6.343932748e-01f }, 
    { 1.950903237e-01f, 9.807852507e-01f }, {-9.238795042e-01f, 3.826834261e-01f }, {-5.555702448e-01f,-8.314695954e-01f }, 
    { 9.801714122e-02f, 9.951847196e-01f }, {-9.807852507e-01f, 1.950903237e-01f }, {-2.902846634e-01f,-9.569403529e-01f }, 
};

static const complex_t w_fft_256_2[] = {
    { 1.000000000e+00f, 0.000000000e+00f }, { 1.000000000e+00f, 0.000000000e+00f }, { 1.000000000e+00f, 0.000000000e+00f }, 
    { 9.996988177e-01f, 2.454122901e-02f }, { 9.987954497e-01f, 4.906767607e-02f }, { 9.972904325e-01f, 7.356456667e-02f }, 
    { 9.987954497e-01f, 4.906767607e-02f }, { 9.951847196e-01f, 9.801714122e-02f }, { 9.891765118e-01f, 1.467304677e-01f }, 
    { 9.972904325e-01f, 7.356456667e-02f }, { 9.891765118e-01f, 1.467304677e-01f }, { 9.757021070e-01f, 2.191012353e-01f }, 
    { 9.951847196e-01f, 9.801714122e-02f }, { 9.807852507e-01f, 1.950903237e-01f }, { 9.569403529e-01f, 2.902846634e-01f }, 
    { 9.924795628e-01f, 1.224106774e-01f }, { 9.700312614e-01f, 2.429801822e-01f }, { 9.329928160e-01f, 3.598950505e-01f }, 
    { 9.891765118e-01f, 1.467304677e-01f }, { 9.569403529e-01f, 2.902846634e-01f }, { 9.039893150e-01f, 4.275550842e-01f }, 
    { 9.852776527e-01f, 1.709618866e-01f }, { 9.415440559e-01f, 3.368898630e-01f }, { 8.700869679e-01f, 4.928981960e-01f }, 
    { 9.807852507e-01f, 1.950903237e-01f }, { 9.238795042e-01f, 3.826834261e-01f }, { 8.314695954e-01f, 5.555702448e-01f }, 
    { 9.757021070e-01f, 2.191012353e-01f }, { 9.039893150e-01f, 4.275550842e-01f }, { 7.883464098e-01f, 6.152315736e-01f }, 
    { 9.700312614e-01f, 2.429801822e-01f }, { 8.819212914e-01f, 4.713967443e-01f }, { 7.409511209e-01f, 6.715589762e-01f }, 
    { 9.637760520e-01f, 2.667127550e-01f }, { 8.577286005e-01f, 5.141027570e-01f }, { 6.895405650e-01f, 7.242470980e-01f }, 
    { 9.569403529e-01f, 2.902846634e-01f }, { 8.314695954e-01f, 5.555702448e-01f }, { 6.343932748e-01f, 7.730104327e-01f }, 
    { 9.495281577e-01f, 3.136817515e-01f }, { 8.032075167e-01f, 5.956993103e-01f }, { 5.758081675e-01f, 8.175848126e-01f }, 
    { 9.415440559e-01f, 3.368898630e-01f }, { 7.730104327e-01f, 6.343932748e-01f }, { 5.141027570e-01f, 8.577286005e-01f }, 
    { 9.329928160e-01f, 3.598950505e-01f }, { 7.409511209e-01f, 6.715589762e-01f }, { 4.496113360e-01f, 8.932242990e-01f }, 
    { 9.238795042e-01f, 3.826834261e-01f }, { 7.071067691e-01f, 7.071067691e-01f }, { 3.826834261e-01f, 9.238795042e-01f }, 
    { 9.142097831e-01f, 4.052413106e-01f }, { 6.715589762e-01f, 7.409511209e-01f }, { 3.136817515e-01f, 9.495281577e-01f }, 
    { 9.039893150e-01f, 4.275550842e-01f }, { 6.343932748e-01f, 7.730104327e-01f }, { 2.429801822e-01f, 9.700312614e-01f }, 
    { 8.932242990e-01f, 4.496113360e-01f }, { 5.956993103e-01f, 8.032075167e-01f }, { 1.709618866e-01f, 9.852776527e-01f }, 
    { 8.819212914e-01f, 4.713967443e-01f }, { 5.555702448e-01f, 8.314695954e-01f }, { 9.801714122e-02f, 9.951847196e-01f }, 
    { 8.700869679e-01f, 4.928981960e-01f }, { 5.141027570e-01f, 8.577286005e-01f }, { 2.454122901e-02f, 9.996988177e-01f }, 
    { 8.577286005e-01f, 5.141027570e-01f }, { 4.713967443e-01f, 8.819212914e-01f }, {-4.906767607e-02f, 9.987954497e-01f }, 
    { 8.448535800e-01f, 5.349976420e-01f }, { 4.275550842e-01f, 9.039893150e-01f }, {-1.224106774e-01f, 9.924795628e-01f }, 
    { 8.314695954e-01f, 5.555702448e-01f }, { 3.826834261e-01f, 9.238795042e-01f }, {-1.950903237e-01f, 9.807852507e-01f }, 
    { 8.175848126e-01f, 5.758081675e-01f }, { 3.368898630e-01f, 9.415440559e-01f }, {-2.667127550e-01f, 9.637760520e-01f }, 
    { 8.032075167e-01f, 5.956993103e-01f }, { 2.902846634e-01f, 9.569403529e-01f }, {-3.368898630e-01f, 9.415440559e-01f }, 
    { 7.883464098e-01f, 6.152315736e-01f }, { 2.429801822e-01f, 9.700312614e-01f }, {-4.052413106e-01f, 9.142097831e-01f }, 
    { 7.730104327e-01f, 6.343932748e-01f }, { 1.950903237e-01f, 9.807852507e-01f }, {-4.713967443e-01f, 8.819212914e-01f }, 
    { 7.572088242e-01f, 6.531728506e-01f }, { 1.467304677e-01f, 9.891765118e-01f }, {-5.349976420e-01f, 8.448535800e-01f }, 
    { 7.409511209e-01f, 6.715589762e-01f }, { 9.801714122e-02f, 9.951847196e-01f }, {-5.956993103e-01f, 8.032075167e-01f }, 
    { 7.242470980e-01f, 6.895405650e-01f }, { 4.906767607e-02f, 9.987954497e-01f }, {-6.531728506e-01f, 7.572088242e-01f }, 
    { 7.071067691e-01f, 7.071067691e-01f }, { 0.000000000e+00f, 1.000000000e+00f }, {-7.071067691e-01f, 7.071067691e-01f }, 
    { 6.895405650e-01f, 7.242470980e-01f }, {-4.906767607e-02f, 9.987954497e-01f }, {-7.572088242e-01f, 6.531728506e-01f }, 
    { 6.715589762e-01f, 7.409511209e-01f }, {-9.801714122e-02f, 9.951847196e-01f }, {-8.032075167e-01f, 5.956993103e-01f }, 
    { 6.531728506e-01f, 7.572088242e-01f }, {-1.467304677e-01f, 9.891765118e-01f }, {-8.448535800e-01f, 5.349976420e-01f }, 
    { 6.343932748e-01f, 7.730104327e-01f }, {-1.950903237e-01f, 9.807852507e-01f }, {-8.819212914e-01f, 4.713967443e-01f }, 
    { 6.152315736e-01f, 7.883464098e-01f }, {-2.429801822e-01f, 9.700312614e-01f }, {-9.142097831e-01f, 4.052413106e-01f }, 
    { 5.956993103e-01f, 8.032075167e-01f }, {-2.902846634e-01f, 9.569403529e-01f }, {-9.415440559e-01f, 3.368898630e-01f }, 
    { 5.758081675e-01f, 8.175848126e-01f }, {-3.368898630e-01f, 9.415440559e-01f }, {-9.637760520e-01f, 2.667127550e-01f }, 
    { 5.555702448e-01f, 8.314695954e-01f }, {-3.826834261e-01f, 9.238795042e-01f }, {-9.807852507e-01f, 1.950903237e-01f }, 
    { 5.349976420e-01f, 8.448535800e-01f }, {-4.275550842e-01f, 9.039893150e-01f }, {-9.924795628e-01f, 1.224106774e-01f }, 
    { 5.141027570e-01f, 8.577286005e-01f }, {-4.713967443e-01f, 8.819212914e-01f }, {-9.987954497e-01f, 4.906767607e-02f }, 
    { 4.928981960e-01f, 8.700869679e-01f }, {-5.141027570e-01f, 8.577286005e-01f }, {-9.996988177e-01f,-2.454122901e-02f }, 
    { 4.713967443e-01f, 8.819212914e-01f }, {-5.555702448e-01f, 8.314695954e-01f }, {-9.951847196e-01f,-9.801714122e-02f }, 
    { 4.496113360e-01f, 8.932242990e-01f }, {-5.956993103e-01f, 8.032075167e-01f }, {-9.852776527e-01f,-1.709618866e-01f }, 
    { 4.275550842e-01f, 9.039893150e-01f }, {-6.343932748e-01f, 7.730104327e-01f }, {-9.700312614e-01f,-2.429801822e-01f }, 
    { 4.052413106e-01f, 9.142097831e-01f }, {-6.715589762e-01f, 7.409511209e-01f }, {-9.495281577e-01f,-3.136817515e-01f }, 
    { 3.826834261e-01f, 9.238795042e-01f }, {-7.071067691e-01f, 7.071067691e-01f }, {-9.238795042e-01f,-3.826834261e-01f }, 
    { 3.598950505e-01f, 9.329928160e-01f }, {-7.409511209e-01f, 6.715589762e-01f }, {-8.932242990e-01f,-4.496113360e-01f }, 
    { 3.368898630e-01f, 9.415440559e-01f }, {-7.730104327e-01f, 6.343932748e-01f }, {-8.577286005e-01f,-5.141027570e-01f }, 
    { 3.136817515e-01f, 9.495281577e-01f }, {-8.032075167e-01f, 5.956993103e-01f }, {-8.175848126e-01f,-5.758081675e-01f }, 
    { 2.902846634e-01f, 9.569403529e-01f }, {-8.314695954e-01f, 5.555702448e-01f }, {-7.730104327e-01f,-6.343932748e-01f }, 
    { 2.667127550e-01f, 9.637760520e-01f }, {-8.577286005e-01f, 5.141027570e-01f }, {-7.242470980e-01f,-6.895405650e-01f }, 
    { 2.429801822e-01f, 9.700312614e-01f }, {-8.819212914e-01f, 4.713967443e-01f }, {-6.715589762e-01f,-7.409511209e-01f }, 
    { 2.191012353e-01f, 9.757021070e-01f }, {-9.039893150e-01f, 4.275550842e-01f }, {-6.152315736e-01f,-7.883464098e-01f }, 
    { 1.950903237e-01f, 9.807852507e-01f }, {-9.238795042e-01f, 3.826834261e-01f }, {-5.555702448e-01f,-8.314695954e-01f }, 
    { 1.709618866e-01f, 9.852776527e-01f }, {-9.415440559e-01f, 3.368898630e-01f }, {-4.928981960e-01f,-8.700869679e-01f }, 
    { 1.467304677e-01f, 9.891765118e-01f }, {-9.569403529e-01f, 2.902846634e-01f }, {-4.275550842e-01f,-9.039893150e-01f }, 
    { 1.224106774e-01f, 9.924795628e-01f }, {-9.700312614e-01f, 2.429801822e-01f }, {-3.598950505e-01f,-9.329928160e-01f }, 
    { 9.801714122e-02f, 9.951847196e-01f }, {-9.807852507e-01f, 1.950903237e-01f }, {-2.902846634e-01f,-9.569403529e-01f }, 
    { 7.356456667e-02f, 9.972904325e-01f }, {-9.891765118e-01f, 1.467304677e-01f }, {-2.191012353e-01f,-9.757021070e-01f }, 
    { 4.906767607e-02f, 9.987954497e-01f }, {-9.951847196e-01f, 9.801714122e-02f }, {-1.467304677e-01f,-9.891765118e-01f }, 
    { 2.454122901e-02f, 9.996988177e-01f }, {-9.987954497e-01f, 4.906767607e-02f }, {-7.356456667e-02f,-9.972904325e-01f }, 
};

static complex_t w_rfft_512[] = {
    {-4.999623597e-01f, 4.938642383e-01f }, {-4.998494089e-01f, 4.877293706e-01f }, 
    {-4.996611774e-01f, 4.815963805e-01f }, {-4.993977249e-01f, 4.754661620e-01f }, 
    {-4.990590513e-01f, 4.693396389e-01f }, {-4.986452162e-01f, 4.632177055e-01f }, 
    {-4.981563091e-01f, 4.571013451e-01f }, {-4.975923598e-01f, 4.509914219e-01f }, 
    {-4.969534874e-01f, 4.448888898e-01f }, {-4.962397814e-01f, 4.387946725e-01f }, 
    {-4.954513311e-01f, 4.327096343e-01f }, {-4.945882559e-01f, 4.266347587e-01f }, 
    {-4.936507046e-01f, 4.205709100e-01f }, {-4.926388264e-01f, 4.145190716e-01f }, 
    {-4.915527403e-01f, 4.084800482e-01f }, {-4.903926253e-01f, 4.024548531e-01f }, 
    {-4.891586900e-01f, 3.964443207e-01f }, {-4.878510535e-01f, 3.904493749e-01f }, 
    {-4.864699841e-01f, 3.844709396e-01f }, {-4.850156307e-01f, 3.785099089e-01f }, 
    {-4.834882319e-01f, 3.725671768e-01f }, {-4.818880260e-01f, 3.666436076e-01f }, 
    {-4.802152514e-01f, 3.607401550e-01f }, {-4.784701765e-01f, 3.548576832e-01f }, 
    {-4.766530097e-01f, 3.489970267e-01f }, {-4.747640789e-01f, 3.431591392e-01f }, 
    {-4.728036523e-01f, 3.373448551e-01f }, {-4.707720280e-01f, 3.315550685e-01f }, 
    {-4.686695039e-01f, 3.257906437e-01f }, {-4.664964080e-01f, 3.200524747e-01f }, 
    {-4.642530382e-01f, 3.143413961e-01f }, {-4.619397521e-01f, 3.086583018e-01f }, 
    {-4.595569372e-01f, 3.030039668e-01f }, {-4.571048915e-01f, 2.973793447e-01f }, 
    {-4.545840025e-01f, 2.917852402e-01f }, {-4.519946575e-01f, 2.862224579e-01f }, 
    {-4.493372440e-01f, 2.806918621e-01f }, {-4.466121495e-01f, 2.751943469e-01f }, 
    {-4.438198209e-01f, 2.697306275e-01f }, {-4.409606457e-01f, 2.643016279e-01f }, 
    {-4.380350411e-01f, 2.589080930e-01f }, {-4.350434840e-01f, 2.535508871e-01f }, 
    {-4.319864213e-01f, 2.482308149e-01f }, {-4.288643003e-01f, 2.429486215e-01f }, 
    {-4.256775975e-01f, 2.377051711e-01f }, {-4.224267900e-01f, 2.325011790e-01f }, 
    {-4.191123545e-01f, 2.273375094e-01f }, {-4.157347977e-01f, 2.222148776e-01f }, 
    {-4.122946560e-01f, 2.171340883e-01f }, {-4.087924063e-01f, 2.120959163e-01f }, 
    {-4.052285850e-01f, 2.071010768e-01f }, {-4.016037583e-01f, 2.021503448e-01f }, 
    {-3.979184628e-01f, 1.972444654e-01f }, {-3.941732049e-01f, 1.923842132e-01f }, 
    {-3.903686106e-01f, 1.875702441e-01f }, {-3.865052164e-01f, 1.828033626e-01f }, 
    {-3.825836182e-01f, 1.780842245e-01f }, {-3.786044121e-01f, 1.734135747e-01f }, 
    {-3.745681942e-01f, 1.687920988e-01f }, {-3.704755604e-01f, 1.642205119e-01f }, 
    {-3.663271368e-01f, 1.596994996e-01f }, {-3.621235490e-01f, 1.552297175e-01f }, 
    {-3.578654230e-01f, 1.508118808e-01f }, {-3.535533845e-01f, 1.464466155e-01f }, 
    {-3.491881192e-01f, 1.421345770e-01f }, {-3.447702825e-01f, 1.378764510e-01f }, 
    {-3.403005004e-01f, 1.336728632e-01f }, {-3.357794881e-01f, 1.295244396e-01f }, 
    {-3.312079012e-01f, 1.254318058e-01f }, {-3.265864253e-01f, 1.213955879e-01f }, 
    {-3.219157755e-01f, 1.174163818e-01f }, {-3.171966374e-01f, 1.134947836e-01f }, 
    {-3.124297559e-01f, 1.096313894e-01f }, {-3.076157868e-01f, 1.058267951e-01f }, 
    {-3.027555346e-01f, 1.020815372e-01f }, {-2.978496552e-01f, 9.839624166e-02f }, 
    {-2.928989232e-01f, 9.477141500e-02f }, {-2.879040837e-01f, 9.120759368e-02f }, 
    {-2.828659117e-01f, 8.770534396e-02f }, {-2.777851224e-01f, 8.426520228e-02f }, 
    {-2.726624906e-01f, 8.088764548e-02f }, {-2.674988210e-01f, 7.757321000e-02f }, 
    {-2.622948289e-01f, 7.432240248e-02f }, {-2.570513785e-01f, 7.113569975e-02f }, 
    {-2.517691851e-01f, 6.801357865e-02f }, {-2.464490980e-01f, 6.495651603e-02f }, 
    {-2.410918921e-01f, 6.196495891e-02f }, {-2.356983721e-01f, 5.903935432e-02f }, 
    {-2.302693576e-01f, 5.618017912e-02f }, {-2.248056680e-01f, 5.338785052e-02f }, 
    {-2.193081230e-01f, 5.066275597e-02f }, {-2.137775421e-01f, 4.800534248e-02f }, 
    {-2.082147747e-01f, 4.541599751e-02f }, {-2.026206553e-01f, 4.289510846e-02f }, 
    {-1.969960183e-01f, 4.044306278e-02f }, {-1.913417131e-01f, 3.806024790e-02f }, 
    {-1.856586039e-01f, 3.574696183e-02f }, {-1.799475253e-01f, 3.350359201e-02f }, 
    {-1.742093414e-01f, 3.133049607e-02f }, {-1.684449315e-01f, 2.922797203e-02f }, 
    {-1.626551449e-01f, 2.719634771e-02f }, {-1.568408757e-01f, 2.523592114e-02f }, 
    {-1.510029733e-01f, 2.334699035e-02f }, {-1.451423317e-01f, 2.152982354e-02f }, 
    {-1.392598450e-01f, 1.978474855e-02f }, {-1.333563775e-01f, 1.811197400e-02f }, 
    {-1.274328232e-01f, 1.651176810e-02f }, {-1.214900911e-01f, 1.498436928e-02f }, 
    {-1.155290529e-01f, 1.353001595e-02f }, {-1.095506176e-01f, 1.214894652e-02f }, 
    {-1.035556868e-01f, 1.084131002e-02f }, {-9.754516184e-02f, 9.607374668e-03f }, 
    {-9.151994437e-02f, 8.447259665e-03f }, {-8.548094332e-02f, 7.361173630e-03f }, 
    {-7.942907512e-02f, 6.349295378e-03f }, {-7.336523384e-02f, 5.411744118e-03f }, 
    {-6.729035079e-02f, 4.548668861e-03f }, {-6.120533869e-02f, 3.760218620e-03f }, 
    {-5.511110276e-02f, 3.046512604e-03f }, {-4.900857061e-02f, 2.407640219e-03f }, 
    {-4.289865494e-02f, 1.843690872e-03f }, {-3.678228334e-02f, 1.354783773e-03f }, 
    {-3.066036850e-02f, 9.409487247e-04f }, {-2.453383803e-02f, 6.022751331e-04f }, 
    {-1.840361208e-02f, 3.388226032e-04f }, {-1.227061450e-02f, 1.505911350e-04f }, 
    {-6.135769188e-03f, 3.764033318e-05f }, {-0.000000000e+00f, 0.000000000e+00f }, 
};

// x[n] in interleaved order
// y[n] in interleaved order
// n >= 2
static void fft_radix2_pass(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/2;
    assert(n >= 1);
    assert(p >= 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];

    for (size_t i = 0; i < t; i++) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 2*(i-k) + k; // output index

        float ar0 = xp0[i].re;
        float ai0 = xp0[i].im;
        float ar1 = xp1[i].re;
        float ai1 = xp1[i].im;

        float wr1 = w[k].re;
        float wi1 = w[k].im;

        float br1 = ar1 * wr1;
        float bi1 = ai1 * wr1;

        br1 += ai1 * wi1;
        bi1 -= ar1 * wi1;

        float cr0 = ar0 + br1;
        float ci0 = ai0 + bi1;
        float cr1 = ar0 - br1;
        float ci1 = ai0 - bi1;

        yp0[j].re = cr0;
        yp0[j].im = ci0;
        yp1[j].re = cr1;
        yp1[j].im = ci1;
    }
}

// x[n] in interleaved order
// y[n] in interleaved order
// n >= 4
static void fft_radix4_pass(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/4;
    assert(n >= 1);
    assert(p >= 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];
    complex_t* yp2 = &y[2*p];
    complex_t* yp3 = &y[3*p];

    for (size_t i = 0; i < t; i++) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 4*(i-k) + k; // output index

		float ar1 = xp1[i].re;
		float ai1 = xp1[i].im;
		float ar2 = xp2[i].re;
		float ai2 = xp2[i].im;
		float ar3 = xp3[i].re;
		float ai3 = xp3[i].im;

		float wr1 = w[3*k+0].re;
		float wi1 = w[3*k+0].im;
		float wr2 = w[3*k+1].re;
		float wi2 = w[3*k+1].im;
		float wr3 = w[3*k+2].re;
		float wi3 = w[3*k+2].im;

		float br1 = ar1 * wr1;
		float bi1 = ai1 * wr1;
		float br2 = ar2 * wr2;
		float bi2 = ai2 * wr2;
		float br3 = ar3 * wr3;
		float bi3 = ai3 * wr3;

		br1 += ai1 * wi1;
		bi1 -= ar1 * wi1;
		br2 += ai2 * wi2;
		bi2 -= ar2 * wi2;
		br3 += ai3 * wi3;
		bi3 -= ar3 * wi3;

		float ar0 = xp0[i].re;
		float ai0 = xp0[i].im;

		float cr0 = ar0 + br2;
		float ci0 = ai0 + bi2;
		float cr2 = ar0 - br2;
		float ci2 = ai0 - bi2;
		float cr1 = br1 + br3;
		float ci1 = bi1 + bi3;
		float cr3 = br1 - br3;
		float ci3 = bi1 - bi3;

        float dr0 = cr0 + cr1;
        float di0 = ci0 + ci1;
        float dr1 = cr2 + ci3;
        float di1 = ci2 - cr3;
        float dr2 = cr0 - cr1;
        float di2 = ci0 - ci1;
        float dr3 = cr2 - ci3;
        float di3 = ci2 + cr3;

		yp0[j].re = dr0;
		yp0[j].im = di0;
		yp1[j].re = dr1;
		yp1[j].im = di1;
		yp2[j].re = dr2;
		yp2[j].im = di2;
		yp3[j].re = dr3;
		yp3[j].im = di3;
    }
}

static auto& fft_radix4_last = fft_radix4_pass;

// x[n] in interleaved order
// y[n] in interleaved order
// n >= 4
static void ifft_radix4_last(complex_t* x, complex_t* y, const complex_t* w, int n, int p) {

    size_t t = n/4;
    assert(n >= 1);
    assert(p >= 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];

    complex_t* yp0 = &y[0*p];
    complex_t* yp1 = &y[1*p];
    complex_t* yp2 = &y[2*p];
    complex_t* yp3 = &y[3*p];

    for (size_t i = 0; i < t; i++) {

        size_t k = i & (p-1);   // twiddle index
        size_t j = 4*(i-k) + k; // output index

		float ar1 = xp1[i].re;
		float ai1 = xp1[i].im;
		float ar2 = xp2[i].re;
		float ai2 = xp2[i].im;
		float ar3 = xp3[i].re;
		float ai3 = xp3[i].im;

		float wr1 = w[3*k+0].re;
		float wi1 = w[3*k+0].im;
		float wr2 = w[3*k+1].re;
		float wi2 = w[3*k+1].im;
		float wr3 = w[3*k+2].re;
		float wi3 = w[3*k+2].im;

		float br1 = ar1 * wr1;
		float bi1 = ai1 * wr1;
		float br2 = ar2 * wr2;
		float bi2 = ai2 * wr2;
		float br3 = ar3 * wr3;
		float bi3 = ai3 * wr3;

		br1 += ai1 * wi1;
		bi1 -= ar1 * wi1;
		br2 += ai2 * wi2;
		bi2 -= ar2 * wi2;
		br3 += ai3 * wi3;
		bi3 -= ar3 * wi3;

		float ar0 = xp0[i].re;
		float ai0 = xp0[i].im;

		float cr0 = ar0 + br2;
		float ci0 = ai0 + bi2;
		float cr2 = ar0 - br2;
		float ci2 = ai0 - bi2;
		float cr1 = br1 + br3;
		float ci1 = bi1 + bi3;
		float cr3 = br1 - br3;
		float ci3 = bi1 - bi3;

        float dr0 = cr0 + cr1;
        float di0 = ci0 + ci1;
        float dr1 = cr2 + ci3;
        float di1 = ci2 - cr3;
        float dr2 = cr0 - cr1;
        float di2 = ci0 - ci1;
        float dr3 = cr2 - ci3;
        float di3 = ci2 + cr3;

        float scale = 1.0f/n;   // scaled by 1/n for ifft
        dr0 *= scale;
        di0 *= scale;
        dr1 *= scale;
        di1 *= scale;
        dr2 *= scale;
        di2 *= scale;
        dr3 *= scale;
        di3 *= scale;

		yp0[j].re = di0;    // swapped re,im for ifft
		yp0[j].im = dr0;
		yp1[j].re = di1;
		yp1[j].im = dr1;
		yp2[j].re = di2;
		yp2[j].im = dr2;
		yp3[j].re = di3;
		yp3[j].im = dr3;
    }
}

// x[n] in interleaved order
// y[n] in interleaved order
// n >= 8
static void fft_radix8_first(complex_t* x, complex_t* y, int n, int p) {

    size_t t = n/8;
    assert(t >= 1);
    assert(p == 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];
    complex_t* xp4 = &x[4*t];
    complex_t* xp5 = &x[5*t];
    complex_t* xp6 = &x[6*t];
    complex_t* xp7 = &x[7*t];

    for (size_t i = 0; i < t; i++) {

        //
        // even
        //
		float ar0 = xp0[i].re;
		float ai0 = xp0[i].im;
		float ar2 = xp2[i].re;
		float ai2 = xp2[i].im;
		float ar4 = xp4[i].re;
		float ai4 = xp4[i].im;
		float ar6 = xp6[i].re;
		float ai6 = xp6[i].im;

		float cr0 = ar0 + ar4;
		float ci0 = ai0 + ai4;
		float cr2 = ar2 + ar6;
		float ci2 = ai2 + ai6;
		float cr4 = ar0 - ar4;
		float ci4 = ai0 - ai4;
		float cr6 = ar2 - ar6;
		float ci6 = ai2 - ai6;

		float dr0 = cr0 + cr2;
		float di0 = ci0 + ci2;
		float dr2 = cr0 - cr2;
		float di2 = ci0 - ci2;
		float dr4 = cr4 + ci6;
		float di4 = ci4 - cr6;
		float dr6 = cr4 - ci6;
		float di6 = ci4 + cr6;

        //
        // odd
        //
		float ar1 = xp1[i].re;
		float ai1 = xp1[i].im;
		float ar3 = xp3[i].re;
		float ai3 = xp3[i].im;
		float ar5 = xp5[i].re;
		float ai5 = xp5[i].im;
		float ar7 = xp7[i].re;
		float ai7 = xp7[i].im;

		float br1 = ar1 + ar5;
		float bi1 = ai1 + ai5;
		float br3 = ar3 + ar7;
		float bi3 = ai3 + ai7;
		float br5 = ar1 - ar5;
		float bi5 = ai1 - ai5;
		float br7 = ar3 - ar7;
		float bi7 = ai3 - ai7;

		float cr1 = br1;
		float ci1 = bi1;
		float cr3 = br3;
		float ci3 = bi3;
		float cr5 = bi5 + br5;
		float ci5 = bi5 - br5;
		float cr7 = bi7 - br7;
		float ci7 = bi7 + br7;

		cr5 *= SQRT1_2;
		ci5 *= SQRT1_2;
		cr7 *= SQRT1_2;
		ci7 *= SQRT1_2;

		float dr1 = cr1 + cr3;
		float di1 = ci1 + ci3;
		float dr3 = cr1 - cr3;
		float di3 = ci1 - ci3;
		float dr5 = cr5 + cr7;
		float di5 = ci5 - ci7;
		float dr7 = cr5 - cr7;
		float di7 = ci5 + ci7;

        //
        // merge
        //
        float er0 = dr0 + dr1;
        float ei0 = di0 + di1;
        float er1 = dr4 + dr5;
        float ei1 = di4 + di5;
        float er2 = dr2 + di3;
        float ei2 = di2 - dr3;
        float er3 = dr6 + di7;
        float ei3 = di6 - dr7;
        float er4 = dr0 - dr1;
        float ei4 = di0 - di1;
        float er5 = dr4 - dr5;
        float ei5 = di4 - di5;
        float er6 = dr2 - di3;
        float ei6 = di2 + dr3;
        float er7 = dr6 - di7;
        float ei7 = di6 + dr7;

		y[8*i+0].re = er0;
		y[8*i+0].im = ei0;
		y[8*i+1].re = er1;
		y[8*i+1].im = ei1;
		y[8*i+2].re = er2;
		y[8*i+2].im = ei2;
		y[8*i+3].re = er3;
		y[8*i+3].im = ei3;
		y[8*i+4].re = er4;
		y[8*i+4].im = ei4;
		y[8*i+5].re = er5;
		y[8*i+5].im = ei5;
		y[8*i+6].re = er6;
		y[8*i+6].im = ei6;
		y[8*i+7].re = er7;
		y[8*i+7].im = ei7;
	}
}

// x[n] in interleaved order
// y[n] in interleaved order
// n >= 8
static void ifft_radix8_first(complex_t* x, complex_t* y, int n, int p) {

    size_t t = n/8;
    assert(t >= 1);
    assert(p == 1);

    complex_t* xp0 = &x[0*t];
    complex_t* xp1 = &x[1*t];
    complex_t* xp2 = &x[2*t];
    complex_t* xp3 = &x[3*t];
    complex_t* xp4 = &x[4*t];
    complex_t* xp5 = &x[5*t];
    complex_t* xp6 = &x[6*t];
    complex_t* xp7 = &x[7*t];

    for (size_t i = 0; i < t; i++) {

        //
        // even
        //
		float ar0 = xp0[i].im;  // swapped re,im for ifft
		float ai0 = xp0[i].re;
		float ar2 = xp2[i].im;
		float ai2 = xp2[i].re;
		float ar4 = xp4[i].im;
		float ai4 = xp4[i].re;
		float ar6 = xp6[i].im;
		float ai6 = xp6[i].re;

		float cr0 = ar0 + ar4;
		float ci0 = ai0 + ai4;
		float cr2 = ar2 + ar6;
		float ci2 = ai2 + ai6;
		float cr4 = ar0 - ar4;
		float ci4 = ai0 - ai4;
		float cr6 = ar2 - ar6;
		float ci6 = ai2 - ai6;

		float dr0 = cr0 + cr2;
		float di0 = ci0 + ci2;
		float dr2 = cr0 - cr2;
		float di2 = ci0 - ci2;
		float dr4 = cr4 + ci6;
		float di4 = ci4 - cr6;
		float dr6 = cr4 - ci6;
		float di6 = ci4 + cr6;

        //
        // odd
        //
		float ar1 = xp1[i].im;  // swapped re,im for ifft
		float ai1 = xp1[i].re;
		float ar3 = xp3[i].im;
		float ai3 = xp3[i].re;
		float ar5 = xp5[i].im;
		float ai5 = xp5[i].re;
		float ar7 = xp7[i].im;
		float ai7 = xp7[i].re;

		float br1 = ar1 + ar5;
		float bi1 = ai1 + ai5;
		float br3 = ar3 + ar7;
		float bi3 = ai3 + ai7;
		float br5 = ar1 - ar5;
		float bi5 = ai1 - ai5;
		float br7 = ar3 - ar7;
		float bi7 = ai3 - ai7;

		float cr1 = br1;
		float ci1 = bi1;
		float cr3 = br3;
		float ci3 = bi3;
		float cr5 = bi5 + br5;
		float ci5 = bi5 - br5;
		float cr7 = bi7 - br7;
		float ci7 = bi7 + br7;

		cr5 *= SQRT1_2;
		ci5 *= SQRT1_2;
		cr7 *= SQRT1_2;
		ci7 *= SQRT1_2;

		float dr1 = cr1 + cr3;
		float di1 = ci1 + ci3;
		float dr3 = cr1 - cr3;
		float di3 = ci1 - ci3;
		float dr5 = cr5 + cr7;
		float di5 = ci5 - ci7;
		float dr7 = cr5 - cr7;
		float di7 = ci5 + ci7;

        //
        // merge
        //
        float er0 = dr0 + dr1;
        float ei0 = di0 + di1;
        float er1 = dr4 + dr5;
        float ei1 = di4 + di5;
        float er2 = dr2 + di3;
        float ei2 = di2 - dr3;
        float er3 = dr6 + di7;
        float ei3 = di6 - dr7;
        float er4 = dr0 - dr1;
        float ei4 = di0 - di1;
        float er5 = dr4 - dr5;
        float ei5 = di4 - di5;
        float er6 = dr2 - di3;
        float ei6 = di2 + dr3;
        float er7 = dr6 - di7;
        float ei7 = di6 + dr7;

		y[8*i+0].re = er0;
		y[8*i+0].im = ei0;
		y[8*i+1].re = er1;
		y[8*i+1].im = ei1;
		y[8*i+2].re = er2;
		y[8*i+2].im = ei2;
		y[8*i+3].re = er3;
		y[8*i+3].im = ei3;
		y[8*i+4].re = er4;
		y[8*i+4].im = ei4;
		y[8*i+5].re = er5;
		y[8*i+5].im = ei5;
		y[8*i+6].re = er6;
		y[8*i+6].im = ei6;
		y[8*i+7].re = er7;
		y[8*i+7].im = ei7;
	}
}

// x[n] in interleaved order
// n >= 4
static void rfft_post(complex_t* x, const complex_t* w, int n) {


    // NOTE: x[n/2].re is packed into x[0].im
    float t = x[0].re;
    x[0].re = t + x[0].im;
    x[0].im = t - x[0].im;

    complex_t* xp0 = &x[1];
    complex_t* xp1 = &x[n/2 - 1];

    assert(n/4 >= 1);
    for (size_t i = 0; i < n/4; i++) {

        float ar = xp0[i].re;
        float ai = xp0[i].im;
        float br = xp1[0-i].re;
        float bi = xp1[0-i].im;

        float wr = w[i].re;
        float wi = w[i].im;

        float cr = ar - br;
        float ci = ai + bi;

        float dr = cr * wr;
        float di = ci * wr;

        dr += ci * wi;
        di -= cr * wi;

        ar = ar + di;
        ai = dr - ai;

        br = br - di;
        bi = dr - bi;

        xp0[i].re = br;
        xp0[i].im = bi;
        xp1[0-i].re = ar;
        xp1[0-i].im = ai;
    }
}

// x[n] in interleaved order
// n >= 4
static void rifft_pre(complex_t* x, const complex_t* w, int n) {

    // NOTE: x[n/2].re is packed into x[0].im
    float t = x[0].re;
    x[0].re = 0.5f * (t + x[0].im); // halved for ifft
    x[0].im = 0.5f * (t - x[0].im); // halved for ifft

    complex_t* xp0 = &x[1];
    complex_t* xp1 = &x[n/2 - 1];

    assert(n/4 >= 1);
    for (size_t i = 0; i < n/4; i++) {

        float ar = xp0[i].re;
        float ai = xp0[i].im;
        float br = xp1[0-i].re;
        float bi = xp1[0-i].im;

        float wr = w[i].re; // negated for ifft
        float wi = w[i].im;

        float cr = br - ar;
        float ci = ai + bi;

        float dr = cr * wr;
        float di = cr * wi;

        dr += ci * wi;
        di -= ci * wr;

        ar = ar + di;
        ai = dr - ai;

        br = br - di;
        bi = dr - bi;

        xp0[i].re = br;
        xp0[i].im = bi;
        xp1[0-i].re = ar;
        xp1[0-i].im = ai;
    }
}

// in-place complex FFT
// buf[n] in interleaved order
static void fft256_ref(float buf[512]) {

    complex_t* x = (complex_t*)buf;
    complex_t y[256];

    fft_radix8_first(x, y, 256, 1);
    fft_radix2_pass(y, x, w_fft_256_0, 256, 8);
    fft_radix4_pass(x, y, w_fft_256_1, 256, 16);
    fft_radix4_last(y, x, w_fft_256_2, 256, 64);
}

// in-place complex IFFT
// buf[n] in interleaved order
static void ifft256_ref(float buf[512]) {

    complex_t* x = (complex_t*)buf;
    complex_t y[256];

    ifft_radix8_first(x, y, 256, 1);
    fft_radix2_pass(y, x, w_fft_256_0, 256, 8);
    fft_radix4_pass(x, y, w_fft_256_1, 256, 16);
    ifft_radix4_last(y, x, w_fft_256_2, 256, 64);
}

// in-place real FFT
// buf[n] in interleaved order
static void rfft512_ref(float buf[512]) {

    fft256_ref(buf);    // half size complex
    rfft_post((complex_t*)buf, w_rfft_512, 512);
}

// in-place real IFFT
// buf[n] in interleaved order
static void rifft512_ref(float buf[512]) {

    rifft_pre((complex_t*)buf, w_rfft_512, 512);
    ifft256_ref(buf);   // half size complex
}

// fft-domain complex multiply-add, for packed complex-conjugate symmetric
// 1 channel input, 2 channel output
static void rfft512_cmadd_1X2_ref(const float src[512], const float coef0[512], const float coef1[512], float dst0[512], float dst1[512]) {

    // NOTE: x[n/2].re is packed into x[0].im
    dst0[0] += src[0] * coef0[0];   // first bin is real
    dst0[1] += src[1] * coef0[1];   // last bin is real

    dst1[0] += src[0] * coef1[0];   // first bin is real
    dst1[1] += src[1] * coef1[1];   // last bin is real

    for (int i = 1; i < 512/2; i++) {
        dst0[2*i+0] += src[2*i+0] * coef0[2*i+0] - src[2*i+1] * coef0[2*i+1];   // re
        dst0[2*i+1] += src[2*i+0] * coef0[2*i+1] + src[2*i+1] * coef0[2*i+0];   // im

        dst1[2*i+0] += src[2*i+0] * coef1[2*i+0] - src[2*i+1] * coef1[2*i+1];   // re
        dst1[2*i+1] += src[2*i+0] * coef1[2*i+1] + src[2*i+1] * coef1[2*i+0];   // im
    }
}

#ifdef FOA_INPUT_FUMA   // input is FuMa (B-format) channel order and normalization

// convert to deinterleaved float (B-format)
static void convertInput_ref(int16_t* src, float *dst[4], float gain, int numFrames) {

    const float scale = gain * (1/32768.0f);

    for (int i = 0; i < numFrames; i++) {
        dst[0][i] = (float)src[4*i+0] * scale;  // W
        dst[1][i] = (float)src[4*i+1] * scale;  // X
        dst[2][i] = (float)src[4*i+2] * scale;  // Y
        dst[3][i] = (float)src[4*i+3] * scale;  // Z
    }
}

#else   // input is ambiX (ACN/SN3D) channel order and normalization

// convert to deinterleaved float (B-format)
static void convertInput_ref(int16_t* src, float *dst[4], float gain, int numFrames) {

    const float scaleW = gain * (1/32768.0f) * SQRT1_2; // -3dB
    const float scale = gain * (1/32768.0f);

    for (int i = 0; i < numFrames; i++) {
        dst[0][i] = (float)src[4*i+0] * scaleW; // W
        dst[2][i] = (float)src[4*i+1] * scale;  // Y
        dst[3][i] = (float)src[4*i+2] * scale;  // Z
        dst[1][i] = (float)src[4*i+3] * scale;  // X
    }
}

#endif

// in-place rotation of the soundfield
// crossfade between old and new rotation, to prevent artifacts
static void rotate_3x3_ref(float* buf[4], const float m0[3][3], const float m1[3][3], const float* win, int numFrames) {

    const float md[3][3] = { 
        { m0[0][0] - m1[0][0], m0[0][1] - m1[0][1], m0[0][2] - m1[0][2] },
        { m0[1][0] - m1[1][0], m0[1][1] - m1[1][1], m0[1][2] - m1[1][2] },
        { m0[2][0] - m1[2][0], m0[2][1] - m1[2][1], m0[2][2] - m1[2][2] },
    };

    for (int i = 0; i < numFrames; i++) {

        float frac = win[i];

        // interpolate the matrix
        float m00 = m1[0][0] + frac * md[0][0];
        float m10 = m1[1][0] + frac * md[1][0];
        float m20 = m1[2][0] + frac * md[2][0];

        float m01 = m1[0][1] + frac * md[0][1];
        float m11 = m1[1][1] + frac * md[1][1];
        float m21 = m1[2][1] + frac * md[2][1];

        float m02 = m1[0][2] + frac * md[0][2];
        float m12 = m1[1][2] + frac * md[1][2];
        float m22 = m1[2][2] + frac * md[2][2];

        // matrix multiply
        float x = m00 * buf[1][i] + m01 * buf[2][i] + m02 * buf[3][i];
        float y = m10 * buf[1][i] + m11 * buf[2][i] + m12 * buf[3][i];
        float z = m20 * buf[1][i] + m21 * buf[2][i] + m22 * buf[3][i];

        buf[1][i] = x;
        buf[2][i] = y;
        buf[3][i] = z;
    }
}

#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

//
// Runtime CPU dispatch
//

#include "CPUDetect.h"

void rfft512_AVX2(float buf[512]);
void rifft512_AVX2(float buf[512]);
void rfft512_cmadd_1X2_AVX2(const float src[512], const float coef0[512], const float coef1[512], float dst0[512], float dst1[512]);
void convertInput_AVX2(int16_t* src, float *dst[4], float gain, int numFrames);
void rotate_3x3_AVX2(float* buf[4], const float m0[3][3], const float m1[3][3], const float* win, int numFrames);

static void rfft512(float buf[512]) {
    static auto f = cpuSupportsAVX2() ? rfft512_AVX2 : rfft512_ref;
    (*f)(buf);  // dispatch
}

static void rifft512(float buf[512]) {
    static auto f = cpuSupportsAVX2() ? rifft512_AVX2 : rifft512_ref;
    (*f)(buf);  // dispatch
}

static void rfft512_cmadd_1X2(const float src[512], const float coef0[512], const float coef1[512], float dst0[512], float dst1[512]) {
    static auto f = cpuSupportsAVX2() ? rfft512_cmadd_1X2_AVX2 : rfft512_cmadd_1X2_ref;
    (*f)(src, coef0, coef1, dst0, dst1);    // dispatch
}

static void convertInput(int16_t* src, float *dst[4], float gain, int numFrames) {
    static auto f = cpuSupportsAVX2() ? convertInput_AVX2 : convertInput_ref;
    (*f)(src, dst, gain, numFrames);  // dispatch
}

static void rotate_3x3(float* buf[4], const float m0[3][3], const float m1[3][3], const float* win, int numFrames) {
    static auto f = cpuSupportsAVX2() ? rotate_3x3_AVX2 : rotate_3x3_ref;
    (*f)(buf, m0, m1, win, numFrames);  // dispatch
}

#else   // portable reference code

static auto& rfft512 = rfft512_ref;
static auto& rifft512 = rifft512_ref;
static auto& rfft512_cmadd_1X2 = rfft512_cmadd_1X2_ref;
static auto& convertInput = convertInput_ref;
static auto& rotate_3x3 = rotate_3x3_ref;

#endif

//
// Equal-gain crossfade
//
// Cos(x)^2 window minimizes the modulation sidebands when a pure tone is panned.
//
ALIGN32 static const float crossfadeTable[FOA_BLOCK] = {
    0.9999571638f, 0.9998286625f, 0.9996145181f, 0.9993147674f, 0.9989294616f, 0.9984586669f, 0.9979024638f, 0.9972609477f, 
    0.9965342285f, 0.9957224307f, 0.9948256934f, 0.9938441703f, 0.9927780295f, 0.9916274538f, 0.9903926402f, 0.9890738004f, 
    0.9876711603f, 0.9861849602f, 0.9846154549f, 0.9829629131f, 0.9812276182f, 0.9794098674f, 0.9775099722f, 0.9755282581f, 
    0.9734650647f, 0.9713207455f, 0.9690956680f, 0.9667902132f, 0.9644047764f, 0.9619397663f, 0.9593956051f, 0.9567727288f, 
    0.9540715869f, 0.9512926422f, 0.9484363708f, 0.9455032621f, 0.9424938187f, 0.9394085563f, 0.9362480035f, 0.9330127019f, 
    0.9297032058f, 0.9263200822f, 0.9228639109f, 0.9193352840f, 0.9157348062f, 0.9120630943f, 0.9083207776f, 0.9045084972f, 
    0.9006269063f, 0.8966766701f, 0.8926584654f, 0.8885729807f, 0.8844209160f, 0.8802029828f, 0.8759199037f, 0.8715724127f, 
    0.8671612547f, 0.8626871855f, 0.8581509717f, 0.8535533906f, 0.8488952299f, 0.8441772878f, 0.8394003728f, 0.8345653032f, 
    0.8296729076f, 0.8247240242f, 0.8197195010f, 0.8146601955f, 0.8095469747f, 0.8043807145f, 0.7991623003f, 0.7938926261f, 
    0.7885725950f, 0.7832031185f, 0.7777851165f, 0.7723195175f, 0.7668072580f, 0.7612492824f, 0.7556465430f, 0.7500000000f, 
    0.7443106207f, 0.7385793801f, 0.7328072602f, 0.7269952499f, 0.7211443451f, 0.7152555484f, 0.7093298688f, 0.7033683215f, 
    0.6973719282f, 0.6913417162f, 0.6852787188f, 0.6791839748f, 0.6730585285f, 0.6669034296f, 0.6607197327f, 0.6545084972f, 
    0.6482707875f, 0.6420076724f, 0.6357202249f, 0.6294095226f, 0.6230766465f, 0.6167226819f, 0.6103487175f, 0.6039558454f, 
    0.5975451610f, 0.5911177627f, 0.5846747519f, 0.5782172325f, 0.5717463110f, 0.5652630961f, 0.5587686987f, 0.5522642316f, 
    0.5457508093f, 0.5392295479f, 0.5327015646f, 0.5261679781f, 0.5196299079f, 0.5130884742f, 0.5065447978f, 0.5000000000f, 
    0.4934552022f, 0.4869115258f, 0.4803700921f, 0.4738320219f, 0.4672984354f, 0.4607704521f, 0.4542491907f, 0.4477357684f, 
    0.4412313013f, 0.4347369039f, 0.4282536890f, 0.4217827675f, 0.4153252481f, 0.4088822373f, 0.4024548390f, 0.3960441546f, 
    0.3896512825f, 0.3832773181f, 0.3769233535f, 0.3705904774f, 0.3642797751f, 0.3579923276f, 0.3517292125f, 0.3454915028f, 
    0.3392802673f, 0.3330965704f, 0.3269414715f, 0.3208160252f, 0.3147212812f, 0.3086582838f, 0.3026280718f, 0.2966316785f, 
    0.2906701312f, 0.2847444516f, 0.2788556549f, 0.2730047501f, 0.2671927398f, 0.2614206199f, 0.2556893793f, 0.2500000000f, 
    0.2443534570f, 0.2387507176f, 0.2331927420f, 0.2276804825f, 0.2222148835f, 0.2167968815f, 0.2114274050f, 0.2061073739f, 
    0.2008376997f, 0.1956192855f, 0.1904530253f, 0.1853398045f, 0.1802804990f, 0.1752759758f, 0.1703270924f, 0.1654346968f, 
    0.1605996272f, 0.1558227122f, 0.1511047701f, 0.1464466094f, 0.1418490283f, 0.1373128145f, 0.1328387453f, 0.1284275873f, 
    0.1240800963f, 0.1197970172f, 0.1155790840f, 0.1114270193f, 0.1073415346f, 0.1033233299f, 0.0993730937f, 0.0954915028f, 
    0.0916792224f, 0.0879369057f, 0.0842651938f, 0.0806647160f, 0.0771360891f, 0.0736799178f, 0.0702967942f, 0.0669872981f, 
    0.0637519965f, 0.0605914437f, 0.0575061813f, 0.0544967379f, 0.0515636292f, 0.0487073578f, 0.0459284131f, 0.0432272712f, 
    0.0406043949f, 0.0380602337f, 0.0355952236f, 0.0332097868f, 0.0309043320f, 0.0286792545f, 0.0265349353f, 0.0244717419f, 
    0.0224900278f, 0.0205901326f, 0.0187723818f, 0.0170370869f, 0.0153845451f, 0.0138150398f, 0.0123288397f, 0.0109261996f, 
    0.0096073598f, 0.0083725462f, 0.0072219705f, 0.0061558297f, 0.0051743066f, 0.0042775693f, 0.0034657715f, 0.0027390523f, 
    0.0020975362f, 0.0015413331f, 0.0010705384f, 0.0006852326f, 0.0003854819f, 0.0001713375f, 0.0000428362f, 0.0000000000f, 
};

// convert quaternion to a column-major 3x3 rotation matrix
static void quatToMatrix_3x3(float w, float x, float y, float z, float m[3][3]) {

	float xx = x * (x + x);
	float xy = x * (y + y);
	float xz = x * (z + z);

	float yy = y * (y + y);
	float yz = y * (z + z);
	float zz = z * (z + z);

	float wx = w * (x + x);
	float wy = w * (y + y);
	float wz = w * (z + z);

	m[0][0] = 1.0f - (yy + zz);
	m[0][1] = xy - wz;
	m[0][2] = xz + wy;

	m[1][0] = xy + wz;
	m[1][1] = 1.0f - (xx + zz);
	m[1][2] = yz - wx;

	m[2][0] = xz - wy;
	m[2][1] = yz + wx;
	m[2][2] = 1.0f - (xx + yy);
}

// Ambisonic to binaural render
void AudioFOA::render(int16_t* input, float* output, int index, float qw, float qx, float qy, float qz, float gain, int numFrames) {

    assert(index >= 0);
    assert(index < FOA_TABLES);
    assert(numFrames == FOA_BLOCK);

    ALIGN32 float fftBuffer[FOA_NFFT];          // in-place FFT buffer
    ALIGN32 float accBuffer[2][FOA_NFFT] = {};  // binaural accumulation buffers
    ALIGN32 float inBuffer[4][FOA_BLOCK];       // deinterleaved input buffers

    float* in[4] = { inBuffer[0], inBuffer[1], inBuffer[2], inBuffer[3] };
    float rotation[3][3];

    // convert input to deinterleaved float
    convertInput(input, in, FOA_GAIN * gain, FOA_BLOCK);

    // convert quaternion to 3x3 rotation
    quatToMatrix_3x3(qw, qx, qy, qz, rotation);

    // rotate the soundfield
    rotate_3x3(in, _rotationState, rotation, crossfadeTable, FOA_BLOCK);

    // rotation history update
    memcpy(_rotationState, rotation, sizeof(_rotationState));

    //
    // Accumulate the contribution of each spherical harmonic into binaural output buffers.
    // Performed in the frequency domain, using overlap-save FFT.
    //
    for (int n = 0; n < 4; n++) {

        // fill with overlapped input
        memcpy(fftBuffer, _fftState[n], FOA_OVERLAP * sizeof(float));       // old input
        memcpy(&fftBuffer[FOA_OVERLAP], in[n], FOA_BLOCK * sizeof(float));  // new input

        // input history update
        memcpy(_fftState[n], &fftBuffer[FOA_BLOCK], FOA_OVERLAP * sizeof(float));

        // forward transform
        rfft512(fftBuffer);

        // multiply-accumulate with filter kernels
        rfft512_cmadd_1X2(fftBuffer, foa_table_table[index][n][0], foa_table_table[index][n][1], accBuffer[0], accBuffer[1]);
    }

    // inverse transform
    rifft512(accBuffer[0]);
    rifft512(accBuffer[1]);

    //
    // Mix into the interleaved output buffer.
    // The first FOA_OVERLAP samples are discarded, due to overlap-save FFT.
    //
    for (int i = 0; i < FOA_BLOCK; i++) {
        output[2*i+0] += accBuffer[0][i + FOA_OVERLAP];
        output[2*i+1] += accBuffer[1][i + FOA_OVERLAP];
    }
}
