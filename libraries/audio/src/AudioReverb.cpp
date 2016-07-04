//
//  AudioReverb.cpp
//  libraries/audio/src
//
//  Created by Ken Cooke on 10/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "AudioReverb.h"

#ifdef _MSC_VER

#include <intrin.h>
inline static int MULHI(int a, int b) {
    long long c = __emul(a, b);
    return ((int*)&c)[1];
}

#else

#define MULHI(a,b)  (int)(((long long)(a) * (b)) >> 32)

#endif  // _MSC_VER

#ifndef MAX
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#endif

static const float PHI = 0.6180339887f; // maximum allpass diffusion
static const float TWOPI = 6.283185307f;

static const double PI = 3.14159265358979323846;
static const double SQRT2 = 1.41421356237309504880;

static const double FIXQ31 = 2147483648.0;
static const double FIXQ32 = 4294967296.0;

// Round an integer to the next power-of-two, at compile time.
// VS2013 does not support constexpr so macros are used instead.
#define SETBITS0(x) (x)
#define SETBITS1(x) (SETBITS0(x) | (SETBITS0(x) >>  1))
#define SETBITS2(x) (SETBITS1(x) | (SETBITS1(x) >>  2))
#define SETBITS3(x) (SETBITS2(x) | (SETBITS2(x) >>  4))
#define SETBITS4(x) (SETBITS3(x) | (SETBITS3(x) >>  8))
#define SETBITS5(x) (SETBITS4(x) | (SETBITS4(x) >> 16))
#define NEXTPOW2(x) (SETBITS5((x) - 1) + 1)

//
// Allpass delay modulation
//
static const int MOD_INTBITS = 4;
static const int MOD_FRACBITS = 31 - MOD_INTBITS;
static const uint32_t MOD_FRACMASK = (1 << MOD_FRACBITS) - 1;
static const float QMOD_TO_FLOAT = 1.0f / (1 << MOD_FRACBITS);

//
// Reverb delay values, defined for sampleRate=48000 roomSize=100% density=100%
//
// max path should already be prime, to prevent getPrime(NEXTPOW2(N)) > NEXTPOW2(N)
//
static const int M_PD0 = 16000; // max predelay = 333ms

static const int M_AP0 = 83;
static const int M_AP1 = 211;
static const int M_AP2 = 311;
static const int M_AP3 = 97;
static const int M_AP4 = 223;
static const int M_AP5 = 293;

static const int M_MT0 = 1017;
static const int M_MT1 = 4077;
static const int M_MT2 = 2039;
static const int M_MT3 = 1017;
static const int M_MT4 = 2593;
static const int M_MT5 = 2039;

static const int M_MT1_2 = 600;
static const int M_MT4_2 = 600;
static const int M_MT1_MAX = MAX(M_MT1, M_MT1_2);
static const int M_MT4_MAX = MAX(M_MT4, M_MT4_2);

static const int M_AP6 = 817;
static const int M_AP7 = 513;
static const int M_AP8 = 765;
static const int M_AP9 = 465;
static const int M_AP10 = 3021;
static const int M_AP11 = 2121;
static const int M_AP12 = 1705;
static const int M_AP13 = 1081;
static const int M_AP14 = 3313;
static const int M_AP15 = 2205;
static const int M_AP16 = 1773;
static const int M_AP17 = 981;

static const int M_AP7_MAX = M_AP7 + (1 << MOD_INTBITS) + 1;    // include max excursion
static const int M_AP9_MAX = M_AP9 + (1 << MOD_INTBITS) + 1;

static const int M_MT6 = 6863;
static const int M_MT7 = 7639;
static const int M_MT8 = 3019;
static const int M_MT9 = 2875;

static const int M_LD0 = 8000;  // max late delay = 166ms
static const int M_MT6_MAX = MAX(M_MT6, M_LD0);
static const int M_MT7_MAX = MAX(M_MT7, M_LD0);
static const int M_MT8_MAX = MAX(M_MT8, M_LD0);
static const int M_MT9_MAX = MAX(M_MT9, M_LD0);

static const int M_AP18 = 131;
static const int M_AP19 = 113;
static const int M_AP20 = 107;
static const int M_AP21 = 127;

//
// Filter design tools using analog-matched response.
// All filter types approximate the s-plane response, including cutoff > Nyquist.
//

static float dBToGain(float dB) {
    return powf(10.0f, dB * (1/20.0f));
}

static double dBToGain(double dB) {
    return pow(10.0, dB * (1/20.0));
}

// Returns the gain of analog (s-plane) peak-filter evaluated at w
static double analogPeak(double w0, double G, double Q, double w) {
    double w0sq, wsq, Gsq, Qsq;
    double num, den;

    w0sq = w0 * w0;
    wsq = w * w;
    Gsq = G * G;
    Qsq = Q * Q;

    num = Qsq * (wsq - w0sq) * (wsq - w0sq) + wsq * w0sq * Gsq;
    den = Qsq * (wsq - w0sq) * (wsq - w0sq) + wsq * w0sq;

    return sqrt(num / den);
}

// Returns the gain of analog (s-plane) shelf evaluated at w
// non-resonant Q = sqrt(0.5) so 1/Q^2 = 2.0
static double analogShelf(double w0, double G, double resonance, int isHigh, double w) {
    double w0sq, wsq, Qrsq;
    double num, den;

    resonance = MIN(MAX(resonance, 0.0), 1.0);
    Qrsq = 2.0 * pow(G, 1.0 - resonance);   // resonant 1/Q^2

    w0sq = w0 * w0;
    wsq = w * w;

    if (isHigh) {
        num = (G*wsq - w0sq) * (G*wsq - w0sq) + wsq * w0sq * Qrsq;
    } else {
        num = (wsq - G*w0sq) * (wsq - G*w0sq) + wsq * w0sq * Qrsq;
    }
    den = wsq * wsq + w0sq * w0sq;

    return sqrt(num / den);
}

// Returns the gain of analog (s-plane) highpass/lowpass evaluated at w
// Q = sqrt(0.5) = 2nd order Butterworth
static double analogFilter(double w0, int isHigh, double w) {
    double w0sq, wsq;
    double num, den;

    w0sq = w0 * w0;
    wsq = w * w;

    if (isHigh) {
        num = wsq * wsq;
    } else {
        num = w0sq * w0sq;
    }
    den = wsq * wsq + w0sq * w0sq;

    return sqrt(num / den);
}

//
// Biquad Peaking EQ using analog matching.
// NOTE: Peak topology becomes ill-conditioned as w0 approaches pi.
//
static void BQPeakBelowPi(double coef[5], double w0, double dbgain, double Q) {
    double G, G1, Gsq, G1sq, Qsq;
    double G11, G00, Gratio;
    double wpi, wh, w0sq, whsq;
    double Wsq, B, A;
    double b0, b1, b2, a0, a1, a2;
    double temp, scale;
    int isUnity;

    // convert cut into boost, invert later
    G = dBToGain(fabs(dbgain));

    // special case near unity gain
    isUnity = G < 1.001;
    G = MAX(G, 1.001);

    // compute the Nyquist gain
    wpi = w0 + 2.8 * (1.0 - w0/PI); // minimax-like error
    wpi = MIN(wpi, PI);
    G1 = analogPeak(w0, G, Q, wpi);

    G1sq = G1 * G1;
    Gsq = G * G;
    Qsq = Q * Q;

    // compute the analog half-gain frequency
    temp = G + 2.0 * Qsq - sqrt(Gsq + 4.0 * Qsq * G);
    wh = sqrt(temp) * w0 / (Q * SQRT2);

    // prewarp freqs of w0 and wh
    w0 = tan(0.5 * w0);
    wh = tan(0.5 * wh);
    w0sq = w0 * w0;
    whsq = wh * wh;

    // compute Wsq, from asymmetry due to G1
    G11 = Gsq - G1sq;
    G00 = Gsq - 1.0;
    Gratio = G11 / G00;
    Wsq = w0sq * sqrt(Gratio);

    // compute B, matching gains at w0 and wh
    temp = 2.0 * Wsq * (G1 - sqrt(Gratio));
    temp += Gsq * whsq * (1.0 - G1sq) / G00;
    temp += G * Gratio * (w0sq - whsq) * (w0sq - whsq) / whsq;
    B = sqrt(temp);

    // compute A, matching gains at w0 and wh
    temp = 2.0 * Wsq;
    temp += (2.0 * G11 * w0sq + (G1sq - G) * whsq) / (G - Gsq);
    temp += Gratio * w0sq * w0sq / (G * whsq);
    A = sqrt(temp);

    // design digital filter via bilinear transform
    b0 = G1 + B + Wsq;
    b1 = 2.0 * (Wsq - G1);
    b2 = G1 - B + Wsq;
    a0 = 1.0 + A + Wsq;
    a1 = 2.0 * (Wsq -  1.0);
    a2 = 1.0 - A + Wsq;

    // for unity gain, ensure poles/zeros in the right place.
    // needed for smooth interpolation when gain is changed.
    if (isUnity) {
        b0 = a0;
        b1 = a1;
        b2 = a2;
    }

    // invert filter for cut
    if (dbgain < 0.0) {
        temp = b0; b0 = a0; a0 = temp;
        temp = b1; b1 = a1; a1 = temp;
        temp = b2; b2 = a2; a2 = temp;
    }

    // normalize
    scale = 1.0 / a0;
    coef[0] = b0 * scale;
    coef[1] = b1 * scale;
    coef[2] = b2 * scale;
    coef[3] = a1 * scale;
    coef[4] = a2 * scale;
}

