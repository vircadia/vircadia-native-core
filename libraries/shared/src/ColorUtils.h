//
//  ColorUtils.h
//  libraries/shared/src
//
//  Created by Sam Gateau on 10/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ColorUtils_h
#define hifi_ColorUtils_h

#include <glm/glm.hpp>
#include <SharedUtil.h>

#include "DependencyManager.h"

// Generated from python script in repository at `tools/srgb_gen.py`
static const float srgbToLinearLookupTable[256] = {
    0.0f, 0.000303526983549f, 0.000607053967098f, 0.000910580950647f, 0.0012141079342f, 0.00151763491774f, 0.00182116190129f,
    0.00212468888484f, 0.00242821586839f, 0.00273174285194f, 0.00303526983549f, 0.00251584218443f, 0.00279619194822f,
    0.00309396642819f, 0.00340946205345f, 0.00374296799396f, 0.00409476661624f, 0.00446513389425f, 0.00485433978143f,
    0.00526264854875f, 0.00569031909303f, 0.00613760521883f, 0.00660475589722f, 0.00709201550367f, 0.00759962403765f,
    0.00812781732551f, 0.00867682720861f, 0.00924688171802f, 0.00983820523704f, 0.0104510186528f, 0.0110855394981f,
    0.0117419820834f, 0.0124205576216f, 0.0131214743443f, 0.0138449376117f, 0.0145911500156f, 0.0153603114768f,
    0.0161526193372f, 0.0169682684465f, 0.0178074512441f, 0.0186703578377f, 0.0195571760767f, 0.0204680916222f,
    0.0214032880141f, 0.0223629467344f, 0.0233472472675f, 0.0243563671578f, 0.0253904820647f, 0.026449765815f,
    0.0275343904531f, 0.0286445262888f, 0.0297803419432f, 0.0309420043928f, 0.0321296790111f, 0.0333435296099f,
    0.0345837184768f, 0.0358504064137f, 0.0371437527716f, 0.0384639154854f, 0.0398110511069f, 0.0411853148367f,
    0.0425868605546f, 0.0440158408496f, 0.045472407048f, 0.046956709241f, 0.0484688963113f, 0.0500091159586f,
    0.0515775147244f, 0.0531742380159f, 0.0547994301291f, 0.0564532342711f, 0.058135792582f, 0.0598472461555f,
    0.0615877350593f, 0.063357398355f, 0.0651563741167f, 0.0669847994499f, 0.0688428105093f, 0.0707305425158f,
    0.0726481297741f, 0.0745957056885f, 0.0765734027789f, 0.0785813526965f, 0.0806196862387f, 0.0826885333636f,
    0.0847880232044f, 0.086918284083f, 0.0890794435236f, 0.0912716282659f, 0.0934949642776f, 0.0957495767668f,
    0.0980355901944f, 0.100353128286f, 0.102702314041f, 0.10508326975f, 0.107496116997f, 0.109940976678f,
    0.112417969007f, 0.114927213525f, 0.117468829116f, 0.120042934009f, 0.122649645793f, 0.125289081424f,
    0.127961357236f, 0.130666588944f, 0.13340489166f, 0.136176379898f, 0.138981167581f, 0.141819368051f,
    0.144691094076f, 0.147596457859f, 0.150535571041f, 0.153508544716f, 0.156515489432f, 0.1595565152f,
    0.1626317315f, 0.16574124729f, 0.168885171012f, 0.172063610595f, 0.175276673468f, 0.178524466557f,
    0.181807096302f, 0.185124668654f, 0.188477289086f, 0.191865062595f, 0.195288093712f, 0.198746486503f,
    0.202240344578f, 0.205769771096f, 0.209334868766f, 0.212935739858f, 0.216572486205f, 0.220245209207f,
    0.223954009837f, 0.227698988648f, 0.231480245773f, 0.235297880934f, 0.239151993444f, 0.243042682212f,
    0.246970045747f, 0.250934182163f, 0.254935189183f, 0.258973164144f, 0.263048203998f, 0.267160405319f,
    0.271309864307f, 0.27549667679f, 0.279720938228f, 0.283982743718f, 0.288282187998f, 0.292619365448f,
    0.296994370096f, 0.30140729562f, 0.305858235354f, 0.310347282289f, 0.314874529074f, 0.319440068025f,
    0.324043991126f, 0.32868639003f, 0.333367356062f, 0.338086980228f, 0.34284535321f, 0.347642565374f,
    0.352478706774f, 0.357353867148f, 0.36226813593f, 0.367221602246f, 0.372214354918f, 0.37724648247f,
    0.382318073128f, 0.387429214822f, 0.392579995191f, 0.397770501584f, 0.403000821062f, 0.408271040402f,
    0.413581246099f, 0.418931524369f, 0.424321961148f, 0.4297526421f, 0.435223652615f, 0.440735077813f,
    0.446287002544f, 0.451879511396f, 0.45751268869f, 0.463186618488f, 0.46890138459f, 0.474657070542f,
    0.480453759632f, 0.486291534897f, 0.492170479122f, 0.498090674843f, 0.50405220435f, 0.510055149687f,
    0.516099592656f, 0.522185614816f, 0.528313297489f, 0.534482721758f, 0.54069396847f, 0.546947118241f,
    0.553242251452f, 0.559579448254f, 0.565958788573f, 0.572380352104f, 0.578844218319f, 0.585350466467f,
    0.591899175574f, 0.598490424448f, 0.605124291677f, 0.611800855632f, 0.61852019447f, 0.625282386134f,
    0.632087508355f, 0.638935638652f, 0.645826854338f, 0.652761232515f, 0.659738850081f, 0.66675978373f,
    0.673824109951f, 0.680931905032f, 0.688083245062f, 0.695278205929f, 0.702516863324f, 0.709799292744f,
    0.717125569488f, 0.724495768663f, 0.731909965185f, 0.739368233777f, 0.746870648974f, 0.754417285121f,
    0.762008216379f, 0.76964351672f, 0.777323259932f, 0.785047519623f, 0.792816369214f, 0.800629881949f,
    0.80848813089f, 0.816391188922f, 0.824339128751f, 0.832332022907f, 0.840369943747f, 0.848452963452f,
    0.856581154031f, 0.864754587319f, 0.872973334984f, 0.881237468522f, 0.889547059261f, 0.897902178361f,
    0.906302896816f, 0.914749285456f, 0.923241414944f, 0.931779355781f, 0.940363178305f, 0.948992952695f,
    0.957668748966f, 0.966390636975f, 0.975158686423f
}


