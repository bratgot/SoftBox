// slrRectLight.cpp - stochastic rectangular area light for ScanlineRender2.
//
// Nuke ships SLR light shaders for DiskLight, SphereLight, DistantLight and
// DomeLight only. A UsdLux RectLight prim declares light:shaderId "RectLight",
// so ScanlineRender2 looks for a plugin named slrRectLight - this one.
//
// Soft shadows come from sampling the light surface the way a path tracer
// does: every illuminate() call picks fresh points on the rectangle from the
// renderer's per-subpixel sampling RNG, so the penumbra converges as the
// render's camera samples go up, instead of summing N hard-shadow lights.
//
// Optional prim attribute:
//   int softbox:shadowSamples  shadow rays per shade (default 1). >1 fires a
//                              jittered stratified grid per shade to trade
//                              render time for less noise at low camera
//                              sample counts.

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4244 4267 4305 4251 4996)
#endif

#include "DDImage/SlrLightShader.h"
#include "DDImage/SlrShadingContext.h"
#include "DDImage/SlrRayContext.h"
#include "DDImage/Pixel.h"
#include "usg/lux/RectLightPrim.h"
#include "ndk/geo/render/SamplingRng.h"

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>

namespace {

struct V3 {
    float x, y, z;
};
inline V3    v3(const fdk::Vec3f& v)            { return {v.x, v.y, v.z}; }
inline V3    sub(const V3& a, const V3& b)      { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3    add(const V3& a, const V3& b)      { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3    mul(const V3& a, float s)          { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(const V3& a, const V3& b)      { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float len(const V3& a)                   { return std::sqrt(dot(a, a)); }
inline fdk::Vec3f fv(const V3& a)               { return fdk::Vec3f(a.x, a.y, a.z); }

// Shading-space frame of the rectangle (lights and Ps() share the renderer's camera-centred space) at one time: centre, half-extent edge
// vectors (local X * width/2, local Y * height/2) and emission normal (-Z).
struct RectFrame {
    V3    c, u, v, n;
};

class SoftRectLight : public slr::SlrLightShader
{
public:
    static slr::SlrShader* build(slr::SlrEngineContext* slrtx) { return new SoftRectLight(slrtx); }

    explicit SoftRectLight(slr::SlrEngineContext* slrtx)
        : SlrLightShader(slrtx, kInputs, kOutputs)
    {
    }

    const char* shaderClass() const override { return "RectLight"; }
    // ScanlineRender2's default integrator only calls illuminate() for lights
    // that report as delta lights, so report true and do the area sampling
    // inside illuminate().
    bool        isDeltaLight() const override { return true; }

    void _updateLightPropertiesAt(const slr::SlrEngineContext& /*slrtx*/,
                                  const usg::Prim&             lightPrim,
                                  const fdk::TimeValue&        time) override
    {
        usg::lux::RectLightPrim rect(lightPrim);
        if (rect.validate()) {
            _width     = rect.getWidth(time);
            _height    = rect.getHeight(time);
            _normalize = rect.getNormalize(time);
            const BaseInputParams p(rect.getColor(time), rect.getIntensity(time), rect.getExposure(time));
            _emission = p.illumColor();
            if (rect.getEnableColorTemperature(time)) {
                const fdk::Vec3f k = ColorTemperatureAsRgb(rect.getColorTemperature(time));
                _emission = fdk::Vec3f(_emission.x * k.x, _emission.y * k.y, _emission.z * k.z);
            }
        }
        _samples = 1;
        usg::Attribute a = lightPrim.getAttr(usg::Token("softbox:shadowSamples"));
        if (a.isValid()) {
            int32_t s = 1;
            if (a.getValue(s, time) > 0)
                _samples = std::max(1, std::min(s, 1024));
        }
    }

    void _validateShader() override
    {
        SlrLightShader::_validateShader();
        // Match slrDiskLight: the shader sets _color itself, and an un-normalized
        // area light's power grows with its area; normalize keeps total power
        // fixed as the light is resized.
        const float a = _normalize ? 1.0f : std::max(_width * _height, 1e-8f);
        _color[DD::Image::Chan_Red]   = _emission.x * a;
        _color[DD::Image::Chan_Green] = _emission.y * a;
        _color[DD::Image::Chan_Blue]  = _emission.z * a;
    }

    bool _validateLightEnabledState() override
    {
        return _color[DD::Image::Chan_Red]   > 0.0f ||
               _color[DD::Image::Chan_Green] > 0.0f ||
               _color[DD::Image::Chan_Blue]  > 0.0f;
    }

    bool illuminate(const slr::SlrShadingContext&      stx,
                    const slr::SlrPrimEnabledLighting& primLightingInfo,
                    const fdk::Vec3f&                  /*lobeN*/,
                    slr::SlrRayContext&                illumRayOut,
                    float&                             directPdfWeightOut,
                    fdk::Vec3f&                        colorOut,
                    fdk::Vec3f&                        shadowTransmissionFactorOut,
                    DD::Image::Pixel&                  surfaceShaderColorAOVs) const override
    {
        if (numMotionXforms() == 0)
            return false;

        const fdk::TimeValue t = stx.time();
        const RectFrame      f = frameAt(t);
        const V3             P = v3(stx.Ps());

        // Surface point must be in front of the emitting side.
        const V3 toC = sub(f.c, P);
        if (dot(f.n, toC) >= 0.0f)
            return false;

        ndk::SamplingRng& rng = stx.samplingRng();
        const int   grid = std::max(1, (int)std::ceil(std::sqrt((float)_samples)));
        const int   n    = grid * grid;
        const float cell = 2.0f / (float)grid;

        float sumW = 0.0f;                  // unshadowed geometric weight
        V3    sumWT = {0.0f, 0.0f, 0.0f};   // shadowed geometric weight per channel
        V3    firstDir = {0.0f, 0.0f, 0.0f};
        float firstDist = 0.0f;

        for (int j = 0; j < grid; ++j) {
            for (int i = 0; i < grid; ++i) {
                // Stratified cell, jittered by the per-subpixel RNG: [-1,1]^2
                const float su = -1.0f + ((float)i + rng.newFloatValue()) * cell;
                const float sv = -1.0f + ((float)j + rng.newFloatValue()) * cell;
                const V3 Q   = add(f.c, add(mul(f.u, su), mul(f.v, sv)));
                const V3 d   = sub(Q, P);
                const float dist = len(d);
                if (dist < 1e-6f)
                    continue;
                const V3 L = mul(d, 1.0f / dist);
                const float cosL = -dot(f.n, L);   // emission cosine at the light
                if (cosL <= 0.0f)
                    continue;
                const float w = cosL / (dist * dist);   // area-light estimator: inverse-square, emitter cosine

                illumRayOut.set(stx.Ps(), fv(L), ndk::RayContext::shadowRay(),
                                t, 0, 1e-4f, dist * (1.0f - 1e-4f));
                fdk::Vec3f T(1.0f);
                getShadowTransmissionFactor(stx, primLightingInfo, illumRayOut, T);

                sumW += w;
                sumWT = add(sumWT, {w * T.x, w * T.y, w * T.z});
                if (firstDist == 0.0f) { firstDir = L; firstDist = dist; }
            }
        }
        if (sumW <= 0.0f)
            return false;

        // One sample: the ray to that sample (exact). Several: point the ray at
        // the centre for the surface's N.L / specular, carry the averaged
        // visibility in the transmission factor.
        if (n == 1) {
            illumRayOut.set(stx.Ps(), fv(firstDir), ndk::RayContext::shadowRay(),
                            t, 0, 1e-4f, firstDist * (1.0f - 1e-4f));
        } else {
            const float dc = len(toC);
            illumRayOut.set(stx.Ps(), fv(mul(toC, 1.0f / dc)), ndk::RayContext::shadowRay(),
                            t, 0, 1e-4f, dc * (1.0f - 1e-4f));
        }

        colorOut.set(_color[DD::Image::Chan_Red],
                     _color[DD::Image::Chan_Green],
                     _color[DD::Image::Chan_Blue]);
        directPdfWeightOut = sumW / (float)n;
        const float inv = 1.0f / sumW;
        shadowTransmissionFactorOut.set(sumWT.x * inv, sumWT.y * inv, sumWT.z * inv);

        if (outputShadowAOVs())
            copyShadowsToOutputColorAOVs(shadowTransmissionFactorOut, surfaceShaderColorAOVs);
        return true;
    }

private:
    RectFrame frameAt(const fdk::TimeValue& t) const
    {
        const fdk::Mat4d xf = (numMotionXforms() > 1) ? getMotionXformAt(t) : getMotionXform(0);
        const double* m = xf.array();   // column-major
        const V3 X = {(float)m[0], (float)m[1], (float)m[2]};
        const V3 Y = {(float)m[4], (float)m[5], (float)m[6]};
        const V3 Z = {(float)m[8], (float)m[9], (float)m[10]};
        RectFrame f;
        f.c = {(float)m[12], (float)m[13], (float)m[14]};
        f.u = mul(X, 0.5f * _width);
        f.v = mul(Y, 0.5f * _height);
        const float zl = std::max(len(Z), 1e-12f);
        f.n = mul(Z, -1.0f / zl);
        return f;
    }

    float _width     = 1.0f;
    float _height    = 1.0f;
    bool  _normalize = false;
    int   _samples   = 1;
    fdk::Vec3f _emission{1.0f, 1.0f, 1.0f};

    static const InputKnobList  kInputs;
    static const OutputPortList kOutputs;
};

// Same input layout as Nuke's slrDiskLight, with the RectLight's width/height
// in place of radius; the renderer rejects a shader with no inputs/outputs.
const slr::SlrShader::InputKnobList SoftRectLight::kInputs = {
    {"xform",                  slr::SlrShader::MAT4_KNOB,   true},
    {"color",                  slr::SlrShader::COLOR3_KNOB, true},
    {"intensity",              slr::SlrShader::FLOAT_KNOB,  true},
    {"colorTemperature",       slr::SlrShader::FLOAT_KNOB,  true},
    {"enableColorTemperature", slr::SlrShader::INT_KNOB,    true},
    {"exposure",               slr::SlrShader::FLOAT_KNOB,  true},
    {"normalize",              slr::SlrShader::INT_KNOB,    true},
    {"diffuse",                slr::SlrShader::FLOAT_KNOB,  true},
    {"specular",               slr::SlrShader::FLOAT_KNOB,  true},
    {"width",                  slr::SlrShader::FLOAT_KNOB,  true},
    {"height",                 slr::SlrShader::FLOAT_KNOB,  true},
};
const slr::SlrShader::OutputPortList SoftRectLight::kOutputs = {
    {"result", slr::SlrShader::PIXEL_KNOB, nullptr},
};

} // namespace

static const slr::SlrShader::Description sRectLightDesc("RectLight", SoftRectLight::build);