//
// Biquad Peaking EQ using a shelf instead of peak.
//
// This uses a shelf topology, matched to the analog peaking filter located above Nyquist.
//
// NOTE: the result is close, but not identical to BQPeakBelowPi(), since a pole/zero
// pair must jump to the right side of the real axis.  Eg. inflection at the peak goes away.
// However, interpolation from peak to shelf is well behaved if the switch is made near pi,
// as the pole/zero be near (and travel down) the real axis.
//
static void BQPeakAbovePi(double coef[5], double w0, double dbgain, double Q) {
    double G, G1, Gsq, Qsq;
    double wpi, wh, wn, wd;
    double wna, wda;
    double gn, gd, gnsq, gdsq;
    double num, den;
    double Wnsq, Wdsq, B, A;
    double b0, b1, b2, a0, a1, a2;
    double temp, scale;
    int isUnity;

    // convert cut into boost, invert later
    G = dBToGain(fabs(dbgain));

    // special case near unity gain
    isUnity = G < 1.001;
    G = MAX(G, 1.001);

    // compute the Nyquist gain
    wpi = PI;
    if (w0 < PI) {
        G1 = G;                         // use the peak gain
    } else {
        G1 = analogPeak(w0, G, Q, wpi); // use Nyquist gain of analog response
    }

    Gsq = G * G;
    Qsq = Q * Q;

    // compute the analog half-gain frequency
    temp = G + 2.0 * Qsq - sqrt(Gsq + 4.0 * Qsq * G);
    wh = sqrt(temp) * w0 / (Q * SQRT2);

    // approximate wn and wd
    // use half-gain frequency as mapping
    wn = 0.5 * wh / sqrt(sqrt((G1)));
    wd = wn * sqrt(G1);
    Wnsq = wn * wn;
    Wdsq = wd * wd;

    // analog freqs of wn and wd
    wna = 2.0 * atan(wn);
    wda = 2.0 * atan(wd);

    // normalized analog gains at wna and wda
    temp = 1.0 / G1;
    gn = temp * analogPeak(w0, G, Q, wna);
    gd = temp * analogPeak(w0, G, Q, wda);
    gnsq = gn * gn;
    gdsq = gd * gd;

    // compute B, matching gains at wn and wd
    temp = 1.0 / (wn * wd);
    den = fabs(gnsq - gdsq);
    num = gnsq * (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gdsq * Wdsq);
    B = temp * sqrt(num / den);

    // compute A, matching gains at wn and wd
    num = (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gnsq * Wdsq);
    A = temp * sqrt(num / den);

    // design digital filter via bilinear transform
    b0 = G1 * (1.0 + B + Wnsq);
    b1 = G1 * 2.0 * (Wnsq - 1.0);
    b2 = G1 * (1.0 - B + Wnsq);
    a0 = 1.0 + A + Wdsq;
    a1 = 2.0 * (Wdsq -  1.0);
    a2 = 1.0 - A + Wdsq;

    // for unity gain, ensure poles/zeros are in the right place.
    // allows smooth coefficient interpolation when gain is changed.
    if (isUnity) {
        b0 = a0;
        b1 = a1;
        b2 = a2;
    }

    // invert filter for cut
    if (dbgain < 0.0) {
        temp = b0; b0 = a0; a0 = temp;
        temp = b1; b1 = a1; a1 = temp;
        temp = b2; b2 = a2; a2 = temp;
    }

    // normalize
    scale = 1.0 / a0;
    coef[0] = b0 * scale;
    coef[1] = b1 * scale;
    coef[2] = b2 * scale;
    coef[3] = a1 * scale;
    coef[4] = a2 * scale;
}

//
// Biquad Peaking EQ using analog matching.
// Supports full range of w0.
//
void BQPeak(double coef[5], double w0, double dbgain, double Q) {
    w0 = MAX(w0, 0.0);  // allow w0 > pi

    Q = MIN(MAX(Q, 1.0e-6), 1.0e+6);

    // Switch from peak to shelf, just before w0 = pi.
    // Too early causes a jump in the peak location, which interpolates to lowered gains.
    // Too late becomes ill-conditioned, and makes no improvement to interpolated response.
    // 3.14 is a good choice.
    if (w0 > 3.14) {
        BQPeakAbovePi(coef, w0, dbgain, Q);
    } else {
        BQPeakBelowPi(coef, w0, dbgain, Q);
    }
}

//
// Biquad Shelf using analog matching.
//
void BQShelf(double coef[5], double w0, double dbgain, double resonance, int isHigh) {
    double G, G1;
    double wpi, wn, wd;
    double wna, wda;
    double gn, gd, gnsq, gdsq;
    double num, den;
    double Wnsq, Wdsq, B, A;
    double b0, b1, b2, a0, a1, a2;
    double temp, scale;
    int isUnity;

    w0 = MAX(w0, 0.0);  // allow w0 > pi

    resonance = MIN(MAX(resonance, 0.0), 1.0);

    // convert cut into boost, invert later
    G = dBToGain(fabs(dbgain));

    // special case near unity gain
    isUnity = G < 1.001;
    G = MAX(G, 1.001);

    // compute the Nyquist gain
    wpi = w0 + 2.8 * (1.0 - w0/PI); // minimax-like error
    wpi = MIN(wpi, PI);
    G1 = analogShelf(w0, G, resonance, isHigh, wpi);

    // approximate wn and wd
    if (isHigh) {
        // use center as mapping
        wn = 0.5 * w0 / sqrt(sqrt((G * G1)));
        wd = wn * sqrt(G1);
    } else {
        // use wd as mapping
        wd = 0.5 * w0;
        wn = wd * sqrt(G/G1);
    }
    Wnsq = wn * wn;
    Wdsq = wd * wd;

    // analog freqs of wn and wd
    wna = 2.0 * atan(wn);
    wda = 2.0 * atan(wd);

    // normalized analog gains at wna and wda
    temp = 1.0 / G1;
    gn = temp * analogShelf(w0, G, resonance, isHigh, wna);
    gd = temp * analogShelf(w0, G, resonance, isHigh, wda);
    gnsq = gn * gn;
    gdsq = gd * gd;

    // compute B, matching gains at wn and wd
    temp = 1.0 / (wn * wd);
    den = fabs(gnsq - gdsq);
    num = gnsq * (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gdsq * Wdsq);
    B = temp * sqrt(num / den);

    // compute A, matching gains at wn and wd
    num = (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gnsq * Wdsq);
    A = temp * sqrt(num / den);

    // design digital filter via bilinear transform
    b0 = G1 * (1.0 + B + Wnsq);
    b1 = G1 * 2.0 * (Wnsq - 1.0);
    b2 = G1 * (1.0 - B + Wnsq);
    a0 = 1.0 + A + Wdsq;
    a1 = 2.0 * (Wdsq -  1.0);
    a2 = 1.0 - A + Wdsq;

    // for unity gain, ensure poles/zeros in the right place.
    // needed for smooth interpolation when gain is changed.
    if (isUnity) {
        b0 = a0;
        b1 = a1;
        b2 = a2;
    }

    // invert filter for cut
    if (dbgain < 0.0) {
        temp = b0; b0 = a0; a0 = temp;
        temp = b1; b1 = a1; a1 = temp;
        temp = b2; b2 = a2; a2 = temp;
    }

    // normalize
    scale = 1.0 / a0;
    coef[0] = b0 * scale;
    coef[1] = b1 * scale;
    coef[2] = b2 * scale;
    coef[3] = a1 * scale;
    coef[4] = a2 * scale;
}

//
// Biquad Lowpass/Highpass using analog matching.
// Q = sqrt(0.5) = 2nd order Butterworth
//
void BQFilter(double coef[5], double w0, int isHigh) {
    double G1;
    double wpi, wn, wd;
    double wna, wda;
    double gn, gd, gnsq, gdsq;
    double num, den;
    double Wnsq, Wdsq, B, A;
    double b0, b1, b2, a0, a1, a2;
    double temp, scale;

    w0 = MAX(w0, 0.0);  // allow w0 > pi for lowpass

    if (isHigh) {

        w0 = MIN(w0, PI);   // disallow w0 > pi for highpass

        // compute the Nyquist gain
        wpi = PI;
        G1 = analogFilter(w0, isHigh, wpi);

        // approximate wn and wd
        wn = 0.0;   // zeros at zero
        wd = 0.5 * w0;

        Wnsq = wn * wn;
        Wdsq = wd * wd;

        // compute B and A
        B = 0.0;
        A = SQRT2 * Wdsq / atan(wd);    // Qd = sqrt(0.5) * atan(wd)/wd;

    } else {

        // compute the Nyquist gain
        wpi = w0 + 2.8 * (1.0 - w0/PI); // minimax-like error
        wpi = MIN(wpi, PI);
        G1 = analogFilter(w0, isHigh, wpi);

        // approximate wn and wd
        wd = 0.5 * w0;
        wn = wd * sqrt(1.0/G1); // down G1 at pi, instead of zeros

        Wnsq = wn * wn;
        Wdsq = wd * wd;

        // analog freqs of wn and wd
        wna = 2.0 * atan(wn);
        wda = 2.0 * atan(wd);

        // normalized analog gains at wna and wda
        temp = 1.0 / G1;
        gn = temp * analogFilter(w0, isHigh, wna);
        gd = temp * analogFilter(w0, isHigh, wda);
        gnsq = gn * gn;
        gdsq = gd * gd;

        // compute B, matching gains at wn and wd
        temp = 1.0 / (wn * wd);
        den = fabs(gnsq - gdsq);
        num = gnsq * (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gdsq * Wdsq);
        B = temp * sqrt(num / den);

        // compute A, matching gains at wn and wd
        num = (Wnsq - Wdsq) * (Wnsq - Wdsq) * (Wnsq + gnsq * Wdsq);
        A = temp * sqrt(num / den);
    }

    // design digital filter via bilinear transform
    b0 = G1 * (1.0 + B + Wnsq);
    b1 = G1 * 2.0 * (Wnsq - 1.0);
    b2 = G1 * (1.0 - B + Wnsq);
    a0 = 1.0 + A + Wdsq;
    a1 = 2.0 * (Wdsq -  1.0);
    a2 = 1.0 - A + Wdsq;

    // normalize
    scale = 1.0 / a0;
    coef[0] = b0 * scale;
    coef[1] = b1 * scale;
    coef[2] = b2 * scale;
    coef[3] = a1 * scale;
    coef[4] = a2 * scale;
}

