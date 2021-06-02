//
//  AudioHRTF.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 1/17/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioHRTF.h"

#include <math.h>
#include <string.h>
#include <assert.h>

#include "AudioHRTFData.h"

#if defined(_MSC_VER)
#define ALIGN32 __declspec(align(32))
#elif defined(__GNUC__)
#define ALIGN32 __attribute__((aligned(32)))
#else
#define ALIGN32
#endif

//
// Equal-gain crossfade
//
// Cos(x)^2 window minimizes the modulation sidebands when a pure tone is panned.
// Transients in the time-varying Thiran allpass filter are eliminated by the initial delay.
// Valimaki, Laakso. "Elimination of Transients in Time-Varying Allpass Fractional Delay Filters"
//
ALIGN32 static const float crossfadeTable[HRTF_BLOCK] = {
    1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 1.0000000000f, 
    0.9999545513f, 0.9998182135f, 0.9995910114f, 0.9992729863f, 0.9988641959f, 0.9983647147f, 0.9977746334f, 0.9970940592f, 
    0.9963231160f, 0.9954619438f, 0.9945106993f, 0.9934695553f, 0.9923387012f, 0.9911183425f, 0.9898087010f, 0.9884100149f, 
    0.9869225384f, 0.9853465419f, 0.9836823120f, 0.9819301512f, 0.9800903780f, 0.9781633270f, 0.9761493483f, 0.9740488082f, 
    0.9718620885f, 0.9695895868f, 0.9672317161f, 0.9647889052f, 0.9622615981f, 0.9596502542f, 0.9569553484f, 0.9541773705f, 
    0.9513168255f, 0.9483742335f, 0.9453501294f, 0.9422450630f, 0.9390595988f, 0.9357943158f, 0.9324498078f, 0.9290266826f, 
    0.9255255626f, 0.9219470843f, 0.9182918983f, 0.9145606690f, 0.9107540747f, 0.9068728075f, 0.9029175730f, 0.8988890902f, 
    0.8947880914f, 0.8906153223f, 0.8863715413f, 0.8820575200f, 0.8776740426f, 0.8732219061f, 0.8687019198f, 0.8641149055f, 
    0.8594616970f, 0.8547431402f, 0.8499600930f, 0.8451134248f, 0.8402040169f, 0.8352327617f, 0.8302005629f, 0.8251083354f, 
    0.8199570049f, 0.8147475079f, 0.8094807915f, 0.8041578130f, 0.7987795403f, 0.7933469510f, 0.7878610328f, 0.7823227830f, 
    0.7767332084f, 0.7710933251f, 0.7654041585f, 0.7596667428f, 0.7538821211f, 0.7480513449f, 0.7421754743f, 0.7362555775f, 
    0.7302927306f, 0.7242880178f, 0.7182425305f, 0.7121573680f, 0.7060336363f, 0.6998724488f, 0.6936749255f, 0.6874421931f, 
    0.6811753847f, 0.6748756396f, 0.6685441031f, 0.6621819261f, 0.6557902652f, 0.6493702826f, 0.6429231452f, 0.6364500251f, 
    0.6299520991f, 0.6234305485f, 0.6168865589f, 0.6103213199f, 0.6037360251f, 0.5971318716f, 0.5905100601f, 0.5838717943f, 
    0.5772182810f, 0.5705507299f, 0.5638703530f, 0.5571783649f, 0.5504759820f, 0.5437644228f, 0.5370449075f, 0.5303186576f, 
    0.5235868960f, 0.5168508463f, 0.5101117333f, 0.5033707820f, 0.4966292180f, 0.4898882667f, 0.4831491537f, 0.4764131040f, 
    0.4696813424f, 0.4629550925f, 0.4562355772f, 0.4495240180f, 0.4428216351f, 0.4361296470f, 0.4294492701f, 0.4227817190f, 
    0.4161282057f, 0.4094899399f, 0.4028681284f, 0.3962639749f, 0.3896786801f, 0.3831134411f, 0.3765694515f, 0.3700479009f, 
    0.3635499749f, 0.3570768548f, 0.3506297174f, 0.3442097348f, 0.3378180739f, 0.3314558969f, 0.3251243604f, 0.3188246153f, 
    0.3125578069f, 0.3063250745f, 0.3001275512f, 0.2939663637f, 0.2878426320f, 0.2817574695f, 0.2757119822f, 0.2697072694f, 
    0.2637444225f, 0.2578245257f, 0.2519486551f, 0.2461178789f, 0.2403332572f, 0.2345958415f, 0.2289066749f, 0.2232667916f, 
    0.2176772170f, 0.2121389672f, 0.2066530490f, 0.2012204597f, 0.1958421870f, 0.1905192085f, 0.1852524921f, 0.1800429951f, 
    0.1748916646f, 0.1697994371f, 0.1647672383f, 0.1597959831f, 0.1548865752f, 0.1500399070f, 0.1452568598f, 0.1405383030f, 
    0.1358850945f, 0.1312980802f, 0.1267780939f, 0.1223259574f, 0.1179424800f, 0.1136284587f, 0.1093846777f, 0.1052119086f, 
    0.1011109098f, 0.0970824270f, 0.0931271925f, 0.0892459253f, 0.0854393310f, 0.0817081017f, 0.0780529157f, 0.0744744374f, 
    0.0709733174f, 0.0675501922f, 0.0642056842f, 0.0609404012f, 0.0577549370f, 0.0546498706f, 0.0516257665f, 0.0486831745f, 
    0.0458226295f, 0.0430446516f, 0.0403497458f, 0.0377384019f, 0.0352110948f, 0.0327682839f, 0.0304104132f, 0.0281379115f, 
    0.0259511918f, 0.0238506517f, 0.0218366730f, 0.0199096220f, 0.0180698488f, 0.0163176880f, 0.0146534581f, 0.0130774616f, 
    0.0115899851f, 0.0101912990f, 0.0088816575f, 0.0076612988f, 0.0065304447f, 0.0054893007f, 0.0045380562f, 0.0036768840f, 
    0.0029059408f, 0.0022253666f, 0.0016352853f, 0.0011358041f, 0.0007270137f, 0.0004089886f, 0.0001817865f, 0.0000454487f, 
};

//
// Fast approximation of the azimuth parallax correction,
// for azimuth = [-pi,pi] and distance = [0.125,2].
//
// Correction becomes 0 at distance = 2.
// Correction is continuous and smooth.
//
static const int NAZIMUTH = 8;
static const float azimuthTable[NAZIMUTH][3] = {
    {  0.018719007f,  0.097263971f, 0.080748954f },     // [-4pi/4,-3pi/4]
    {  0.066995833f,  0.319754290f, 0.336963269f },     // [-3pi/4,-2pi/4]
    { -0.066989851f, -0.101178847f, 0.006359474f },     // [-2pi/4,-1pi/4]
    { -0.018727343f, -0.020357568f, 0.040065626f },     // [-1pi/4,-0pi/4]
    { -0.005519051f, -0.018744412f, 0.040065629f },     // [ 0pi/4, 1pi/4]
    { -0.001201296f, -0.025103593f, 0.042396711f },     // [ 1pi/4, 2pi/4]
    {  0.001198959f, -0.032642381f, 0.048316220f },     // [ 2pi/4, 3pi/4]
    {  0.005519640f, -0.053424870f, 0.073296888f },     // [ 3pi/4, 4pi/4]
};

