#pragma once

class Node;

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4244 4305 4251 4800 4996)
#endif

#include <DDImage/GeomOp.h>
#include <DDImage/Knobs.h>
#include <DDImage/ViewerContext.h>
#include <DDImage/gl.h>

#include <usg/engine/GeomEngine.h>
#include <usg/engine/GeomSceneContext.h>
#include <usg/geom/Layer.h>
#include <usg/geom/Path.h>
#include <usg/lux/DiskLightPrim.h>
#include <usg/lux/RectLightPrim.h>
#include <usg/lux/ShadowAPI.h>
#include <usg/geom/XformCommonAPI.h>
#include <usg/base/Token.h>
#include <fdk/math/Vec3.h>
#include <fdk/math/Mat4.h>

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

#include <string>

static const char* const CLASS = "AreaLight";

namespace Defaults {
    constexpr float  WIDTH             = 1.0f;
    constexpr float  HEIGHT            = 1.0f;
    constexpr float  INTENSITY         = 1.0f;
    constexpr float  EXPOSURE          = 0.0f;
    constexpr float  COLOR[3]          = {1.f, 1.f, 1.f};
    constexpr bool   NORMALIZE         = true;
    constexpr int    SHADOW_SAMPLES    = 1;
    constexpr int    SHADOW_MODE       = 0;   // 0 = stochastic RectLight, 1 = grid
}

class GeoAreaLightOp : public DD::Image::GeomOp
{
public:
    class Engine : public DD::Image::GeomOpEngine
    {
    public:
        Engine(ndk::GeomOpNode* parent);
    protected:
        void processScenegraph(usg::GeomSceneContext& context) override;
    private:
        void writeLightPrims(usg::GeomSceneContext& context);
        void writeRectLight(usg::GeomSceneContext& context, const usg::Path& path,
                            float width, float height, float intensity, float exposure,
                            bool normalize, const fdk::Vec3f& color,
                            const fdk::Mat4d& xform, int shadowSamples);
        usg::GeomStateTarget _defineTarget;
        usg::GeomStateTarget _modifyTarget;
    };

    static const DD::Image::GeomOp::Description description;
    static DD::Image::Op* build(Node* node) { return new GeoAreaLightOp(node); }

    explicit GeoAreaLightOp(Node* node);

    const char* Class()     const override { return CLASS; }
    const char* node_help() const override {
        return "Area light with soft shadows for ScanlineRender2.\n\n"
               "Stochastic mode (default) writes a USD RectLight whose shadow rays are sampled "
               "across the light surface by SoftBox's slrRectLight shader, giving smooth, "
               "physically based penumbras. Grid mode uses the older grid of hard-shadow "
               "DiskLights.";
    }

    int minimum_inputs() const override { return 1; }
    int maximum_inputs() const override { return 1; }
    const char* input_label(int n, char*) const override { return "scene"; }

    void knobs(DD::Image::Knob_Callback f) override;
    int  knob_changed(DD::Image::Knob* k) override;

    void build_handles(DD::Image::ViewerContext* ctx) override;
    void draw_handle(DD::Image::ViewerContext* ctx) override;

    // Transform
    fdk::Mat4d _xform{fdk::Mat4d::getIdentity()};

    // Light
    float  _width    = Defaults::WIDTH;
    float  _height   = Defaults::HEIGHT;
    float  _intensity= Defaults::INTENSITY;
    float  _exposure = Defaults::EXPOSURE;
    double _color[3] = {Defaults::COLOR[0], Defaults::COLOR[1], Defaults::COLOR[2]};
    bool   _normalize = Defaults::NORMALIZE;

    // Shadow
    int    _shadowSamples = Defaults::SHADOW_SAMPLES;
    int    _shadowMode    = Defaults::SHADOW_MODE;

    // USD
    std::string _primPath = "/Lights/AreaLight";
};