//
// PoleZero Shelf. For Lowpass/Highpass, setCoef dbgain to -100dB.
// NOTE: w0 always sets the pole frequency (3dB corner from unity gain)
//
void PZShelf(double coef[3], double w0, double dbgain, int isHigh) {
    double G, G0, G1;
    double b0, b1, a0, a1;
    double temp, scale;

    w0 = MAX(w0, 0.0);  // allow w0 > pi

    // convert boost into cut, invert later
    G = dBToGain(-fabs(dbgain));

    if (isHigh) {
        G0 = 1.0;   // gain at DC
        G1 = G;     // gain at Nyquist
    } else {
        G0 = G;
        G1 = 1.0;
    }

    b0 = 1.0;
    a0 = 1.0;
    b1 = -exp(-w0 * G0 / G1);
    a1 = -exp(-w0);

    b1 += 0.12 * (1.0 + b1) * (1.0 + b1) * (1.0 - G1);  // analog-matched gain near Nyquist

    scale = G0 * (1.0 + a1) / (1.0 + b1);
    b0 *= scale;
    b1 *= scale;

    // invert filter for boost
    if (dbgain > 0.0) {
        temp = b0; b0 = a0; a0 = temp;
        temp = b1; b1 = a1; a1 = temp;
    }

    // normalize
    scale = 1.0 / a0;
    coef[0] = b0 * scale;
    coef[1] = b1 * scale;
    coef[2] = a1 * scale;
}

class BandwidthEQ {

    float _buffer[4] {};

    float _output0 = 0.0f;
    float _output1 = 0.0f;

    float _dcL = 0.0f;
    float _dcR = 0.0f;

    float _b0 = 1.0f;
    float _b1 = 0.0f;
    float _b2 = 0.0f;
    float _a1 = 0.0f;
    float _a2 = 0.0f;

    float _alpha = 0.0f;

public:
    void setFreq(float freq, float sampleRate) {
        freq = MIN(MAX(freq, 1.0f), 24000.0f);

        // lowpass filter, -3dB @ freq
        double coef[5];
        BQFilter(coef, TWOPI * freq / sampleRate, 0);
        _b0 = (float)coef[0];
        _b1 = (float)coef[1];
        _b2 = (float)coef[2];
        _a1 = (float)coef[3];
        _a2 = (float)coef[4];

        // DC-blocking filter, -3dB @ 10Hz
        _alpha = 1.0f - expf(-TWOPI * 10.0f / sampleRate);
    }

    void process(float input0, float input1, float& output0, float& output1) {
        output0 = _output0;
        output1 = _output1;

        // prevent denormalized zero-input limit cycles in the reverb
        input0 += 1.0e-20f;
        input1 += 1.0e-20f;

        // remove DC
        input0 -= _dcL;
        input1 -= _dcR;

        _dcL += _alpha * input0;
        _dcR += _alpha * input1;

        // transposed Direct Form II
        _output0   = _b0 * input0 + _buffer[0];
        _buffer[0] = _b1 * input0 - _a1 * _output0 + _buffer[1];
        _buffer[1] = _b2 * input0 - _a2 * _output0;

        _output1   = _b0 * input1 + _buffer[2];
        _buffer[2] = _b1 * input1 - _a1 * _output1 + _buffer[3];
        _buffer[3] = _b2 * input1 - _a2 * _output1;
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output0 = 0.0f;
        _output1 = 0.0f;
        _dcL = 0.0f;
        _dcR = 0.0f;
    }
};

template<int N>
class DelayLine {

    float _buffer[N] {};

    float _output = 0.0f;

    int _index = 0;
    int _delay = N;

public:
    void setDelay(int d) {
        d = MIN(MAX(d, 1), N);

        _delay = d;
    }

    void process(float input, float& output) {
        output = _output;

        int k = (_index - _delay) & (N - 1);

        _output = _buffer[k];

        _buffer[_index] = input;
        _index = (_index + 1) & (N - 1);
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output = 0.0f;
    }
};

template<int N>
class Allpass {

    float _buffer[N] {};

    float _output = 0.0f;
    float _coef = 0.5f;

    int _index0 = 0;
    int _index1 = 0;
    int _delay = N;

public:
    void setDelay(int d) {   
        d = MIN(MAX(d, 1), N);

        _index1 = (_index0 - d) & (N - 1);
        _delay = d; 
    }

    int getDelay() {
        return _delay;
    }

    void setCoef(float coef) {
        coef = MIN(MAX(coef, -1.0f), 1.0f);

        _coef = coef;
    }

    void process(float input, float& output) {
        output = _output;

        _output = _buffer[_index1] - _coef * input; // feedforward path
        _buffer[_index0] = input + _coef * _output; // feedback path

        _index0 = (_index0 + 1) & (N - 1);
        _index1 = (_index1 + 1) & (N - 1);
    }

    void getOutput(float& output) {
        output = _output;
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output = 0.0f;
    }
};

class RandomLFO {

    int32_t _y0 = 0;
    int32_t _y1 = 0;
    int32_t _k = 0;

    int32_t _gain = 0;

    uint32_t _rand0 = 0;
    int32_t _q0 = 0;                // prev
    int32_t _r0 = 0;                // next
    int32_t _m0 = _r0/2 - _q0/2;    // slope
    int32_t _b0 = _r0/2 + _q0/2;    // offset

    uint32_t _rand1 = 0;
    int32_t _q1 = 0;
    int32_t _r1 = 0;
    int32_t _m1 = _q1/2 - _r1/2;
    int32_t _b1 = _q1/2 + _r1/2;

public:
    void setFreq(float freq, float sampleRate) {
        freq = MIN(freq, 1/16.0f * sampleRate);
        freq = MAX(freq, 1/16777216.0f * sampleRate);

        double w = PI * (double)freq / (double)sampleRate;

        // amplitude slightly less than 1.0
        _y0 = 0;
        _y1 = (int32_t)(0.999 * cos(w) * FIXQ31);

        _k = (int32_t)(2.0 * sin(w) * FIXQ32);
    }

    void setGain(int32_t gain) {
        gain = MIN(MAX(gain, 0), 0x7fffffff);

        _gain = gain;
    }

    void process(int32_t& lfoSin, int32_t& lfoCos) {
        lfoSin = _y0;
        lfoCos = _y1;

        // "Magic Circle" quadrature oscillator
        _y1 -= MULHI(_k, _y0);
        _y0 += MULHI(_k, _y1);

        // since the oscillators are in quadrature, zero-crossing in one detects a peak in the other
        if ((lfoCos ^ _y1) < 0) {
            //_rand0 = 69069 * _rand0 + 1;
            _rand0 = (11 * _rand0 + 0x04000000) & 0xfc000000;   // periodic version

            _q0 = _r0;
            _r0 = 2 * MULHI((int32_t)_rand0, _gain);    // Q31

            // scale the peak-to-peak segment to traverse from q0 to r0
            _m0 = _r0/2 - _q0/2;  // slope in Q31
            _b0 = _r0/2 + _q0/2;  // offset in Q31

            int32_t sign = _y1 >> 31;
            _m0 = (_m0 ^ sign) - sign;
        }
        if ((lfoSin ^ _y0) < 0) {
            //_rand1 = 69069 * _rand1 + 1;
            _rand1 = (11 * _rand1 + 0x04000000) & 0xfc000000;   // periodic version

            _q1 = _r1;
            _r1 = 2 * MULHI((int32_t)_rand1, _gain);    // Q31

            // scale the peak-to-peak segment to traverse from q1 to r1
            _m1 = _q1/2 - _r1/2;  // slope in Q31
            _b1 = _q1/2 + _r1/2;  // offset in Q31

            int32_t sign = _y0 >> 31;
            _m1 = (_m1 ^ sign) - sign;
        }

        lfoSin = 2 * MULHI(lfoSin, _m0) + _b0;  // Q31
        lfoCos = 2 * MULHI(lfoCos, _m1) + _b1;  // Q31
    }
};

template<int N>
class AllPassMod {

    float _buffer[N] {};

    float _output = 0.0f;
    float _coef = 0.5f;

    int _index = 0;
    int _delay = N;

public:
    void setDelay(int d) {   
        d = MIN(MAX(d, 1), N);

        _delay = d; 
    }

    int getDelay() {
        return _delay;
    }

