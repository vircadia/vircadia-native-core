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

#include <math.h>
#include <string.h>
#include <assert.h>

#include "AudioHRTF.h"
#include "AudioHRTFData.h"

#if defined(_MSC_VER)
#define ALIGN32 __declspec(align(32))
#elif defined(__GNUC__)
#define ALIGN32 __attribute__((aligned(32)))
#else
#define ALIGN32
#endif

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
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
// Model the frequency-dependent attenuation of sound propogation in air.
//
// Fit using linear regression to a log-log model of lowpass cutoff frequency vs distance,
// loosely based on data from Handbook of Acoustics. Only the onset of significant
// attenuation is modelled, not the filter slope.
//
//   1m -> -3dB @ 55kHz
//  10m -> -3dB @ 12kHz
// 100m -> -3dB @ 2.5kHz
//  1km -> -3dB @ 0.6kHz
// 10km -> -3dB @ 0.1kHz
//
static const int NLOWPASS = 64;
static const float lowpassTable[NLOWPASS][5] = {    // { b0, b1, b2, a1, a2 }
    // distance = 1
    { 0.999772371f, 1.399489756f,  0.454495527f,  1.399458985f, 0.454298669f },
    { 0.999631480f, 1.357609808f,  0.425210203f,  1.357549905f, 0.424901586f },
    { 0.999405154f, 1.311503050f,  0.394349994f,  1.311386830f, 0.393871368f },
    { 0.999042876f, 1.260674595f,  0.361869089f,  1.260450057f, 0.361136504f },
    // distance = 2
    { 0.998465222f, 1.204646525f,  0.327757118f,  1.204214978f, 0.326653886f },
    { 0.997548106f, 1.143019308f,  0.292064663f,  1.142195387f, 0.290436690f },
    { 0.996099269f, 1.075569152f,  0.254941286f,  1.074009405f, 0.252600301f },
    { 0.993824292f, 1.002389610f,  0.216688640f,  0.999469185f, 0.213433357f },
    // distance = 4
    { 0.990280170f, 0.924075266f,  0.177827150f,  0.918684864f, 0.173497723f },
    { 0.984818279f, 0.841917936f,  0.139164195f,  0.832151968f, 0.133748443f },
    { 0.976528670f, 0.758036513f,  0.101832398f,  0.740761682f, 0.095635899f },
    { 0.964216485f, 0.675305244f,  0.067243474f,  0.645654855f, 0.061110348f },
    // distance = 8
    { 0.946463038f, 0.596943020f,  0.036899688f,  0.547879974f, 0.032425772f },
    { 0.921823868f, 0.525770189f,  0.012060451f,  0.447952111f, 0.011702396f },
    { 0.890470015f, 0.463334299f, -0.001227816f,  0.347276405f, 0.005300092f },
    { 0.851335343f, 0.407521164f, -0.009353968f,  0.241900234f, 0.007602305f },
    // distance = 16
    { 0.804237360f, 0.358139558f, -0.014293332f,  0.130934213f, 0.017149373f },
    { 0.750073259f, 0.314581568f, -0.016625381f,  0.014505388f, 0.033524057f },
    { 0.690412072f, 0.275936128f, -0.017054561f, -0.106682490f, 0.055976129f },
    { 0.627245545f, 0.241342015f, -0.016246850f, -0.231302564f, 0.083643275f },
    // distance = 32
    { 0.562700627f, 0.210158533f, -0.014740899f, -0.357562697f, 0.115680957f },
    { 0.498787849f, 0.181982455f, -0.012925406f, -0.483461730f, 0.151306628f },
    { 0.437224055f, 0.156585449f, -0.011055180f, -0.607042210f, 0.189796534f },
    { 0.379336998f, 0.133834032f, -0.009281617f, -0.726580065f, 0.230469477f },
    // distance = 64
    { 0.326040627f, 0.113624970f, -0.007683443f, -0.840693542f, 0.272675696f },
    { 0.277861727f, 0.095845793f, -0.006291936f, -0.948380091f, 0.315795676f },
    { 0.234997480f, 0.080357656f, -0.005109519f, -1.049001190f, 0.359246807f },
    { 0.197386484f, 0.066993521f, -0.004122547f, -1.142236313f, 0.402493771f },
    // distance = 128
    { 0.164780457f, 0.055564709f, -0.003309645f, -1.228023442f, 0.445058962f },
    { 0.136808677f, 0.045870650f, -0.002646850f, -1.306498037f, 0.486530514f },
    { 0.113031290f, 0.037708627f, -0.002110591f, -1.377937457f, 0.526566783f },
    { 0.092980475f, 0.030881892f, -0.001679255f, -1.442713983f, 0.564897095f },
    // distance = 256
    { 0.076190239f, 0.025205585f, -0.001333863f, -1.501257246f, 0.601319206f },
    { 0.062216509f, 0.020510496f, -0.001058229f, -1.554025452f, 0.635694228f },
    { 0.050649464f, 0.016644994f, -0.000838826f, -1.601484205f, 0.667939837f },
    { 0.041120009f, 0.013475547f, -0.000664513f, -1.644091518f, 0.698022561f },
    // distance = 512
    { 0.033302044f, 0.010886252f, -0.000526217f, -1.682287704f, 0.725949783f },
    { 0.026911868f, 0.008777712f, -0.000416605f, -1.716488979f, 0.751761953f },
    { 0.021705773f, 0.007065551f, -0.000329788f, -1.747083800f, 0.775525335f },
    { 0.017476603f, 0.005678758f, -0.000261057f, -1.774431204f, 0.797325509f },
    // distance = 1024
    { 0.014049828f, 0.004558012f, -0.000206658f, -1.798860530f, 0.817261711f },
    { 0.011279504f, 0.003654067f, -0.000163610f, -1.820672082f, 0.835442043f },
    { 0.009044384f, 0.002926264f, -0.000129544f, -1.840138412f, 0.851979516f },
    { 0.007244289f, 0.002341194f, -0.000102586f, -1.857505967f, 0.866988864f },
    // distance = 2048
    { 0.005796846f, 0.001871515f, -0.000081250f, -1.872996926f, 0.880584038f },
    { 0.004634607f, 0.001494933f, -0.000064362f, -1.886811124f, 0.892876302f },
    { 0.003702543f, 0.001193324f, -0.000050993f, -1.899127955f, 0.903972829f },
    { 0.002955900f, 0.000951996f, -0.000040407f, -1.910108223f, 0.913975712f },
    // distance = 4096
    { 0.002358382f, 0.000759068f, -0.000032024f, -1.919895894f, 0.922981321f },
    { 0.001880626f, 0.000604950f, -0.000025383f, -1.928619738f, 0.931079931f },
    { 0.001498926f, 0.000481920f, -0.000020123f, -1.936394836f, 0.938355560f },
    { 0.001194182f, 0.000383767f, -0.000015954f, -1.943323983f, 0.944885977f },
    // distance = 8192
    { 0.000951028f, 0.000305502f, -0.000012651f, -1.949498943f, 0.950742822f },
    { 0.000757125f, 0.000243126f, -0.000010033f, -1.955001608f, 0.955991826f },
    { 0.000602572f, 0.000193434f, -0.000007957f, -1.959905036f, 0.960693085f },
    { 0.000479438f, 0.000153861f, -0.000006312f, -1.964274383f, 0.964901371f },
    // distance = 16384
    { 0.000381374f, 0.000122359f, -0.000005007f, -1.968167752f, 0.968666478f },
    { 0.000303302f, 0.000097288f, -0.000003972f, -1.971636944f, 0.972033562f },
    { 0.000241166f, 0.000077342f, -0.000003151f, -1.974728138f, 0.975043493f },
    { 0.000191726f, 0.000061475f, -0.000002500f, -1.977482493f, 0.977733194f },
    // distance = 32768
    { 0.000152399f, 0.000048857f, -0.000001984f, -1.979936697f, 0.980135969f },
    { 0.000121122f, 0.000038825f, -0.000001574f, -1.982123446f, 0.982281818f },
    { 0.000096252f, 0.000030849f, -0.000001249f, -1.984071877f, 0.984197728f },
    { 0.000076480f, 0.000024509f, -0.000000991f, -1.985807957f, 0.985907955f },
};