//
// Model the normalized near-field Distance Variation Filter.
//
// This version is parameterized by the DC gain correction, instead of directly by azimuth and distance.
// A first-order shelving filter is used to minimize the disturbance in ITD.
//
// Loosely based on data from S. Spagnol, "Distance rendering and perception of nearby virtual sound sources
// with a near-field filter model," Applied Acoustics (2017)
//
static const int NNEARFIELD = 9;
static const float nearFieldTable[NNEARFIELD][3] = {    // { b0, b1, a1 }
    { 0.008410604f, -0.000262748f, -0.991852144f },     // gain = 1/256
    { 0.016758914f, -0.000529590f, -0.983770676f },     // gain = 1/128
    { 0.033270607f, -0.001075350f, -0.967804743f },     // gain = 1/64
    { 0.065567740f, -0.002213762f, -0.936646021f },     // gain = 1/32
    { 0.127361554f, -0.004667324f, -0.877305769f },     // gain = 1/16
    { 0.240536414f, -0.010201827f, -0.769665412f },     // gain = 1/8
    { 0.430858205f, -0.023243052f, -0.592384847f },     // gain = 1/4
    { 0.703238106f, -0.054157913f, -0.350919807f },     // gain = 1/2
    { 1.000000000f, -0.123144711f, -0.123144711f },     // gain = 1/1
};

//
// Parametric lowpass filter to model sound propogation or occlusion.
//
// lpf = 0.0 -> -3dB @ 50kHz
// lpf = 0.5 -> -3dB @ 1kHz
// lpf = 1.0 -> -3dB @ 20Hz
//
static const int NLOWPASS = 32;
static const float lowpassTable[NLOWPASS+1][5] = {  // { b0, b1, b2, a1, a2 }
    { 0.9996582613f, 1.3644521648f,  0.4299107175f,  1.3643981990f, 0.4296229446f },
    { 0.9990601568f, 1.2627213717f,  0.3631477252f,  1.2625024258f, 0.3624268280f },
    { 0.9974547575f, 1.1378303854f,  0.2891398515f,  1.1369629374f, 0.2874620569f },
    { 0.9932384344f, 0.9872078424f,  0.2089943789f,  0.9839050501f, 0.2055356056f },
    { 0.9825457933f, 0.8153687744f,  0.1271135720f,  0.8036320348f, 0.1213961050f },
    { 0.9572356804f, 0.6404312275f,  0.0534129844f,  0.6033230637f, 0.0477568288f },
    { 0.9052878744f, 0.4902779401f,  0.0035032262f,  0.3924772681f, 0.0065917726f },
    { 0.8204774205f, 0.3736089028f, -0.0129974730f,  0.1678426876f, 0.0132461627f },
    { 0.7032096959f, 0.2836328681f, -0.0170877258f, -0.0810811878f, 0.0508360260f },
    { 0.5685067272f, 0.2128349296f, -0.0148937235f, -0.3461942779f, 0.1126422113f },
    { 0.4355093111f, 0.1558974062f, -0.0110025095f, -0.6105302595f, 0.1909344673f },
    { 0.3186188589f, 0.1108581568f, -0.0074653192f, -0.8569688248f, 0.2789805212f },
    { 0.2244962739f, 0.0766060095f, -0.0048289293f, -1.0745081373f, 0.3707814914f },
    { 0.1535044640f, 0.0516447640f, -0.0030384640f, -1.2590370066f, 0.4611477706f },
    { 0.1025113288f, 0.0341204303f, -0.0018818088f, -1.4113207964f, 0.5460707468f },
    { 0.0672016063f, 0.0221823522f, -0.0011552756f, -1.5347007285f, 0.6229294113f },
    { 0.0434202931f, 0.0142393067f, -0.0007060306f, -1.6334567973f, 0.6904103664f },
    { 0.0277383489f, 0.0090500025f, -0.0004305987f, -1.7118804671f, 0.7482382198f },
    { 0.0175636227f, 0.0057072537f, -0.0002624537f, -1.7738404438f, 0.7968488665f },
    { 0.0110441068f, 0.0035773504f, -0.0001599927f, -1.8226329785f, 0.8370944430f },
    { 0.0069069312f, 0.0022316608f, -0.0000975848f, -1.8609764152f, 0.8700174224f },
    { 0.0043012064f, 0.0013870046f, -0.0000595614f, -1.8910688315f, 0.8966974811f },
    { 0.0026696068f, 0.0008595333f, -0.0000363798f, -1.9146662133f, 0.9181589737f },
    { 0.0016526098f, 0.0005314445f, -0.0000222355f, -1.9331608518f, 0.9353226705f },
    { 0.0010209520f, 0.0003280036f, -0.0000135987f, -1.9476515008f, 0.9489868578f },
    { 0.0006297162f, 0.0002021591f, -0.0000083208f, -1.9590027292f, 0.9598262837f },
    { 0.0003879180f, 0.0001244611f, -0.0000050936f, -1.9678935939f, 0.9684008793f },
    { 0.0002387308f, 0.0000765601f, -0.0000031192f, -1.9748568416f, 0.9751690132f },
    { 0.0001468057f, 0.0000470631f, -0.0000019106f, -1.9803101382f, 0.9805020963f },
    { 0.0000902227f, 0.0000289155f, -0.0000011706f, -1.9845807858f, 0.9846987534f },
    { 0.0000554223f, 0.0000177584f, -0.0000007174f, -1.9879252038f, 0.9879976671f },
    { 0.0000340324f, 0.0000109027f, -0.0000004397f, -1.9905442465f, 0.9905887419f },
    { 0.0000208917f, 0.0000066920f, -0.0000002695f, -1.9925952275f, 0.9926225417f },
};

//
// on x86 architecture, assume that SSE2 is present
//
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <emmintrin.h>