    void setCoef(float coef) {
        coef = MIN(MAX(coef, -1.0f), 1.0f);

        _coef = coef;
    }

    void process(float input, int32_t mod, float& output) {
        output = _output;

        // add modulation to delay
        int32_t offset = _delay + (mod >> MOD_FRACBITS);
        float frac = (mod & MOD_FRACMASK) * QMOD_TO_FLOAT;

        // 3rd-order Lagrange interpolation
        int k0 = (_index - (offset-1)) & (N - 1);
        int k1 = (_index - (offset+0)) & (N - 1);
        int k2 = (_index - (offset+1)) & (N - 1);
        int k3 = (_index - (offset+2)) & (N - 1);

        float x0 = _buffer[k0];
        float x1 = _buffer[k1];
        float x2 = _buffer[k2];
        float x3 = _buffer[k3];

        // compute the polynomial coefficients
        float c0 = (1/6.0f) * (x3 - x0) + (1/2.0f) * (x1 - x2);
        float c1 = (1/2.0f) * (x0 + x2) - x1;
        float c2 = x2 - (1/3.0f) * x0 - (1/2.0f) * x1 - (1/6.0f) * x3;
        float c3 = x1;

        // compute the polynomial
        float delayMod = ((c0 * frac + c1) * frac + c2) * frac + c3;

        _output = delayMod - _coef * input;         // feedforward path
        _buffer[_index] = input + _coef * _output;  // feedback path

        _index = (_index + 1) & (N - 1);
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output = 0.0f;
    }
};

class LowpassEQ {

    float _buffer[2] {};

    float _output = 0.0f;

    float _b0 = 1.0f;
    float _b1 = 0.0f;
    float _b2 = 0.0f;

public:
    void setFreq(float sampleRate) {
        sampleRate = MIN(MAX(sampleRate, 24000.0f), 48000.0f);

        // two-zero lowpass filter, with zeros at approximately 12khz
        // zero radius is adjusted to match the response from 0..9khz
        _b0 = 0.5f;
        _b1 = 0.5f * sqrtf(2.0f * 12000.0f/(0.5f * sampleRate) - 1.0f);
        _b2 = 0.5f - _b1;
    }

    void process(float input, float& output) {
        output = _output;

        _output = _b0 * input + _b1 * _buffer[0] + _b2 * _buffer[1];
        _buffer[1] = _buffer[0];
        _buffer[0] = input;
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output = 0.0f;
    }
};

class DampingEQ {

    float _buffer[2] {};

    float _output = 0.0f;

    float _b0 = 1.0f;
    float _b1 = 0.0f;
    float _b2 = 0.0f;
    float _a1 = 0.0f;
    float _a2 = 0.0f;

public:
    void setCoef(float dBgain0, float dBgain1, float freq0, float freq1, float sampleRate) {
        dBgain0 = MIN(MAX(dBgain0, -100.0f), 100.0f);
        dBgain1 = MIN(MAX(dBgain1, -100.0f), 100.0f);
        freq0 = MIN(MAX(freq0, 1.0f), 24000.0f);
        freq1 = MIN(MAX(freq1, 1.0f), 24000.0f);

        double coefLo[3], coefHi[3];
        PZShelf(coefLo, TWOPI * freq0 / sampleRate, dBgain0, 0);    // low shelf
        PZShelf(coefHi, TWOPI * freq1 / sampleRate, dBgain1, 1);    // high shelf

        // convolve into a single biquad
        _b0 = (float)(coefLo[0] * coefHi[0]);
        _b1 = (float)(coefLo[0] * coefHi[1] + coefLo[1] * coefHi[0]);
        _b2 = (float)(coefLo[1] * coefHi[1]);
        _a1 = (float)(coefLo[2] + coefHi[2]);
        _a2 = (float)(coefLo[2] * coefHi[2]);
    }

    void process(float input, float& output) {
        output = _output;

        // transposed Direct Form II
        _output    = _b0 * input + _buffer[0];
        _buffer[0] = _b1 * input - _a1 * _output + _buffer[1];
        _buffer[1] = _b2 * input - _a2 * _output;
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output = 0.0f;
    }
};

template<int N>
class MultiTap2 {

    float _buffer[N] {};

    float _output0 = 0.0f;
    float _output1 = 0.0f;

    float _gain0 = 1.0f;
    float _gain1 = 1.0f;

    int _index = 0;
    int _delay0 = N;
    int _delay1 = N;

public:
    void setDelay(int d0, int d1) {
        d0 = MIN(MAX(d0, 1), N);
        d1 = MIN(MAX(d1, 1), N);

        _delay0 = d0;
        _delay1 = d1;
    }

    int getDelay(int k) {
        switch (k) {
            case 0: return _delay0;
            case 1: return _delay1;
            default: return 0;
        }
    }

    void setGain(float g0, float g1) {
        _gain0 = g0;
        _gain1 = g1;
    }

    void process(float input, float& output0, float& output1) {
        output0 = _output0;
        output1 = _output1;

        int k0 = (_index - _delay0) & (N - 1);
        int k1 = (_index - _delay1) & (N - 1);

        _output0 = _gain0 * _buffer[k0];
        _output1 = _gain1 * _buffer[k1];

        _buffer[_index] = input;
        _index = (_index + 1) & (N - 1);
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output0 = 0.0f;
        _output1 = 0.0f;
    }
};

template<int N>
class MultiTap3 {

    float _buffer[N] {};

    float _output0 = 0.0f;
    float _output1 = 0.0f;
    float _output2 = 0.0f;

    float _gain0 = 1.0f;
    float _gain1 = 1.0f;
    float _gain2 = 1.0f;

    int _index = 0;
    int _delay0 = N;
    int _delay1 = N;
    int _delay2 = N;

public:
    void setDelay(int d0, int d2) {
        d0 = MIN(MAX(d0, 1), N);
        d2 = MIN(MAX(d2, 1), N);

        _delay0 = d0;
        _delay1 = d0 - 1;
        _delay2 = d2;
    }

    int getDelay(int k) {
        switch (k) {
            case 0: return _delay0;
            case 1: return _delay1;
            case 2: return _delay2;
            default: return 0;
        }
    }

    void setGain(float g0, float g1, float g2) {
        _gain0 = g0;
        _gain1 = g1;
        _gain2 = g2;
    }

    void process(float input, float& output0, float& output1, float& output2) {
        output0 = _output0;
        output1 = _output1;
        output2 = _output2;

        int k0 = (_index - _delay0) & (N - 1);
        int k1 = (_index - _delay1) & (N - 1);
        int k2 = (_index - _delay2) & (N - 1);

        _output0 = _gain0 * _buffer[k0];
        _output1 = _gain1 * _buffer[k1];
        _output2 = _gain2 * _buffer[k2];

        _buffer[_index] = input;
        _index = (_index + 1) & (N - 1);
    }

    void reset() {
        memset(_buffer, 0, sizeof(_buffer));
        _output0 = 0.0f;
        _output1 = 0.0f;
        _output2 = 0.0f;
    }
};

//
// Stereo Reverb
//
class ReverbImpl {

    // Preprocess
    BandwidthEQ _bw;
    DelayLine<NEXTPOW2(M_PD0)> _dl0;
    DelayLine<NEXTPOW2(M_PD0)> _dl1;

    // Early Left
    float _earlyMix1L = 0.0f;
    float _earlyMix2L = 0.0f;

    MultiTap3<NEXTPOW2(M_MT0)> _mt0;
    Allpass<NEXTPOW2(M_AP0)> _ap0;
    MultiTap3<NEXTPOW2(M_MT1_MAX)> _mt1;
    Allpass<NEXTPOW2(M_AP1)> _ap1;
    Allpass<NEXTPOW2(M_AP2)> _ap2;
    MultiTap2<NEXTPOW2(M_MT2)> _mt2;

    // Early Right
    float _earlyMix1R = 0.0f;
    float _earlyMix2R = 0.0f;

    MultiTap3<NEXTPOW2(M_MT3)> _mt3;
    Allpass<NEXTPOW2(M_AP3)> _ap3;
    MultiTap3<NEXTPOW2(M_MT4_MAX)> _mt4;
    Allpass<NEXTPOW2(M_AP4)> _ap4;
    Allpass<NEXTPOW2(M_AP5)> _ap5;
    MultiTap2<NEXTPOW2(M_MT5)> _mt5;

    RandomLFO _lfo;

    // Late
    Allpass<NEXTPOW2(M_AP6)> _ap6;
    AllPassMod<NEXTPOW2(M_AP7_MAX)> _ap7;
    DampingEQ _eq0;
    MultiTap2<NEXTPOW2(M_MT6_MAX)> _mt6;

    Allpass<NEXTPOW2(M_AP8)> _ap8;
    AllPassMod<NEXTPOW2(M_AP9_MAX)> _ap9;
    DampingEQ _eq1;
    MultiTap2<NEXTPOW2(M_MT7_MAX)> _mt7;

    Allpass<NEXTPOW2(M_AP10)> _ap10;
    Allpass<NEXTPOW2(M_AP11)> _ap11;
    Allpass<NEXTPOW2(M_AP12)> _ap12;
    Allpass<NEXTPOW2(M_AP13)> _ap13;
    MultiTap2<NEXTPOW2(M_MT8_MAX)> _mt8;
    LowpassEQ _lp0;