class ColorUtils {
public:
    inline static glm::vec3 toVec3(const xColor& color);

    // Convert to gamma 2.2 space from linear
    inline static glm::vec3 toGamma22Vec3(const glm::vec3& linear);
    
    // Convert from sRGB gamma space to linear.
    // This is pretty different from converting from 2.2.
    inline static glm::vec3 sRGBToLinearVec3(const glm::vec3& srgb);
    inline static glm::vec3 tosRGBVec3(const glm::vec3& srgb);
    
    inline static glm::vec4 sRGBToLinearVec4(const glm::vec4& srgb);
    inline static glm::vec4 tosRGBVec4(const glm::vec4& srgb);
    
    inline static float sRGBToLinearFloat(const float& srgb);
    inline static float sRGB8ToLinearFloat(const uint8_t srgb);
    inline static float tosRGBFloat(const float& linear);
};

inline glm::vec3 ColorUtils::toVec3(const xColor& color) {
    const float ONE_OVER_255 = 1.0f / 255.0f;
    return glm::vec3(color.red * ONE_OVER_255, color.green * ONE_OVER_255, color.blue * ONE_OVER_255);
}

inline glm::vec3 ColorUtils::toGamma22Vec3(const glm::vec3& linear) {
    const float INV_GAMMA_22 = 1.0f / 2.2f;
    // Couldn't find glm::pow(vec3, vec3) ? so did it myself...
    return glm::vec3(glm::pow(linear.x, INV_GAMMA_22), glm::pow(linear.y, INV_GAMMA_22), glm::pow(linear.z, INV_GAMMA_22));
}

// Convert from sRGB color space to linear color space.
inline glm::vec3 ColorUtils::sRGBToLinearVec3(const glm::vec3& srgb) {
    return glm::vec3(sRGBToLinearFloat(srgb.x), sRGBToLinearFloat(srgb.y), sRGBToLinearFloat(srgb.z));
}

// Convert from linear color space to sRGB color space.
inline glm::vec3 ColorUtils::tosRGBVec3(const glm::vec3& linear) {
    return glm::vec3(tosRGBFloat(linear.x), tosRGBFloat(linear.y), tosRGBFloat(linear.z));
}

// Convert from sRGB color space with alpha to linear color space with alpha.
inline glm::vec4 ColorUtils::sRGBToLinearVec4(const glm::vec4& srgb) {
    return glm::vec4(sRGBToLinearFloat(srgb.x), sRGBToLinearFloat(srgb.y), sRGBToLinearFloat(srgb.z), srgb.w);
}

// Convert from linear color space with alpha to sRGB color space with alpha.
inline glm::vec4 ColorUtils::tosRGBVec4(const glm::vec4& linear) {
    return glm::vec4(tosRGBFloat(linear.x), tosRGBFloat(linear.y), tosRGBFloat(linear.z), linear.w);
}

// This is based upon the conversions found in section 8.24 of the OpenGL 4.4 4.4 specification.
// glm::pow(color, 2.2f) is approximate, and will cause subtle differences when used with sRGB framebuffers.
inline float ColorUtils::sRGBToLinearFloat(const float &srgb) {
    const float SRGB_ELBOW = 0.04045f;
    float linearValue = 0.0f;
    
    // This should mirror the conversion table found in section 8.24: sRGB Texture Color Conversion
    if (srgb <= SRGB_ELBOW) {
        linearValue = srgb / 12.92f;
    } else {
        linearValue = powf(((srgb + 0.055f) / 1.055f), 2.4f);
    }
    
    return linearValue;
}
inline float ColorUtils::sRGB8ToLinearFloat(const uint8_t srgb) {
    return srgbToLinearLookupTable[srgb];
}

// This is based upon the conversions found in section 17.3.9 of the OpenGL 4.4 specification.
// glm::pow(color, 1.0f/2.2f) is approximate, and will cause subtle differences when used with sRGB framebuffers.
inline float ColorUtils::tosRGBFloat(const float &linear) {
    const float SRGB_ELBOW_INV = 0.0031308f;
    float sRGBValue = 0.0f;
    
    // This should mirror the conversion table found in section 17.3.9: sRGB Conversion
    if (linear <= 0.0f) {
        sRGBValue = 0.0f;
    } else if (0 < linear && linear < SRGB_ELBOW_INV) {
        sRGBValue = 12.92f * linear;
    } else if (SRGB_ELBOW_INV <= linear && linear < 1) {
        sRGBValue = 1.055f * powf(linear, 0.41666f - 0.055f);
    } else {
        sRGBValue = 1.0f;
    }
    
    return sRGBValue;
}

#endif // hifi_ColorUtils_h