// 1 channel input, 4 channel output
static void FIR_1x4_SSE(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

    float* coef0 = coef[0] + HRTF_TAPS - 1;     // process backwards
    float* coef1 = coef[1] + HRTF_TAPS - 1;
    float* coef2 = coef[2] + HRTF_TAPS - 1;
    float* coef3 = coef[3] + HRTF_TAPS - 1;

    assert(numFrames % 4 == 0);

    for (int i = 0; i < numFrames; i += 4) {

        __m128 acc0 = _mm_setzero_ps();
        __m128 acc1 = _mm_setzero_ps();
        __m128 acc2 = _mm_setzero_ps();
        __m128 acc3 = _mm_setzero_ps();

        float* ps = &src[i - HRTF_TAPS + 1];    // process forwards

        static_assert(HRTF_TAPS % 4 == 0, "HRTF_TAPS must be a multiple of 4");

        for (int k = 0; k < HRTF_TAPS; k += 4) {

            __m128 x0 = _mm_loadu_ps(&ps[k+0]);
            acc0 = _mm_add_ps(acc0, _mm_mul_ps(_mm_load1_ps(&coef0[-k-0]), x0));
            acc1 = _mm_add_ps(acc1, _mm_mul_ps(_mm_load1_ps(&coef1[-k-0]), x0));
            acc2 = _mm_add_ps(acc2, _mm_mul_ps(_mm_load1_ps(&coef2[-k-0]), x0));
            acc3 = _mm_add_ps(acc3, _mm_mul_ps(_mm_load1_ps(&coef3[-k-0]), x0));

            __m128 x1 = _mm_loadu_ps(&ps[k+1]);
            acc0 = _mm_add_ps(acc0, _mm_mul_ps(_mm_load1_ps(&coef0[-k-1]), x1));
            acc1 = _mm_add_ps(acc1, _mm_mul_ps(_mm_load1_ps(&coef1[-k-1]), x1));
            acc2 = _mm_add_ps(acc2, _mm_mul_ps(_mm_load1_ps(&coef2[-k-1]), x1));
            acc3 = _mm_add_ps(acc3, _mm_mul_ps(_mm_load1_ps(&coef3[-k-1]), x1));

            __m128 x2 = _mm_loadu_ps(&ps[k+2]);
            acc0 = _mm_add_ps(acc0, _mm_mul_ps(_mm_load1_ps(&coef0[-k-2]), x2));
            acc1 = _mm_add_ps(acc1, _mm_mul_ps(_mm_load1_ps(&coef1[-k-2]), x2));
            acc2 = _mm_add_ps(acc2, _mm_mul_ps(_mm_load1_ps(&coef2[-k-2]), x2));
            acc3 = _mm_add_ps(acc3, _mm_mul_ps(_mm_load1_ps(&coef3[-k-2]), x2));

            __m128 x3 = _mm_loadu_ps(&ps[k+3]);
            acc0 = _mm_add_ps(acc0, _mm_mul_ps(_mm_load1_ps(&coef0[-k-3]), x3));
            acc1 = _mm_add_ps(acc1, _mm_mul_ps(_mm_load1_ps(&coef1[-k-3]), x3));
            acc2 = _mm_add_ps(acc2, _mm_mul_ps(_mm_load1_ps(&coef2[-k-3]), x3));
            acc3 = _mm_add_ps(acc3, _mm_mul_ps(_mm_load1_ps(&coef3[-k-3]), x3));
        }

        _mm_storeu_ps(&dst0[i], acc0);
        _mm_storeu_ps(&dst1[i], acc1);
        _mm_storeu_ps(&dst2[i], acc2);
        _mm_storeu_ps(&dst3[i], acc3);
    }
}

// 4 channel planar to interleaved
static void interleave_4x4_SSE(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames) {

    assert(numFrames % 4 == 0);

    for (int i = 0; i < numFrames; i += 4) {

        __m128 x0 = _mm_loadu_ps(&src0[i]);
        __m128 x1 = _mm_loadu_ps(&src1[i]);
        __m128 x2 = _mm_loadu_ps(&src2[i]);
        __m128 x3 = _mm_loadu_ps(&src3[i]);

        // interleave (4x4 matrix transpose)
        __m128 t0 = _mm_unpacklo_ps(x0, x1);
        __m128 t2 = _mm_unpacklo_ps(x2, x3);
        __m128 t1 = _mm_unpackhi_ps(x0, x1);
        __m128 t3 = _mm_unpackhi_ps(x2, x3);

        x0 = _mm_movelh_ps(t0, t2);
        x1 = _mm_movehl_ps(t2, t0);
        x2 = _mm_movelh_ps(t1, t3);
        x3 = _mm_movehl_ps(t3, t1);

        _mm_storeu_ps(&dst[4*i+0], x0);
        _mm_storeu_ps(&dst[4*i+4], x1);
        _mm_storeu_ps(&dst[4*i+8], x2);
        _mm_storeu_ps(&dst[4*i+12], x3);
    }
}

// process 2 cascaded biquads on 4 channels (interleaved)
// biquads computed in parallel, by adding one sample of delay
static void biquad2_4x4_SSE(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames) {

    // enable flush-to-zero mode to prevent denormals
    unsigned int ftz = _MM_GET_FLUSH_ZERO_MODE();
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

    // restore state
    __m128 y00 = _mm_loadu_ps(&state[0][0]);
    __m128 w10 = _mm_loadu_ps(&state[1][0]);
    __m128 w20 = _mm_loadu_ps(&state[2][0]);

    __m128 y01;
    __m128 w11 = _mm_loadu_ps(&state[1][4]);
    __m128 w21 = _mm_loadu_ps(&state[2][4]);

    // first biquad coefs
    __m128 b00 = _mm_loadu_ps(&coef[0][0]);
    __m128 b10 = _mm_loadu_ps(&coef[1][0]);
    __m128 b20 = _mm_loadu_ps(&coef[2][0]);
    __m128 a10 = _mm_loadu_ps(&coef[3][0]);
    __m128 a20 = _mm_loadu_ps(&coef[4][0]);

    // second biquad coefs
    __m128 b01 = _mm_loadu_ps(&coef[0][4]);
    __m128 b11 = _mm_loadu_ps(&coef[1][4]);
    __m128 b21 = _mm_loadu_ps(&coef[2][4]);
    __m128 a11 = _mm_loadu_ps(&coef[3][4]);
    __m128 a21 = _mm_loadu_ps(&coef[4][4]);

    for (int i = 0; i < numFrames; i++) {

        __m128 x00 = _mm_loadu_ps(&src[4*i]);
        __m128 x01 = y00;   // first biquad output

        // transposed Direct Form II
        y00 = _mm_add_ps(w10, _mm_mul_ps(x00, b00));
        y01 = _mm_add_ps(w11, _mm_mul_ps(x01, b01));

        w10 = _mm_add_ps(w20, _mm_mul_ps(x00, b10));
        w11 = _mm_add_ps(w21, _mm_mul_ps(x01, b11));

        w20 = _mm_mul_ps(x00, b20);
        w21 = _mm_mul_ps(x01, b21);

        w10 = _mm_sub_ps(w10, _mm_mul_ps(y00, a10));
        w11 = _mm_sub_ps(w11, _mm_mul_ps(y01, a11));

        w20 = _mm_sub_ps(w20, _mm_mul_ps(y00, a20));
        w21 = _mm_sub_ps(w21, _mm_mul_ps(y01, a21));

        _mm_storeu_ps(&dst[4*i], y01);  // second biquad output
    }

    // save state
    _mm_storeu_ps(&state[0][0], y00);
    _mm_storeu_ps(&state[1][0], w10);
    _mm_storeu_ps(&state[2][0], w20);

    _mm_storeu_ps(&state[1][4], w11);
    _mm_storeu_ps(&state[2][4], w21);

    _MM_SET_FLUSH_ZERO_MODE(ftz);
}