    Allpass<NEXTPOW2(M_AP14)> _ap14;
    Allpass<NEXTPOW2(M_AP15)> _ap15;
    Allpass<NEXTPOW2(M_AP16)> _ap16;
    Allpass<NEXTPOW2(M_AP17)> _ap17;
    MultiTap2<NEXTPOW2(M_MT9_MAX)> _mt9;
    LowpassEQ _lp1;

    // Output Left
    Allpass<NEXTPOW2(M_AP18)> _ap18;
    Allpass<NEXTPOW2(M_AP19)> _ap19;

    // Output Right
    Allpass<NEXTPOW2(M_AP20)> _ap20;
    Allpass<NEXTPOW2(M_AP21)> _ap21;

    float _earlyGain = 0.0f;
    float _wetDryMix = 0.0f;

public:
    void setParameters(ReverbParameters *p);
    void process(float** inputs, float** outputs, int numFrames);
    void reset();
};

static const short primeTable[] = {
       1,    2,    3,    5,    7,   11,   13,   17,   19,   23,   29,   31,   37,   41,   43, 
      47,   53,   59,   61,   67,   71,   73,   79,   83,   89,   97,  101,  103,  107,  109, 
     113,  127,  131,  137,  139,  149,  151,  157,  163,  167,  173,  179,  181,  191,  193, 
     197,  199,  211,  223,  227,  229,  233,  239,  241,  251,  257,  263,  269,  271,  277, 
     281,  283,  293,  307,  311,  313,  317,  331,  337,  347,  349,  353,  359,  367,  373, 
     379,  383,  389,  397,  401,  409,  419,  421,  431,  433,  439,  443,  449,  457,  461, 
     463,  467,  479,  487,  491,  499,  503,  509,  521,  523,  541,  547,  557,  563,  569, 
     571,  577,  587,  593,  599,  601,  607,  613,  617,  619,  631,  641,  643,  647,  653, 
     659,  661,  673,  677,  683,  691,  701,  709,  719,  727,  733,  739,  743,  751,  757, 
     761,  769,  773,  787,  797,  809,  811,  821,  823,  827,  829,  839,  853,  857,  859, 
     863,  877,  881,  883,  887,  907,  911,  919,  929,  937,  941,  947,  953,  967,  971, 
     977,  983,  991,  997, 1009, 1013, 1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 
    1069, 1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181, 
    1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249, 1259, 1277, 1279, 1283, 1289, 
    1291, 1297, 1301, 1303, 1307, 1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423, 
    1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481, 1483, 1487, 1489, 1493, 1499, 
    1511, 1523, 1531, 1543, 1549, 1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609, 
    1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693, 1697, 1699, 1709, 1721, 1723, 
    1733, 1741, 1747, 1753, 1759, 1777, 1783, 1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861, 
    1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931, 1933, 1949, 1951, 1973, 1979, 
    1987, 1993, 1997, 1999, 2003, 2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083, 
    2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143, 2153, 2161, 2179, 2203, 2207, 
    2213, 2221, 2237, 2239, 2243, 2251, 2267, 2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311, 
    2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, 2381, 2383, 2389, 2393, 2399, 2411, 2417, 
    2423, 2437, 2441, 2447, 2459, 2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551, 
    2557, 2579, 2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657, 2659, 2663, 2671, 2677, 2683, 
    2687, 2689, 2693, 2699, 2707, 2711, 2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777, 
    2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837, 2843, 2851, 2857, 2861, 2879, 2887, 2897, 
    2903, 2909, 2917, 2927, 2939, 2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023, 
    3037, 3041, 3049, 3061, 3067, 3079, 3083, 3089, 3109, 3119, 3121, 3137, 3163, 3167, 3169, 
    3181, 3187, 3191, 3203, 3209, 3217, 3221, 3229, 3251, 3253, 3257, 3259, 3271, 3299, 3301, 
    3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, 3359, 3361, 3371, 3373, 3389, 3391, 3407, 
    3413, 3433, 3449, 3457, 3461, 3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533, 
    3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607, 3613, 3617, 3623, 3631, 3637, 
    3643, 3659, 3671, 3673, 3677, 3691, 3697, 3701, 3709, 3719, 3727, 3733, 3739, 3761, 3767, 
    3769, 3779, 3793, 3797, 3803, 3821, 3823, 3833, 3847, 3851, 3853, 3863, 3877, 3881, 3889, 
    3907, 3911, 3917, 3919, 3923, 3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003, 4007, 4013, 
    4019, 4021, 4027, 4049, 4051, 4057, 4073, 4079, 4091, 4093, 4099, 4111, 4127, 4129, 4133, 
    4139, 4153, 4157, 4159, 4177, 4201, 4211, 4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259, 
    4261, 4271, 4273, 4283, 4289, 4297, 4327, 4337, 4339, 4349, 4357, 4363, 4373, 4391, 4397, 
    4409, 4421, 4423, 4441, 4447, 4451, 4457, 4463, 4481, 4483, 4493, 4507, 4513, 4517, 4519, 
    4523, 4547, 4549, 4561, 4567, 4583, 4591, 4597, 4603, 4621, 4637, 4639, 4643, 4649, 4651, 
    4657, 4663, 4673, 4679, 4691, 4703, 4721, 4723, 4729, 4733, 4751, 4759, 4783, 4787, 4789, 
    4793, 4799, 4801, 4813, 4817, 4831, 4861, 4871, 4877, 4889, 4903, 4909, 4919, 4931, 4933, 
    4937, 4943, 4951, 4957, 4967, 4969, 4973, 4987, 4993, 4999, 5003, 5009, 5011, 5021, 5023, 
    5039, 5051, 5059, 5077, 5081, 5087, 5099, 5101, 5107, 5113, 5119, 5147, 5153, 5167, 5171, 
    5179, 5189, 5197, 5209, 5227, 5231, 5233, 5237, 5261, 5273, 5279, 5281, 5297, 5303, 5309, 
    5323, 5333, 5347, 5351, 5381, 5387, 5393, 5399, 5407, 5413, 5417, 5419, 5431, 5437, 5441, 
    5443, 5449, 5471, 5477, 5479, 5483, 5501, 5503, 5507, 5519, 5521, 5527, 5531, 5557, 5563, 
    5569, 5573, 5581, 5591, 5623, 5639, 5641, 5647, 5651, 5653, 5657, 5659, 5669, 5683, 5689, 
    5693, 5701, 5711, 5717, 5737, 5741, 5743, 5749, 5779, 5783, 5791, 5801, 5807, 5813, 5821, 
    5827, 5839, 5843, 5849, 5851, 5857, 5861, 5867, 5869, 5879, 5881, 5897, 5903, 5923, 5927, 
    5939, 5953, 5981, 5987, 6007, 6011, 6029, 6037, 6043, 6047, 6053, 6067, 6073, 6079, 6089, 
    6091, 6101, 6113, 6121, 6131, 6133, 6143, 6151, 6163, 6173, 6197, 6199, 6203, 6211, 6217, 
    6221, 6229, 6247, 6257, 6263, 6269, 6271, 6277, 6287, 6299, 6301, 6311, 6317, 6323, 6329, 
    6337, 6343, 6353, 6359, 6361, 6367, 6373, 6379, 6389, 6397, 6421, 6427, 6449, 6451, 6469, 
    6473, 6481, 6491, 6521, 6529, 6547, 6551, 6553, 6563, 6569, 6571, 6577, 6581, 6599, 6607, 
    6619, 6637, 6653, 6659, 6661, 6673, 6679, 6689, 6691, 6701, 6703, 6709, 6719, 6733, 6737, 
    6761, 6763, 6779, 6781, 6791, 6793, 6803, 6823, 6827, 6829, 6833, 6841, 6857, 6863, 6869, 
    6871, 6883, 6899, 6907, 6911, 6917, 6947, 6949, 6959, 6961, 6967, 6971, 6977, 6983, 6991, 
    6997, 7001, 7013, 7019, 7027, 7039, 7043, 7057, 7069, 7079, 7103, 7109, 7121, 7127, 7129, 
    7151, 7159, 7177, 7187, 7193, 7207, 7211, 7213, 7219, 7229, 7237, 7243, 7247, 7253, 7283, 
    7297, 7307, 7309, 7321, 7331, 7333, 7349, 7351, 7369, 7393, 7411, 7417, 7433, 7451, 7457, 
    7459, 7477, 7481, 7487, 7489, 7499, 7507, 7517, 7523, 7529, 7537, 7541, 7547, 7549, 7559, 
    7561, 7573, 7577, 7583, 7589, 7591, 7603, 7607, 7621, 7639, 7643, 7649, 7669, 7673, 7681, 
    7687, 7691, 7699, 7703, 7717, 7723, 7727, 7741, 7753, 7757, 7759, 7789, 7793, 7817, 7823, 
    7829, 7841, 7853, 7867, 7873, 7877, 7879, 7883, 7901, 7907, 7919, 7927, 7933, 7937, 7949, 
    7951, 7963, 7993, 8009, 8011, 8017, 8039, 8053, 8059, 8069, 8081, 8087, 8089, 8093, 8101, 
    8111, 8117, 8123, 8147, 8161, 8167, 8171, 8179, 8191,  
};

static int getPrime(int n) {
    int low = 0;
    int high = sizeof(primeTable) / sizeof(primeTable[0]) -  1;

    // clip to table limits
    if (n <= primeTable[low]) {
        return primeTable[low];
    }
    if (n >= primeTable[high]) {
        return primeTable[high];
    }

    // binary search
    while (low <= high) {
        int mid = (low + high) >> 1;

        if (n < primeTable[mid]) {
            high = mid - 1;
        } else if (n > primeTable[mid]) {
            low = mid + 1;
        } else {
            return n;   // found it
        }
    }

    //return primeTable[high];  // lower prime
    //return (n - primeTable[high]) < (primeTable[low] - n) ? primeTable[high] : primeTable[low]; // nearest prime
    return primeTable[low];     // higher prime
}