static const float TWOPI = 6.283185307f;

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

        assert(HRTF_TAPS % 4 == 0);

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

//
// Runtime CPU dispatch
//

#include "CPUDetect.h"

void FIR_1x4_AVX2(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames);
void FIR_1x4_AVX512(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames);

static void FIR_1x4(float* src, float* dst0, float* dst1, float* dst2, float* dst3, float coef[4][HRTF_TAPS], int numFrames) {

    static auto f = cpuSupportsAVX512() ? FIR_1x4_AVX512 : (cpuSupportsAVX2() ? FIR_1x4_AVX2 : FIR_1x4_SSE);
    (*f)(src, dst0, dst1, dst2, dst3, coef, numFrames); // dispatch
}

// 4 channel planar to interleaved
static void interleave_4x4(float* src0, float* src1, float* src2, float* src3, float* dst, int numFrames) {

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
static void biquad2_4x4(float* src, float* dst, float coef[5][8], float state[3][8], int numFrames) {

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
static void crossfade_4x2(float* src, float* dst, const float* win, int numFrames) {

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
static void interpolate(float* dst, const float* src0, const float* src1, float frac, float gain) {

    __m128 f0 = _mm_set1_ps(gain * (1.0f - frac));
    __m128 f1 = _mm_set1_ps(gain * frac);

    assert(HRTF_TAPS % 4 == 0);

    for (int k = 0; k < HRTF_TAPS; k += 4) {

        __m128 x0 = _mm_loadu_ps(&src0[k]);
        __m128 x1 = _mm_loadu_ps(&src1[k]);

        x0 = _mm_add_ps(_mm_mul_ps(f0, x0), _mm_mul_ps(f1, x1));

        _mm_storeu_ps(&dst[k], x0);
    }
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

        assert(HRTF_TAPS % 4 == 0);

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
static void interpolate(float* dst, const float* src0, const float* src1, float frac, float gain) {

    float f0 = HRTF_GAIN * gain * (1.0f - frac);
    float f1 = HRTF_GAIN * gain * frac;

    for (int k = 0; k < HRTF_TAPS; k++) {
        dst[k] = f0 * src0[k] + f1 * src1[k];
    }
}

#endif

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

static void distanceBiquad(float distance, float& b0, float& b1, float& b2, float& a1, float& a2) {

    //
    // Computed from a lookup table quantized to distance = 2^(N/4)
    // and reconstructed by piecewise linear interpolation.
    // Approximation error < 0.25dB
    //

    float x = distance;
    x = MIN(MAX(x, 1.0f), 1<<30);
    x *= x;
    x *= x; // x = distance^4

    // split x into e and frac, such that x = 2^(e+0) + frac * (2^(e+1) - 2^(e+0))
    int e;
    float frac;
    splitf(x, e, frac);

    // clamp to table limits
    if (e < 0) {
        e = 0;
        frac = 0.0f;
    } 
    if (e > NLOWPASS-2) {
        e = NLOWPASS-2;
        frac = 1.0f;
    }
    assert(frac >= 0.0f);
    assert(frac <= 1.0f);
    assert(e+0 >= 0);
    assert(e+1 < NLOWPASS);

    // piecewise linear interpolation
    b0 = lowpassTable[e+0][0] + frac * (lowpassTable[e+1][0] - lowpassTable[e+0][0]);
    b1 = lowpassTable[e+0][1] + frac * (lowpassTable[e+1][1] - lowpassTable[e+0][1]);
    b2 = lowpassTable[e+0][2] + frac * (lowpassTable[e+1][2] - lowpassTable[e+0][2]);
    a1 = lowpassTable[e+0][3] + frac * (lowpassTable[e+1][3] - lowpassTable[e+0][3]);
    a2 = lowpassTable[e+0][4] + frac * (lowpassTable[e+1][4] - lowpassTable[e+0][4]);
}

// compute new filters for a given azimuth, distance and gain
static void setFilters(float firCoef[4][HRTF_TAPS], float bqCoef[5][8], int delay[4], 
                       int index, float azimuth, float distance, float gain, int channel) {

    // convert from radians to table units
    azimuth *= HRTF_AZIMUTHS / TWOPI;

    // wrap to principle value
    while (azimuth < 0.0f) {
        azimuth += HRTF_AZIMUTHS;
    }
    while (azimuth >= HRTF_AZIMUTHS) {
        azimuth -= HRTF_AZIMUTHS;
    }

    // table parameters
    int az0 = (int)azimuth;
    int az1 = (az0 + 1) % HRTF_AZIMUTHS;
    float frac = azimuth - (float)az0;

    assert((az0 >= 0) && (az0 < HRTF_AZIMUTHS));
    assert((az1 >= 0) && (az1 < HRTF_AZIMUTHS));
    assert((frac >= 0.0f) && (frac < 1.0f));

    // interpolate FIR
    interpolate(firCoef[channel+0], ir_table_table[index][az0][0], ir_table_table[index][az1][0], frac, gain);
    interpolate(firCoef[channel+1], ir_table_table[index][az0][1], ir_table_table[index][az1][1], frac, gain);

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
    // Second biquad implements the distance filter.
    //
    distanceBiquad(distance, b0, b1, b2, a1, a2);

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

void AudioHRTF::render(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames) {

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

    // to avoid polluting the cache, old filters are recomputed instead of stored
    setFilters(firCoef, bqCoef, delay, index, _azimuthState, _distanceState, _gainState, L0);

    // compute new filters
    setFilters(firCoef, bqCoef, delay, index, azimuth, distance, gain, L1);

    // new parameters become old
    _azimuthState = azimuth;
    _distanceState = distance;
    _gainState = gain;

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

    _silentState = false;
}

void AudioHRTF::renderSilent(int16_t* input, float* output, int index, float azimuth, float distance, float gain, int numFrames) {

    // process the first silent block, to flush internal state
    if (!_silentState) {
        render(input, output, index, azimuth, distance, gain, numFrames);
    } 

    // new parameters become old
    _azimuthState = azimuth;
    _distanceState = distance;
    _gainState = gain;

    _silentState = true;
}