// crossfade 4 inputs into 2 outputs with accumulation (interleaved)
static void crossfade_4x2_SSE(float* src, float* dst, const float* win, int numFrames) {

    assert(numFrames % 4 == 0);

    for (int i = 0; i < numFrames; i += 4) {

        __m128 f0 = _mm_loadu_ps(&win[i]);

        __m128 x0 = _mm_loadu_ps(&src[4*i+0]);
        __m128 x1 = _mm_loadu_ps(&src[4*i+4]);
        __m128 x2 = _mm_loadu_ps(&src[4*i+8]);
        __m128 x3 = _mm_loadu_ps(&src[4*i+12]);

        __m128 y0 = _mm_loadu_ps(&dst[2*i+0]);
        __m128 y1 = _mm_loadu_ps(&dst[2*i+4]);

        // deinterleave (4x4 matrix transpose)
        __m128 t0 = _mm_unpacklo_ps(x0, x1);
        __m128 t2 = _mm_unpacklo_ps(x2, x3);
        __m128 t1 = _mm_unpackhi_ps(x0, x1);
        __m128 t3 = _mm_unpackhi_ps(x2, x3);

        x0 = _mm_movelh_ps(t0, t2);
        x1 = _mm_movehl_ps(t2, t0);
        x2 = _mm_movelh_ps(t1, t3);
        x3 = _mm_movehl_ps(t3, t1);

        // crossfade
        x0 = _mm_sub_ps(x0, x2);
        x1 = _mm_sub_ps(x1, x3);
        x2 = _mm_add_ps(x2, _mm_mul_ps(f0, x0));
        x3 = _mm_add_ps(x3, _mm_mul_ps(f0, x1));

        // interleave
        x0 = _mm_unpacklo_ps(x2, x3);
        x1 = _mm_unpackhi_ps(x2, x3);

        // accumulate
        y0 = _mm_add_ps(y0, x0);
        y1 = _mm_add_ps(y1, x1);

        _mm_storeu_ps(&dst[2*i+0], y0);
        _mm_storeu_ps(&dst[2*i+4], y1);
    }
}

// linear interpolation with gain
static void interpolate_SSE(const float* src0, const float* src1, float* dst, float frac, float gain) {

    __m128 f0 = _mm_set1_ps(gain * (1.0f - frac));
    __m128 f1 = _mm_set1_ps(gain * frac);

    static_assert(HRTF_TAPS % 4 == 0, "HRTF_TAPS must be a multiple of 4");

    for (int k = 0; k < HRTF_TAPS; k += 4) {

        __m128 x0 = _mm_loadu_ps(&src0[k]);
        __m128 x1 = _mm_loadu_ps(&src1[k]);

        x0 = _mm_add_ps(_mm_mul_ps(f0, x0), _mm_mul_ps(f1, x1));

        _mm_storeu_ps(&dst[k], x0);
    }
}

//
// Runtime CPU dispatch
//

#include "CPUDetect.h"

void FIR_1x4_AVX2(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames);
void FIR_1x4_AVX512(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames);
void interleave_4x4_AVX2(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames);
void biquad2_4x4_AVX2(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames);
void crossfade_4x2_AVX2(float* src, float* dst, const float* win, int numFrames);
void interpolate_AVX2(const float* src0, const float* src1, float* dst, float frac, float gain);