static int scaleDelay(float delay, float sampleRate) {
    return getPrime((int)(delay * (sampleRate/48000.0f) + 0.5f));
}

//
// Piecewise-linear lookup tables
// input clamped to [0.0f, 100.0f]
//
static const float earlyMix0Table[][2] = {
    {0.0000f, 0.6000f},
   {63.3333f, 0.0800f},
   {83.3333f, 0.0200f},
   {93.3333f, 0.0048f},
  {100.0000f, 0.0048f},
};

static const float earlyMix1Table[][2] = {
    {0.0000f, 0.3360f},
   {20.0000f, 0.6000f},
  {100.0000f, 0.0240f},
};

static const float earlyMix2Table[][2] = {
    {0.0000f, 0.0480f},
   {13.3333f, 0.0960f},
   {53.3333f, 0.9600f},
  {100.0000f, 0.1200f},
};

static const float lateMix0Table[][2] = {
    {0.0000f, 0.1250f},
   {13.3333f, 0.1875f},
   {66.6666f, 0.7500f},
  {100.0000f, 0.8750f},
};

static const float lateMix1Table[][2] = {
    {0.0000f, 0.9990f},
   {33.3333f, 0.5000f},
   {66.6666f, 0.9990f},
   {93.3333f, 0.6000f},
  {100.0000f, 0.6000f},
};

static const float lateMix2Table[][2] = {
    {0.0000f, 0.9990f},
   {33.3333f, 0.9990f},
   {63.3333f, 0.4500f},
  {100.0000f, 0.9990f},
};

static const float diffusionCoefTable[][2] = {
    {0.0000f, 0.0000f},
   {20.0000f, 0.0470f},
   {33.3333f, 0.0938f},
   {46.6666f, 0.1563f},
   {60.0000f, 0.2344f},
   {73.3333f, 0.3125f},
   {93.3333f, 0.5000f},
  {100.0000f, PHI},
};

static const float roomSizeTable[][2] = {
    {0.0000f, 0.1500f},
   {25.0000f, 0.3000f},
   {50.0000f, 0.5000f},
  {100.0000f, 1.0000f},
};

static float interpolateTable(const float table[][2], float x) {
    x = MIN(MAX(x, 0.0f), 100.0f);

    // locate the segment in the table
    int i = 0;
    while (x > table[i+1][0]) {
        i++;
    }

    // linear interpolate
    float frac = (x - table[i+0][0]) / (table[i+1][0] - table[i+0][0]);
    return table[i+0][1] + frac * (table[i+1][1] - table[i+0][1]);
}

void ReverbImpl::setParameters(ReverbParameters *p) {

    float sampleRate = MIN(MAX(p->sampleRate, 24000.0f), 48000.0f);

    // Bandwidth
    _bw.setFreq(p->bandwidth, sampleRate);

    // Modulation
    _lfo.setFreq(p->modRate, sampleRate);
    _lfo.setGain((int32_t)MIN(MAX((double)p->modDepth * (1/100.0) * FIXQ31, 0.0), (double)0x7fffffff));

    //
    // Set delays
    //
    int preDelay = (int)(p->preDelay * (1/1000.0f) * sampleRate + 0.5f);
    preDelay = MIN(MAX(preDelay, 1), M_PD0);
    _dl0.setDelay(preDelay);
    _dl1.setDelay(preDelay);

    // RoomSize scalefactor
    float roomSize = interpolateTable(roomSizeTable, p->roomSize);

    // Density scalefactors
    float density0 = p->density * (1/25.0f) - 0.0f;
    float density1 = p->density * (1/25.0f) - 1.0f;
    float density2 = p->density * (1/25.0f) - 2.0f;
    float density3 = p->density * (1/25.0f) - 3.0f;
    density0 = MIN(MAX(density0, 0.0f), 1.0f);
    density1 = MIN(MAX(density1, 0.0f), 1.0f);
    density2 = MIN(MAX(density2, 0.0f), 1.0f);
    density3 = MIN(MAX(density3, 0.0f), 1.0f);

    // Early delays
    _ap0.setDelay(scaleDelay(M_AP0 * 1.0f, sampleRate));
    _ap1.setDelay(scaleDelay(M_AP1 * 1.0f, sampleRate));
    _ap2.setDelay(scaleDelay(M_AP2 * 1.0f, sampleRate));
    _ap3.setDelay(scaleDelay(M_AP3 * 1.0f, sampleRate));
    _ap4.setDelay(scaleDelay(M_AP4 * 1.0f, sampleRate));
    _ap5.setDelay(scaleDelay(M_AP5 * 1.0f, sampleRate));

    _mt0.setDelay(scaleDelay(M_MT0 * roomSize, sampleRate), 1);
    _mt1.setDelay(scaleDelay(M_MT1 * roomSize, sampleRate), scaleDelay(M_MT1_2 * 1.0f, sampleRate));
    _mt2.setDelay(scaleDelay(M_MT2 * roomSize, sampleRate), 1);
    _mt3.setDelay(scaleDelay(M_MT3 * roomSize, sampleRate), 1);
    _mt4.setDelay(scaleDelay(M_MT4 * roomSize, sampleRate), scaleDelay(M_MT4_2 * 1.0f, sampleRate));
    _mt5.setDelay(scaleDelay(M_MT5 * roomSize, sampleRate), 1);

    // Late delays
    _ap6.setDelay(scaleDelay(M_AP6 * roomSize * density3, sampleRate));
    _ap7.setDelay(scaleDelay(M_AP7 * roomSize, sampleRate));
    _ap8.setDelay(scaleDelay(M_AP8 * roomSize * density3, sampleRate));
    _ap9.setDelay(scaleDelay(M_AP9 * roomSize, sampleRate));
    _ap10.setDelay(scaleDelay(M_AP10 * roomSize * density1, sampleRate));
    _ap11.setDelay(scaleDelay(M_AP11 * roomSize * density2, sampleRate));
    _ap12.setDelay(scaleDelay(M_AP12 * roomSize, sampleRate));
    _ap13.setDelay(scaleDelay(M_AP13 * roomSize * density3, sampleRate));
    _ap14.setDelay(scaleDelay(M_AP14 * roomSize * density1, sampleRate));
    _ap15.setDelay(scaleDelay(M_AP15 * roomSize * density2, sampleRate));
    _ap16.setDelay(scaleDelay(M_AP16 * roomSize * density3, sampleRate));
    _ap17.setDelay(scaleDelay(M_AP17 * roomSize * density3, sampleRate));

    int lateDelay = scaleDelay(p->lateDelay * (1/1000.0f) * 48000, sampleRate);
    lateDelay = MIN(MAX(lateDelay, 1), M_LD0);

    _mt6.setDelay(scaleDelay(M_MT6 * roomSize * density3, sampleRate), lateDelay);
    _mt7.setDelay(scaleDelay(M_MT7 * roomSize * density2, sampleRate), lateDelay);
    _mt8.setDelay(scaleDelay(M_MT8 * roomSize * density0, sampleRate), lateDelay);
    _mt9.setDelay(scaleDelay(M_MT9 * roomSize, sampleRate), lateDelay);

    // Output delays
    _ap18.setDelay(scaleDelay(M_AP18 * 1.0f, sampleRate));
    _ap19.setDelay(scaleDelay(M_AP19 * 1.0f, sampleRate));
    _ap20.setDelay(scaleDelay(M_AP20 * 1.0f, sampleRate));
    _ap21.setDelay(scaleDelay(M_AP21 * 1.0f, sampleRate));

    // RT60 is determined by mean delay of feedback paths
    int loopDelay;
    loopDelay = _ap6.getDelay();
    loopDelay += _ap7.getDelay();
    loopDelay += _ap8.getDelay();
    loopDelay += _ap9.getDelay();
    loopDelay += _ap10.getDelay();
    loopDelay += _ap11.getDelay();
    loopDelay += _ap12.getDelay();
    loopDelay += _ap13.getDelay();
    loopDelay += _ap14.getDelay();
    loopDelay += _ap15.getDelay();
    loopDelay += _ap16.getDelay();
    loopDelay += _ap17.getDelay();
    loopDelay += _mt6.getDelay(0);
    loopDelay += _mt7.getDelay(0);
    loopDelay += _mt8.getDelay(0);
    loopDelay += _mt9.getDelay(0);
    loopDelay /= 2;

    //
    // Set gains
    //
    float rt60 = MIN(MAX(p->reverbTime, 0.01f), 100.0f);
    float rt60Gain = -60.0f * loopDelay / (rt60 * sampleRate);  // feedback gain (dB) for desired RT

    float bassMult = MIN(MAX(p->bassMult, 0.1f), 10.0f);
    float bassGain = (1.0f / bassMult - 1.0f) * rt60Gain;       // filter gain (dB) that results in RT *= mult

    float loopGain1 = sqrtf(0.5f * dBToGain(rt60Gain));         // distributed (series-parallel) loop gain
    float loopGain2 = 0.9f - (0.63f * loopGain1);

    // Damping
    _eq0.setCoef(bassGain, p->highGain, p->bassFreq, p->highFreq, sampleRate);
    _eq1.setCoef(bassGain, p->highGain, p->bassFreq, p->highFreq, sampleRate);
    _lp0.setFreq(sampleRate);
    _lp1.setFreq(sampleRate);

    float earlyDiffusionCoef = interpolateTable(diffusionCoefTable, p->earlyDiffusion);

    // Early Left
    _earlyMix1L = interpolateTable(earlyMix1Table, p->earlyMixRight);
    _earlyMix2L = interpolateTable(earlyMix2Table, p->earlyMixLeft);

    _mt0.setGain(0.2f, 0.4f, interpolateTable(earlyMix0Table, p->earlyMixLeft));

    _ap0.setCoef(earlyDiffusionCoef);
    _ap1.setCoef(earlyDiffusionCoef);
    _ap2.setCoef(earlyDiffusionCoef);

    _mt1.setGain(0.2f, 0.6f, interpolateTable(lateMix0Table, p->lateMixLeft) * 0.125f);

    _mt2.setGain(interpolateTable(lateMix1Table, p->lateMixLeft) * loopGain2, 
                 interpolateTable(lateMix2Table, p->lateMixLeft) * loopGain2);

    // Early Right
    _earlyMix1R = interpolateTable(earlyMix1Table, p->earlyMixLeft);
    _earlyMix2R = interpolateTable(earlyMix2Table, p->earlyMixRight);

    _mt3.setGain(0.2f, 0.4f, interpolateTable(earlyMix0Table, p->earlyMixRight));

    _ap3.setCoef(earlyDiffusionCoef);
    _ap4.setCoef(earlyDiffusionCoef);
    _ap5.setCoef(earlyDiffusionCoef);

    _mt4.setGain(0.2f, 0.6f, interpolateTable(lateMix0Table, p->lateMixRight) * 0.125f);

    _mt5.setGain(interpolateTable(lateMix1Table, p->lateMixRight) * loopGain2, 
                 interpolateTable(lateMix2Table, p->lateMixRight) * loopGain2);

    _earlyGain = dBToGain(p->earlyGain);

    // Late
    float lateDiffusionCoef = interpolateTable(diffusionCoefTable, p->lateDiffusion);
    _ap6.setCoef(lateDiffusionCoef);
    _ap7.setCoef(lateDiffusionCoef);
    _ap8.setCoef(lateDiffusionCoef);
    _ap9.setCoef(lateDiffusionCoef);

    _ap10.setCoef(PHI);
    _ap11.setCoef(PHI);
    _ap12.setCoef(lateDiffusionCoef);
    _ap13.setCoef(lateDiffusionCoef);

    _ap14.setCoef(PHI);
    _ap15.setCoef(PHI);
    _ap16.setCoef(lateDiffusionCoef);
    _ap17.setCoef(lateDiffusionCoef);

    float lateGain = dBToGain(p->lateGain) * 2.0f;
    _mt6.setGain(loopGain1, lateGain * interpolateTable(lateMix0Table, p->lateMixLeft));
    _mt7.setGain(loopGain1, lateGain * interpolateTable(lateMix0Table, p->lateMixRight));
    _mt8.setGain(loopGain1, lateGain * interpolateTable(lateMix2Table, p->lateMixLeft) * loopGain2 * 0.125f);
    _mt9.setGain(loopGain1, lateGain * interpolateTable(lateMix2Table, p->lateMixRight) * loopGain2 * 0.125f);

    // Output
    float outputDiffusionCoef = lateDiffusionCoef * 0.6f;
    _ap18.setCoef(outputDiffusionCoef);
    _ap19.setCoef(outputDiffusionCoef);
    _ap20.setCoef(outputDiffusionCoef);
    _ap21.setCoef(outputDiffusionCoef);

    _wetDryMix = p->wetDryMix * (1/100.0f);
    _wetDryMix = MIN(MAX(_wetDryMix, 0.0f), 1.0f);
}

void ReverbImpl::process(float** inputs, float** outputs, int numFrames) {

    for (int i = 0; i < numFrames; i++) {
        float x0, x1, y0, y1, y2, y3;

        // Preprocess
        x0 = inputs[0][i];
        x1 = inputs[1][i];
        _bw.process(x0, x1, x0, x1);

        float preL, preR;
        _dl0.process(x0, preL);
        _dl1.process(x1, preR);

        // Early Left
        float early0L, early1L, early2L, earlyOutL;
        _mt0.process(preL, x0, x1, y0);
        _ap0.process(x0 + x1, y1);
        _mt1.process(y1, x0, x1, early0L);
        _ap1.process(x0 + x1, y2);
        _ap2.process(y2, x0);
        _mt2.process(x0, early1L, early2L);

        earlyOutL = (y0 + y1 * _earlyMix1L + y2 * _earlyMix2L) * _earlyGain;

        // Early Right
        float early0R, early1R, early2R, earlyOutR;
        _mt3.process(preR, x0, x1, y0);
        _ap3.process(x0 + x1, y1);
        _mt4.process(y1, x0, x1, early0R);
        _ap4.process(x0 + x1, y2);
        _ap5.process(y2, x0);
        _mt5.process(x0, early1R, early2R);

        earlyOutR = (y0 + y1 * _earlyMix1R + y2 * _earlyMix2R) * _earlyGain;

        // LFO update
        int32_t lfoSin, lfoCos;
        _lfo.process(lfoSin, lfoCos);

        // Late
        float lateOut0;
        _ap6.getOutput(x0);
        _ap7.process(x0, lfoSin, x0);
        _eq0.process(-early0L + x0, x0);
        _mt6.process(x0, y0, lateOut0);

        float lateOut1;
        _ap8.getOutput(x0);
        _ap9.process(x0, lfoCos, x0);
        _eq1.process(-early0R + x0, x0);
        _mt7.process(x0, y1, lateOut1);

        float lateOut2;
        _ap10.getOutput(x0);
        _ap11.process(-early2L + x0, x0);
        _ap12.process(x0, x0);
        _ap13.process(-early2L - x0, x0);
        _mt8.process(-early0L + x0, x0, lateOut2);
        _lp0.process(x0, y2);

        float lateOut3;
        _ap14.getOutput(x0);
        _ap15.process(-early2R + x0, x0);
        _ap16.process(x0, x0);
        _ap17.process(-early2R - x0, x0);
        _mt9.process(-early0R + x0, x0, lateOut3);
        _lp1.process(x0, y3);

        // Feedback matrix
        _ap6.process(early1L + y2 - y3, x0);
        _ap8.process(early1R - y2 - y3, x0);
        _ap10.process(-early2R + y0 + y1, x0);
        _ap14.process(-early2L - y0 + y1, x0);

        // Output Left
        _ap18.process(-earlyOutL + lateOut0 + lateOut3, x0);
        _ap19.process(x0, y0);

        // Output Right
        _ap20.process(-earlyOutR + lateOut1 + lateOut2, x1);
        _ap21.process(x1, y1);

        x0 = inputs[0][i];
        x1 = inputs[1][i];
        outputs[0][i] = x0 + (y0 - x0) * _wetDryMix;
        outputs[1][i] = x1 + (y1 - x1) * _wetDryMix;
    }
}

// clear internal state, but retain settings
void ReverbImpl::reset() {

    _bw.reset();

    _dl0.reset();
    _dl1.reset();

    _mt0.reset();
    _mt1.reset();
    _mt2.reset();
    _mt3.reset();
    _mt4.reset();
    _mt5.reset();
    _mt6.reset();
    _mt7.reset();
    _mt8.reset();
    _mt9.reset();

    _ap0.reset();
    _ap1.reset();
    _ap2.reset();
    _ap3.reset();
    _ap4.reset();
    _ap5.reset();
    _ap6.reset();
    _ap7.reset();
    _ap8.reset();
    _ap9.reset();
    _ap10.reset();
    _ap11.reset();
    _ap12.reset();
    _ap13.reset();
    _ap14.reset();
    _ap15.reset();
    _ap16.reset();
    _ap17.reset();
    _ap18.reset();
    _ap19.reset();
    _ap20.reset();
    _ap21.reset();

    _eq0.reset();
    _eq1.reset();

    _lp0.reset();
    _lp1.reset();
}

//
// Public API
//

static const int REVERB_BLOCK = 256;

AudioReverb::AudioReverb(float sampleRate) {

    _impl = new ReverbImpl;

    // format conversion buffers
    _inout[0] = new float[REVERB_BLOCK];
    _inout[1] = new float[REVERB_BLOCK];

    // default parameters
    ReverbParameters p;
    p.sampleRate = sampleRate;
    p.bandwidth = 10000.0f;

    p.preDelay = 20.0f;
    p.lateDelay = 0.0f;

    p.reverbTime = 2.0f;

    p.earlyDiffusion = 100.0f;
    p.lateDiffusion = 100.0f;

    p.roomSize = 50.0f;
    p.density = 100.0f;

    p.bassMult = 1.5f;
    p.bassFreq = 250.0f;
    p.highGain = -6.0f;
    p.highFreq = 3000.0f;

    p.modRate = 2.3f;
    p.modDepth = 50.0f;

    p.earlyGain = 0.0f;
    p.lateGain = 0.0f;

    p.earlyMixLeft = 20.0f;
    p.earlyMixRight = 20.0f;
    p.lateMixLeft = 90.0f;
    p.lateMixRight = 90.0f;

    p.wetDryMix = 100.0f;

    setParameters(&p);
}