static void FIR_1x4(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {
#ifndef STACK_PROTECTOR
    // Enabling -fstack-protector on gcc causes an undefined reference to FIR_1x4_AVX512 here
    static auto f = cpuSupportsAVX512() ? FIR_1x4_AVX512 : (cpuSupportsAVX2() ? FIR_1x4_AVX2 : FIR_1x4_SSE);
#else
    static auto f = cpuSupportsAVX2() ? FIR_1x4_AVX2 : FIR_1x4_SSE;
#endif
    (*f)(src, dst0, dst1, dst2, dst3, coef, numFrames); // dispatch
}

static void interleave_4x4(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames) {
    static auto f = cpuSupportsAVX2() ? interleave_4x4_AVX2 : interleave_4x4_SSE;
    (*f)(src0, src1, src2, src3, dst, numFrames); // dispatch
}

static void biquad2_4x4(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames) {
    static auto f = cpuSupportsAVX2() ? biquad2_4x4_AVX2 : biquad2_4x4_SSE;
    (*f)(src, dst, coef, state, numFrames); // dispatch
}

static void crossfade_4x2(float* src, float* dst, const float* win, int numFrames) {
    static auto f = cpuSupportsAVX2() ? crossfade_4x2_AVX2 : crossfade_4x2_SSE;
    (*f)(src, dst, win, numFrames); // dispatch
}

static void interpolate(const float* src0, const float* src1, float* dst, float frac, float gain) {
    static auto f = cpuSupportsAVX2() ? interpolate_AVX2 : interpolate_SSE;
    (*f)(src0, src1, dst, frac, gain); // dispatch
}

#else   // portable reference code

// 1 channel input, 4 channel output
static void FIR_1x4(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

    float* coef0 = coef[0] + HRTF_TAPS - 1;     // process backwards
    float* coef1 = coef[1] + HRTF_TAPS - 1;
    float* coef2 = coef[2] + HRTF_TAPS - 1;
    float* coef3 = coef[3] + HRTF_TAPS - 1;

    assert(numFrames % 4 == 0);

    for (int i = 0; i < numFrames; i += 4) {

        dst0[i+0] = 0.0f;
        dst0[i+1] = 0.0f;
        dst0[i+2] = 0.0f;
        dst0[i+3] = 0.0f;

        dst1[i+0] = 0.0f;
        dst1[i+1] = 0.0f;
        dst1[i+2] = 0.0f;
        dst1[i+3] = 0.0f;

        dst2[i+0] = 0.0f;
        dst2[i+1] = 0.0f;
        dst2[i+2] = 0.0f;
        dst2[i+3] = 0.0f;

        dst3[i+0] = 0.0f;
        dst3[i+1] = 0.0f;
        dst3[i+2] = 0.0f;
        dst3[i+3] = 0.0f;

        float* ps = &src[i - HRTF_TAPS + 1];    // process forwards

        static_assert(HRTF_TAPS % 4 == 0, "HRTF_TAPS must be a multiple of 4");

        for (int k = 0; k < HRTF_TAPS; k += 4) {

            // channel 0
            dst0[i+0] += coef0[-k-0] * ps[k+0] + coef0[-k-1] * ps[k+1] + coef0[-k-2] * ps[k+2] + coef0[-k-3] * ps[k+3];
            dst0[i+1] += coef0[-k-0] * ps[k+1] + coef0[-k-1] * ps[k+2] + coef0[-k-2] * ps[k+3] + coef0[-k-3] * ps[k+4];
            dst0[i+2] += coef0[-k-0] * ps[k+2] + coef0[-k-1] * ps[k+3] + coef0[-k-2] * ps[k+4] + coef0[-k-3] * ps[k+5];
            dst0[i+3] += coef0[-k-0] * ps[k+3] + coef0[-k-1] * ps[k+4] + coef0[-k-2] * ps[k+5] + coef0[-k-3] * ps[k+6];

            // channel 1
            dst1[i+0] += coef1[-k-0] * ps[k+0] + coef1[-k-1] * ps[k+1] + coef1[-k-2] * ps[k+2] + coef1[-k-3] * ps[k+3];
            dst1[i+1] += coef1[-k-0] * ps[k+1] + coef1[-k-1] * ps[k+2] + coef1[-k-2] * ps[k+3] + coef1[-k-3] * ps[k+4];
            dst1[i+2] += coef1[-k-0] * ps[k+2] + coef1[-k-1] * ps[k+3] + coef1[-k-2] * ps[k+4] + coef1[-k-3] * ps[k+5];
            dst1[i+3] += coef1[-k-0] * ps[k+3] + coef1[-k-1] * ps[k+4] + coef1[-k-2] * ps[k+5] + coef1[-k-3] * ps[k+6];

            // channel 2
            dst2[i+0] += coef2[-k-0] * ps[k+0] + coef2[-k-1] * ps[k+1] + coef2[-k-2] * ps[k+2] + coef2[-k-3] * ps[k+3];
            dst2[i+1] += coef2[-k-0] * ps[k+1] + coef2[-k-1] * ps[k+2] + coef2[-k-2] * ps[k+3] + coef2[-k-3] * ps[k+4];
            dst2[i+2] += coef2[-k-0] * ps[k+2] + coef2[-k-1] * ps[k+3] + coef2[-k-2] * ps[k+4] + coef2[-k-3] * ps[k+5];
            dst2[i+3] += coef2[-k-0] * ps[k+3] + coef2[-k-1] * ps[k+4] + coef2[-k-2] * ps[k+5] + coef2[-k-3] * ps[k+6];

            // channel 3
            dst3[i+0] += coef3[-k-0] * ps[k+0] + coef3[-k-1] * ps[k+1] + coef3[-k-2] * ps[k+2] + coef3[-k-3] * ps[k+3];
            dst3[i+1] += coef3[-k-0] * ps[k+1] + coef3[-k-1] * ps[k+2] + coef3[-k-2] * ps[k+3] + coef3[-k-3] * ps[k+4];
            dst3[i+2] += coef3[-k-0] * ps[k+2] + coef3[-k-1] * ps[k+3] + coef3[-k-2] * ps[k+4] + coef3[-k-3] * ps[k+5];
            dst3[i+3] += coef3[-k-0] * ps[k+3] + coef3[-k-1] * ps[k+4] + coef3[-k-2] * ps[k+5] + coef3[-k-3] * ps[k+6];
        }
    }
}

// 4 channel planar to interleaved
static void interleave_4x4(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames) {

    for (int i = 0; i < numFrames; i++) {

        dst[4*i+0] = src0[i];
        dst[4*i+1] = src1[i];
        dst[4*i+2] = src2[i];
        dst[4*i+3] = src3[i];
    }
}

// process 2 cascaded biquads on 4 channels (interleaved)
// biquads are computed in parallel, by adding one sample of delay
static void biquad2_4x4(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames) {

    // restore state
    float y00 = state[0][0];
    float w10 = state[1][0];
    float w20 = state[2][0];

    float y01 = state[0][1];
    float w11 = state[1][1];
    float w21 = state[2][1];

    float y02 = state[0][2];
    float w12 = state[1][2];
    float w22 = state[2][2];

    float y03 = state[0][3];
    float w13 = state[1][3];
    float w23 = state[2][3];

    float y04;
    float w14 = state[1][4];
    float w24 = state[2][4];

    float y05;
    float w15 = state[1][5];
    float w25 = state[2][5];

    float y06;
    float w16 = state[1][6];
    float w26 = state[2][6];

    float y07;
    float w17 = state[1][7];
    float w27 = state[2][7];

    // first biquad coefs
    float b00 = coef[0][0];
    float b10 = coef[1][0];
    float b20 = coef[2][0];
    float a10 = coef[3][0];
    float a20 = coef[4][0];

    float b01 = coef[0][1];
    float b11 = coef[1][1];
    float b21 = coef[2][1];
    float a11 = coef[3][1];
    float a21 = coef[4][1];

    float b02 = coef[0][2];
    float b12 = coef[1][2];
    float b22 = coef[2][2];
    float a12 = coef[3][2];
    float a22 = coef[4][2];

    float b03 = coef[0][3];
    float b13 = coef[1][3];
    float b23 = coef[2][3];
    float a13 = coef[3][3];
    float a23 = coef[4][3];

    // second biquad coefs
    float b04 = coef[0][4];
    float b14 = coef[1][4];
    float b24 = coef[2][4];
    float a14 = coef[3][4];
    float a24 = coef[4][4];

    float b05 = coef[0][5];
    float b15 = coef[1][5];
    float b25 = coef[2][5];
    float a15 = coef[3][5];
    float a25 = coef[4][5];

    float b06 = coef[0][6];
    float b16 = coef[1][6];
    float b26 = coef[2][6];
    float a16 = coef[3][6];
    float a26 = coef[4][6];

    float b07 = coef[0][7];
    float b17 = coef[1][7];
    float b27 = coef[2][7];
    float a17 = coef[3][7];
    float a27 = coef[4][7];

    for (int i = 0; i < numFrames; i++) {

        // first biquad input
        float x00 = src[4*i+0] + 1.0e-20f;    // prevent denormals
        float x01 = src[4*i+1] + 1.0e-20f;
        float x02 = src[4*i+2] + 1.0e-20f;
        float x03 = src[4*i+3] + 1.0e-20f;
        // second biquad input is previous output
        float x04 = y00;
        float x05 = y01;
        float x06 = y02;
        float x07 = y03;

        // transposed Direct Form II
        y00 = b00 * x00 + w10;
        w10 = b10 * x00 - a10 * y00 + w20;
        w20 = b20 * x00 - a20 * y00;

        y01 = b01 * x01 + w11;
        w11 = b11 * x01 - a11 * y01 + w21;
        w21 = b21 * x01 - a21 * y01;

        y02 = b02 * x02 + w12;
        w12 = b12 * x02 - a12 * y02 + w22;
        w22 = b22 * x02 - a22 * y02;

        y03 = b03 * x03 + w13;
        w13 = b13 * x03 - a13 * y03 + w23;
        w23 = b23 * x03 - a23 * y03;

        // transposed Direct Form II
        y04 = b04 * x04 + w14;
        w14 = b14 * x04 - a14 * y04 + w24;
        w24 = b24 * x04 - a24 * y04;

        y05 = b05 * x05 + w15;
        w15 = b15 * x05 - a15 * y05 + w25;
        w25 = b25 * x05 - a25 * y05;

        y06 = b06 * x06 + w16;
        w16 = b16 * x06 - a16 * y06 + w26;
        w26 = b26 * x06 - a26 * y06;

        y07 = b07 * x07 + w17;
        w17 = b17 * x07 - a17 * y07 + w27;
        w27 = b27 * x07 - a27 * y07;

        dst[4*i+0] = y04;   // second biquad output
        dst[4*i+1] = y05;
        dst[4*i+2] = y06;
        dst[4*i+3] = y07;
    }

    // save state
    state[0][0] = y00;
    state[1][0] = w10;
    state[2][0] = w20;

    state[0][1] = y01;
    state[1][1] = w11;
    state[2][1] = w21;

    state[0][2] = y02;
    state[1][2] = w12;
    state[2][2] = w22;

    state[0][3] = y03;
    state[1][3] = w13;
    state[2][3] = w23;

    state[1][4] = w14;
    state[2][4] = w24;

    state[1][5] = w15;
    state[2][5] = w25;

    state[1][6] = w16;
    state[2][6] = w26;

    state[1][7] = w17;
    state[2][7] = w27;
}

// crossfade 4 inputs into 2 outputs with accumulation (interleaved)
static void crossfade_4x2(float* src, float* dst, const float* win, int numFrames) {

    for (int i = 0; i < numFrames; i++) {

        float frac = win[i];

        dst[2*i+0] += src[4*i+2] + frac * (src[4*i+0] - src[4*i+2]);
        dst[2*i+1] += src[4*i+3] + frac * (src[4*i+1] - src[4*i+3]);
    }
}

// linear interpolation with gain
static void interpolate(const float* src0, const float* src1, float* dst, float frac, float gain) {

    float f0 = gain * (1.0f - frac);
    float f1 = gain * frac;

    for (int k = 0; k < HRTF_TAPS; k++) {
        dst[k] = f0 * src0[k] + f1 * src1[k];
    }
}

#endif

// apply gain crossfade with accumulation (interleaved)
static void gainfade_1x2(int16_t* src, float* dst, const float* win, float gain0, float gain1, int numFrames) {

    gain0 *= (1/32768.0f);  // int16_t to float
    gain1 *= (1/32768.0f);

    for (int i = 0; i < numFrames; i++) {

        float frac = win[i];
        float gain = gain1 + frac * (gain0 - gain1);

        float x0 = (float)src[i] * gain;

        dst[2*i+0] += x0;
        dst[2*i+1] += x0;
    }
}

// apply gain crossfade with accumulation (interleaved)
static void gainfade_2x2(int16_t* src, float* dst, const float* win, float gain0, float gain1, int numFrames) {

    gain0 *= (1/32768.0f);  // int16_t to float
    gain1 *= (1/32768.0f);

    for (int i = 0; i < numFrames; i++) {

        float frac = win[i];
        float gain = gain1 + frac * (gain0 - gain1);

        float x0 = (float)src[2*i+0] * gain;
        float x1 = (float)src[2*i+1] * gain;

        dst[2*i+0] += x0;
        dst[2*i+1] += x1;
    }
}

// design a 2nd order Thiran allpass
static void ThiranBiquad(float f, float& b0, float& b1, float& b2, float& a1, float& a2) {

    a1 = -2.0f * (f - 2.0f) / (f + 1.0f);
    a2 = ((f - 1.0f) * (f - 2.0f)) / ((f + 1.0f) * (f + 2.0f));
    b0 = a2;
    b1 = a1;
    b2 = 1.0f;
}

// split x into exponent and fraction (0.0f to 1.0f)
static void splitf(float x, int& expn, float& frac) {

    union { float f; int i; } mant, bits = { x };
    const int IEEE754_MANT_BITS = 23;
    const int IEEE754_EXPN_BIAS = 127;

    mant.i = bits.i & ((1 << IEEE754_MANT_BITS) - 1);
    mant.i |= (IEEE754_EXPN_BIAS << IEEE754_MANT_BITS);
    
    frac = mant.f - 1.0f;
    expn = (bits.i >> IEEE754_MANT_BITS) - IEEE754_EXPN_BIAS;
}

static void lowpassBiquad(float lpf, float& b0, float& b1, float& b2, float& a1, float& a2) {

    //
    // Computed from a lookup table and piecewise linear interpolation.
    // Approximation error < 0.25dB
    //
    float x = lpf * NLOWPASS;

    // split x into index and fraction
    int i = (int)x;
    float frac = x - (float)i;

    // clamp to table limits
    if (i < 0) {
        i = 0;
        frac = 0.0f;
    } 
    if (i > NLOWPASS-1) {
        i = NLOWPASS-1;
        frac = 1.0f;
    }
    assert(frac >= 0.0f);
    assert(frac <= 1.0f);
    assert(i+0 >= 0);
    assert(i+1 <= NLOWPASS);

    // piecewise linear interpolation
    b0 = lowpassTable[i+0][0] + frac * (lowpassTable[i+1][0] - lowpassTable[i+0][0]);
    b1 = lowpassTable[i+0][1] + frac * (lowpassTable[i+1][1] - lowpassTable[i+0][1]);
    b2 = lowpassTable[i+0][2] + frac * (lowpassTable[i+1][2] - lowpassTable[i+0][2]);
    a1 = lowpassTable[i+0][3] + frac * (lowpassTable[i+1][3] - lowpassTable[i+0][3]);
    a2 = lowpassTable[i+0][4] + frac * (lowpassTable[i+1][4] - lowpassTable[i+0][4]);
}

//
// Geometric correction of the azimuth, to the angle at each ear.
// D. Brungart, "Auditory parallax effects in the HRTF for nearby sources," IEEE WASPAA (1999).
//
static void nearFieldAzimuthCorrection(float azimuth, float distance, float& azimuthL, float& azimuthR) {

#ifdef HRTF_AZIMUTH_EXACT
    float dx = distance * cosf(azimuth);
    float dy = distance * sinf(azimuth);

    float dx0 = HRTF_AZIMUTH_REF * cosf(azimuth);
    float dy0 = HRTF_AZIMUTH_REF * sinf(azimuth);

    azimuthL += atan2f(dy + HRTF_HEAD_RADIUS, dx) - atan2f(dy0 + HRTF_HEAD_RADIUS, dx0);
    azimuthR += atan2f(dy - HRTF_HEAD_RADIUS, dx) - atan2f(dy0 - HRTF_HEAD_RADIUS, dx0);
#else
    // at reference distance, the azimuth parallax is correct
    float fy = (HRTF_AZIMUTH_REF - distance) / distance;

    float x0 = +azimuth;
    float x1 = -azimuth;    // compute using symmetry

    const float RAD_TO_INDEX = 1.2732394f;  // 8/(2*pi), rounded down
    int k0 = (int)(RAD_TO_INDEX * x0 + 4.0f);
    int k1 = (NAZIMUTH-1) - k0;
    assert(k0 >= 0);
    assert(k1 >= 0);
    assert(k0 < NAZIMUTH);
    assert(k1 < NAZIMUTH);

    // piecewise polynomial approximation over azimuth=[-pi,pi]
    float fx0 = (azimuthTable[k0][0] * x0 + azimuthTable[k0][1]) * x0 + azimuthTable[k0][2];
    float fx1 = (azimuthTable[k1][0] * x1 + azimuthTable[k1][1]) * x1 + azimuthTable[k1][2];

    // approximate the azimuth correction
    // NOTE: must converge to 0 when distance = HRTF_AZIMUTH_REF
    azimuthL += fx0 * fy;
    azimuthR -= fx1 * fy;
#endif
}

//
// Approximate the near-field DC gain correction at each ear.
//
static void nearFieldGainCorrection(float azimuth, float distance, float& gainL, float& gainR) {

    // normalized distance factor = [0,1] as distance = [HRTF_NEARFIELD_MAX,HRTF_HEAD_RADIUS]
    assert(distance < HRTF_NEARFIELD_MAX);
    assert(distance > HRTF_HEAD_RADIUS);
	float d = (HRTF_NEARFIELD_MAX - distance) * (1.0f / (HRTF_NEARFIELD_MAX - HRTF_HEAD_RADIUS));

    // angle of incidence at each ear
    float angleL = azimuth + PI_OVER_TWO;
    float angleR = azimuth - PI_OVER_TWO;
    if (angleL > +PI) {
        angleL -= TWO_PI;
    }
    if (angleR < -PI) {
        angleR += TWO_PI;
    }
    assert(angleL >= -PI);
    assert(angleL <= +PI);
    assert(angleR >= -PI);
    assert(angleR <= +PI);

    // normalized occlusion factor = [0,1] as angle of incidence = [0,pi]
    angleL *= angleL;
    angleR *= angleR;
    float cL = ((-0.000452339132f * angleL - 0.00173192444f) * angleL + 0.162476536f) * angleL;
    float cR = ((-0.000452339132f * angleR - 0.00173192444f) * angleR + 0.162476536f) * angleR;

    // approximate the gain correction
    // NOTE: must converge to 0 when distance = HRTF_NEARFIELD_MAX
    gainL -= d * cL;
    gainR -= d * cR;
}

//
// Approximate the normalized near-field Distance Variation Function at each ear.
// A. Kan, "Distance Variation Function for simulation of near-field virtual auditory space," IEEE ICASSP (2006)
//
static void nearFieldFilter(float gain, float& b0, float& b1, float& a1) {

    //
    // Computed from a lookup table quantized to gain = 2^(-N)
    // and reconstructed by piecewise linear interpolation.
    //

    // split gain into e and frac, such that gain = 2^(e+0) + frac * (2^(e+1) - 2^(e+0))
    int e;
    float frac;
    splitf(gain, e, frac);

    // clamp to table limits
    e += NNEARFIELD-1;
    if (e < 0) {
        e = 0;
        frac = 0.0f;
    } 
    if (e > NNEARFIELD-2) {
        e = NNEARFIELD-2;
        frac = 1.0f;
    }
    assert(frac >= 0.0f);
    assert(frac <= 1.0f);
    assert(e+0 >= 0);
    assert(e+1 < NNEARFIELD);

    // piecewise linear interpolation
    b0 = nearFieldTable[e+0][0] + frac * (nearFieldTable[e+1][0] - nearFieldTable[e+0][0]);
    b1 = nearFieldTable[e+0][1] + frac * (nearFieldTable[e+1][1] - nearFieldTable[e+0][1]);
    a1 = nearFieldTable[e+0][2] + frac * (nearFieldTable[e+1][2] - nearFieldTable[e+0][2]);
}

static void azimuthToIndex(float azimuth, int& index0, int& index1, float& frac) {

    // convert from radians to table units
    azimuth *= (HRTF_AZIMUTHS / TWO_PI);

    if (azimuth < 0.0f) {
        azimuth += HRTF_AZIMUTHS;
    }

    // table parameters
    index0 = (int)azimuth;
    index1 = index0 + 1;
    frac = azimuth - (float)index0;

    if (index0 >= HRTF_AZIMUTHS) {
        index0 -= HRTF_AZIMUTHS;
    }
    if (index1 >= HRTF_AZIMUTHS) {
        index1 -= HRTF_AZIMUTHS;
    }

    assert((index0 >= 0) && (index0 < HRTF_AZIMUTHS));
    assert((index1 >= 0) && (index1 < HRTF_AZIMUTHS));
    assert((frac >= 0.0f) && (frac < 1.0f));
}

// compute new filters for a given azimuth, distance and gain
static void setFilters(float firCoef[4][HRTF_TAPS], float bqCoef[5][8], int delay[4], 
                       int index, float azimuth, float distance, float gain, float lpf, int channel) {

    if (azimuth > PI) {
        azimuth -= TWO_PI;
    }
    assert(azimuth >= -PI);
    assert(azimuth <= +PI);

    distance = std::max(distance, HRTF_NEARFIELD_MIN);

    // compute the azimuth correction at each ear
    float azimuthL = azimuth;
    float azimuthR = azimuth;
    if (distance < HRTF_AZIMUTH_REF) {
        nearFieldAzimuthCorrection(azimuth, distance, azimuthL, azimuthR);
    }

    // compute the DC gain correction at each ear
    float gainL = 1.0f;
    float gainR = 1.0f;
    if (distance < HRTF_NEARFIELD_MAX) {
        nearFieldGainCorrection(azimuth, distance, gainL, gainR);
    }

    // parameters for table interpolation
    int azL0, azR0, az0;
    int azL1, azR1, az1;
    float fracL, fracR, frac;

    azimuthToIndex(azimuthL, azL0, azL1, fracL);
    azimuthToIndex(azimuthR, azR0, azR1, fracR);
    azimuthToIndex(azimuth, az0, az1, frac);

    // interpolate FIR
    interpolate(ir_table_table[index][azL0][0], ir_table_table[index][azL1][0], firCoef[channel+0], fracL, gain * gainL);
    interpolate(ir_table_table[index][azR0][1], ir_table_table[index][azR1][1], firCoef[channel+1], fracR, gain * gainR);

    // interpolate ITD
    float itd = (1.0f - frac) * itd_table_table[index][az0] + frac * itd_table_table[index][az1];

    // split ITD into integer and fractional delay
    int itdi = (int)fabsf(itd);
    float itdf = fabsf(itd) - (float)itdi;

    assert(itdi <= HRTF_DELAY);
    assert(itdf <= 1.0f);

    //
    // Compute a 2nd-order Thiran allpass for the fractional delay.
    // With nominal delay of 2, the active range of [2.0, 3.0] results
    // in group delay flat to 1.5KHz and fast transient settling time.
    //
    float b0, b1, b2, a1, a2;
    ThiranBiquad(2.0f + itdf, b0, b1, b2, a1, a2);

    // positive ITD means left channel is delayed
    if (itd >= 0.0f) {

        // left (contralateral) = 2 + itdi + itdf
        bqCoef[0][channel+0] = b0;
        bqCoef[1][channel+0] = b1;
        bqCoef[2][channel+0] = b2;
        bqCoef[3][channel+0] = a1;
        bqCoef[4][channel+0] = a2;
        delay[channel+0] = itdi;

        // right (ipsilateral) = 2
        bqCoef[0][channel+1] = 0.0f;
        bqCoef[1][channel+1] = 0.0f;
        bqCoef[2][channel+1] = 1.0f;
        bqCoef[3][channel+1] = 0.0f;
        bqCoef[4][channel+1] = 0.0f;
        delay[channel+1] = 0;

    } else {

        // left (ipsilateral) = 2
        bqCoef[0][channel+0] = 0.0f;
        bqCoef[1][channel+0] = 0.0f;
        bqCoef[2][channel+0] = 1.0f;
        bqCoef[3][channel+0] = 0.0f;
        bqCoef[4][channel+0] = 0.0f;
        delay[channel+0] = 0;

        // right (contralateral) = 2 + itdi + itdf
        bqCoef[0][channel+1] = b0;
        bqCoef[1][channel+1] = b1;
        bqCoef[2][channel+1] = b2;
        bqCoef[3][channel+1] = a1;
        bqCoef[4][channel+1] = a2;
        delay[channel+1] = itdi;
    }

    //
    // Second biquad implements the near-field or distance filter.
    //
    if (distance < HRTF_NEARFIELD_MAX) {

        nearFieldFilter(gainL, b0, b1, a1);

        bqCoef[0][channel+4] = b0;
        bqCoef[1][channel+4] = b1;
        bqCoef[2][channel+4] = 0.0f;
        bqCoef[3][channel+4] = a1;
        bqCoef[4][channel+4] = 0.0f;

        nearFieldFilter(gainR, b0, b1, a1);

        bqCoef[0][channel+5] = b0;
        bqCoef[1][channel+5] = b1;
        bqCoef[2][channel+5] = 0.0f;
        bqCoef[3][channel+5] = a1;
        bqCoef[4][channel+5] = 0.0f;

    } else {

        lowpassBiquad(lpf, b0, b1, b2, a1, a2);

        bqCoef[0][channel+4] = b0;
        bqCoef[1][channel+4] = b1;
        bqCoef[2][channel+4] = b2;
        bqCoef[3][channel+4] = a1;
        bqCoef[4][channel+4] = a2;

        bqCoef[0][channel+5] = b0;
        bqCoef[1][channel+5] = b1;
        bqCoef[2][channel+5] = b2;
        bqCoef[3][channel+5] = a1;
        bqCoef[4][channel+5] = a2;
    }
}

void AudioHRTF::render(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames,
                       float lpfDistance) {

    assert(index >= 0);
    assert(index < HRTF_TABLES);
    assert(numFrames == HRTF_BLOCK);

    ALIGN32 float in[HRTF_TAPS + HRTF_BLOCK];               // mono
    ALIGN32 float firCoef[4][HRTF_TAPS];                    // 4-channel
    ALIGN32 float firBuffer[4][HRTF_DELAY + HRTF_BLOCK];    // 4-channel
    ALIGN32 float bqCoef[5][8];                             // 4-channel (interleaved)
    ALIGN32 float bqBuffer[4 * HRTF_BLOCK];                 // 4-channel (interleaved)
    int delay[4];                                           // 4-channel (interleaved)

    // apply global and local gain adjustment
    gain *= _gainAdjust;

    // apply distance filter
    float lpf = 0.5f * fastLog2f(std::max(distance, 1.0f)) / fastLog2f(std::max(lpfDistance, 2.0f));
    lpf = std::min(std::max(lpf, 0.0f), 1.0f);

    // disable interpolation from reset state
    if (_resetState) {
        _azimuthState = azimuth;
        _distanceState = distance;
        _gainState = gain;
        _lpfState = lpf;
    }

    // to avoid polluting the cache, old filters are recomputed instead of stored
    setFilters(firCoef, bqCoef, delay, index, _azimuthState, _distanceState, _gainState, _lpfState, L0);

    // compute new filters
    setFilters(firCoef, bqCoef, delay, index, azimuth, distance, gain, lpf, L1);

    // new parameters become old
    _azimuthState = azimuth;
    _distanceState = distance;
    _gainState = gain;
    _lpfState = lpf;

    // convert mono input to float
    for (int i = 0; i < HRTF_BLOCK; i++) {
        in[HRTF_TAPS+i] = (float)input[i] * (1/32768.0f);
    }

    // FIR state update
    memcpy(in, _firState, HRTF_TAPS * sizeof(float));
    memcpy(_firState, &in[HRTF_BLOCK], HRTF_TAPS * sizeof(float));

    // process old/new FIR
    FIR_1x4(&in[HRTF_TAPS], 
            &firBuffer[L0][HRTF_DELAY], 
            &firBuffer[R0][HRTF_DELAY], 
            &firBuffer[L1][HRTF_DELAY], 
            &firBuffer[R1][HRTF_DELAY], 
            firCoef, HRTF_BLOCK);

    // delay state update
    memcpy(firBuffer[L0], _delayState[L0], HRTF_DELAY * sizeof(float));
    memcpy(firBuffer[R0], _delayState[R0], HRTF_DELAY * sizeof(float));
    memcpy(firBuffer[L1], _delayState[L1], HRTF_DELAY * sizeof(float));
    memcpy(firBuffer[R1], _delayState[R1], HRTF_DELAY * sizeof(float));

    memcpy(_delayState[L0], &firBuffer[L1][HRTF_BLOCK], HRTF_DELAY * sizeof(float));  // new state becomes old
    memcpy(_delayState[R0], &firBuffer[R1][HRTF_BLOCK], HRTF_DELAY * sizeof(float));  // new state becomes old
    memcpy(_delayState[L1], &firBuffer[L1][HRTF_BLOCK], HRTF_DELAY * sizeof(float));
    memcpy(_delayState[R1], &firBuffer[R1][HRTF_BLOCK], HRTF_DELAY * sizeof(float));

    // interleave with old/new integer delay
    interleave_4x4(&firBuffer[L0][HRTF_DELAY] - delay[L0],
                   &firBuffer[R0][HRTF_DELAY] - delay[R0],
                   &firBuffer[L1][HRTF_DELAY] - delay[L1],
                   &firBuffer[R1][HRTF_DELAY] - delay[R1],
                   bqBuffer, HRTF_BLOCK);

    // process old/new biquads
    biquad2_4x4(bqBuffer, bqBuffer, bqCoef, _bqState, HRTF_BLOCK);

    // new state becomes old
    _bqState[0][L0] = _bqState[0][L1];
    _bqState[1][L0] = _bqState[1][L1];
    _bqState[2][L0] = _bqState[2][L1];

    _bqState[0][R0] = _bqState[0][R1];
    _bqState[1][R0] = _bqState[1][R1];
    _bqState[2][R0] = _bqState[2][R1];

    _bqState[0][L2] = _bqState[0][L3];
    _bqState[1][L2] = _bqState[1][L3];
    _bqState[2][L2] = _bqState[2][L3];

    _bqState[0][R2] = _bqState[0][R3];
    _bqState[1][R2] = _bqState[1][R3];
    _bqState[2][R2] = _bqState[2][R3];

    // crossfade old/new output and accumulate
    crossfade_4x2(bqBuffer, output, crossfadeTable, HRTF_BLOCK);

    _resetState = false;
}

void AudioHRTF::mixMono(int16_t* input, float* output, float gain, int numFrames) {

    assert(numFrames == HRTF_BLOCK);

    // apply global and local gain adjustment
    gain *= _gainAdjust;

    // disable interpolation from reset state
    if (_resetState) {
        _gainState = gain;
    }

    // crossfade gain and accumulate
    gainfade_1x2(input, output, crossfadeTable, _gainState, gain, HRTF_BLOCK);

    // new parameters become old
    _gainState = gain;

    _resetState = false;
}

void AudioHRTF::mixStereo(int16_t* input, float* output, float gain, int numFrames) {

    assert(numFrames == HRTF_BLOCK);

    // apply global and local gain adjustment
    gain *= _gainAdjust;

    // disable interpolation from reset state
    if (_resetState) {
        _gainState = gain;
    }

    // crossfade gain and accumulate
    gainfade_2x2(input, output, crossfadeTable, _gainState, gain, HRTF_BLOCK);

    // new parameters become old
    _gainState = gain;

    _resetState = false;
}