AudioReverb::~AudioReverb() {
    delete _impl;

    delete[] _inout[0];
    delete[] _inout[1];
}

void AudioReverb::setParameters(ReverbParameters *p) {
    _params = *p;
    _impl->setParameters(p);
};

void AudioReverb::getParameters(ReverbParameters *p) {
    *p = _params;
};

void AudioReverb::reset() {
    _impl->reset();
}

void AudioReverb::render(float** inputs, float** outputs, int numFrames) {
    _impl->process(inputs, outputs, numFrames);
}

//
// on x86 architecture, assume that SSE2 is present
//
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)

#include <emmintrin.h>

// convert int16_t to float, deinterleave stereo
void AudioReverb::convertInput(const int16_t* input, float** outputs, int numFrames) {
    __m128 scale = _mm_set1_ps(1/32768.0f);

    int i = 0;
    for (; i < numFrames - 3; i += 4) {
        __m128i a0 = _mm_loadu_si128((__m128i*)&input[2*i]);
        __m128i a1 = a0;

        // deinterleave and sign-extend
        a0 = _mm_madd_epi16(a0, _mm_set1_epi32(0x00000001));
        a1 = _mm_madd_epi16(a1, _mm_set1_epi32(0x00010000));

        __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);
        __m128 f1 = _mm_mul_ps(_mm_cvtepi32_ps(a1), scale);

        _mm_storeu_ps(&outputs[0][i], f0);
        _mm_storeu_ps(&outputs[1][i], f1);
    }
    for (; i < numFrames; i++) {
        __m128i a0 = _mm_cvtsi32_si128(*(int32_t*)&input[2*i]);
        __m128i a1 = a0;

        // deinterleave and sign-extend
        a0 = _mm_madd_epi16(a0, _mm_set1_epi32(0x00000001));
        a1 = _mm_madd_epi16(a1, _mm_set1_epi32(0x00010000));

        __m128 f0 = _mm_mul_ps(_mm_cvtepi32_ps(a0), scale);
        __m128 f1 = _mm_mul_ps(_mm_cvtepi32_ps(a1), scale);

        _mm_store_ss(&outputs[0][i], f0);
        _mm_store_ss(&outputs[1][i], f1);
    }
}

// fast TPDF dither in [-1.0f, 1.0f]
static inline __m128 dither4() {
    static __m128i rz;

    // update the 8 different maximum-length LCGs
    rz = _mm_mullo_epi16(rz, _mm_set_epi16(25173, -25511, -5975, -23279, 19445, -27591, 30185, -3495));
    rz = _mm_add_epi16(rz, _mm_set_epi16(13849, -32767, 105, -19675, -7701, -32679, -13225, 28013));

    // promote to 32-bit
    __m128i r0 = _mm_unpacklo_epi16(rz, _mm_setzero_si128());
    __m128i r1 = _mm_unpackhi_epi16(rz, _mm_setzero_si128());

    // return (r0 - r1) * (1/65536.0f);
    __m128 d0 = _mm_cvtepi32_ps(_mm_sub_epi32(r0, r1));
    return _mm_mul_ps(d0, _mm_set1_ps(1/65536.0f));
}

// convert float to int16_t with dither, interleave stereo
void AudioReverb::convertOutput(float** inputs, int16_t* output, int numFrames) {
    __m128 scale = _mm_set1_ps(32768.0f);

    int i = 0;
    for (; i < numFrames - 3; i += 4) {
        __m128 f0 = _mm_mul_ps(_mm_loadu_ps(&inputs[0][i]), scale);
        __m128 f1 = _mm_mul_ps(_mm_loadu_ps(&inputs[1][i]), scale);

        __m128 d0 = dither4();
        f0 = _mm_add_ps(f0, d0);
        f1 = _mm_add_ps(f1, d0);

        // round and saturate
        __m128i a0 = _mm_cvtps_epi32(f0);
        __m128i a1 = _mm_cvtps_epi32(f1);
        a0 = _mm_packs_epi32(a0, a0);
        a1 = _mm_packs_epi32(a1, a1);

        // interleave
        a0 = _mm_unpacklo_epi16(a0, a1);
        _mm_storeu_si128((__m128i*)&output[2*i], a0);
    }
    for (; i < numFrames; i++) {
        __m128 f0 = _mm_mul_ps(_mm_load_ss(&inputs[0][i]), scale);
        __m128 f1 = _mm_mul_ps(_mm_load_ss(&inputs[1][i]), scale);

        __m128 d0 = dither4();
        f0 = _mm_add_ps(f0, d0);
        f1 = _mm_add_ps(f1, d0);

        // round and saturate
        __m128i a0 = _mm_cvtps_epi32(f0);
        __m128i a1 = _mm_cvtps_epi32(f1);
        a0 = _mm_packs_epi32(a0, a0);
        a1 = _mm_packs_epi32(a1, a1);

        // interleave
        a0 = _mm_unpacklo_epi16(a0, a1);
        *(int32_t*)&output[2*i] = _mm_cvtsi128_si32(a0);
    }
}

// deinterleave stereo
void AudioReverb::convertInput(const float* input, float** outputs, int numFrames) {

    int i = 0;
    for (; i < numFrames - 3; i += 4) {
        __m128 f0 = _mm_loadu_ps(&input[2*i + 0]);
        __m128 f1 = _mm_loadu_ps(&input[2*i + 4]);

        // deinterleave
        _mm_storeu_ps(&outputs[0][i], _mm_shuffle_ps(f0, f1, _MM_SHUFFLE(2,0,2,0)));
        _mm_storeu_ps(&outputs[1][i], _mm_shuffle_ps(f0, f1, _MM_SHUFFLE(3,1,3,1)));
    }
    for (; i < numFrames; i++) {
        // deinterleave
        outputs[0][i] = input[2*i + 0];
        outputs[1][i] = input[2*i + 1];
    }
}

// interleave stereo
void AudioReverb::convertOutput(float** inputs, float* output, int numFrames) {

    int i = 0;
    for(; i < numFrames - 3; i += 4) {
        __m128 f0 = _mm_loadu_ps(&inputs[0][i]);
        __m128 f1 = _mm_loadu_ps(&inputs[1][i]);

        // interleave
        _mm_storeu_ps(&output[2*i + 0],_mm_unpacklo_ps(f0,f1));
        _mm_storeu_ps(&output[2*i + 4],_mm_unpackhi_ps(f0,f1));
    }
    for(; i < numFrames; i++) {
        // interleave
        output[2*i + 0] = inputs[0][i];
        output[2*i + 1] = inputs[1][i];
    }
}

#else

// convert int16_t to float, deinterleave stereo
void AudioReverb::convertInput(const int16_t* input, float** outputs, int numFrames) {
    const float scale = 1/32768.0f;

    for (int i = 0; i < numFrames; i++) {
        outputs[0][i] = (float)input[2*i + 0] * scale;
        outputs[1][i] = (float)input[2*i + 1] * scale;
    }
}

// fast TPDF dither in [-1.0f, 1.0f]
static inline float dither() {
    static uint32_t rz = 0;
    rz = rz * 69069 + 1;
    int32_t r0 = rz & 0xffff;
    int32_t r1 = rz >> 16;
    return (int32_t)(r0 - r1) * (1/65536.0f);
}

// convert float to int16_t with dither, interleave stereo
void AudioReverb::convertOutput(float** inputs, int16_t* output, int numFrames) {
    const float scale = 32768.0f;

    for (int i = 0; i < numFrames; i++) {

        float f0 = inputs[0][i] * scale;
        float f1 = inputs[1][i] * scale;

        float d = dither();
        f0 += d;
        f1 += d;

        // round and saturate
        f0 += (f0 < 0.0f ? -0.5f : +0.5f);
        f1 += (f1 < 0.0f ? -0.5f : +0.5f);
        f0 = MIN(MAX(f0, -32768.0f), 32767.0f);
        f1 = MIN(MAX(f1, -32768.0f), 32767.0f);

        // interleave
        output[2*i + 0] = (int16_t)f0;
        output[2*i + 1] = (int16_t)f1;
    }
}

// deinterleave stereo
void AudioReverb::convertInput(const float* input, float** outputs, int numFrames) {

    for (int i = 0; i < numFrames; i++) {
        // deinterleave
        outputs[0][i] = input[2*i + 0];
        outputs[1][i] = input[2*i + 1];
    }
}

// interleave stereo
void AudioReverb::convertOutput(float** inputs, float* output, int numFrames) {

    for (int i = 0; i < numFrames; i++) {
        // interleave
        output[2*i + 0] = inputs[0][i];
        output[2*i + 1] = inputs[1][i];
    }
}

#endif

//
// This version handles input/output as interleaved int16_t
//
void AudioReverb::render(const int16_t* input, int16_t* output, int numFrames) {

    while (numFrames) {

        int n = MIN(numFrames, REVERB_BLOCK);

        convertInput(input, _inout, n);

        _impl->process(_inout, _inout, n);

        convertOutput(_inout, output, n);

        input += 2 * n;
        output += 2 * n;
        numFrames -= n;
    }
}

//
// This version handles input/output as interleaved float
//
void AudioReverb::render(const float* input, float* output, int numFrames) {

    while (numFrames) {

        int n = MIN(numFrames, REVERB_BLOCK);

        convertInput(input, _inout, n);

        _impl->process(_inout, _inout, n);

        convertOutput(_inout, output, n);

        input += 2 * n;
        output += 2 * n;
        numFrames -= n;
    }
}
