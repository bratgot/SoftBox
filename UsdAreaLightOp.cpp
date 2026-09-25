#include "UsdAreaLightOp.h"
#include <DDImage/Knob.h>
#include <cmath>
#include <sstream>

const DD::Image::GeomOp::Description GeoAreaLightOp::description(CLASS, build);

GeoAreaLightOp::GeoAreaLightOp(Node* node)
    : DD::Image::GeomOp(node, BuildEngine<Engine>())
{
}

void GeoAreaLightOp::knobs(DD::Image::Knob_Callback f)
{
    GeomOp::knobs(f);

    DD::Image::Tab_knob(f, "Transform");
    DD::Image::Axis_knob(f, &_xform, "transform");
    KnobModifiesAttribValues(f);

    DD::Image::Tab_knob(f, "Area Light");
    DD::Image::Float_knob(f, &_width,  DD::Image::IRange(0.001, 100.0), "width",  "Width");
    DD::Image::Tooltip(f, "Width of the light surface in scene units.");
    KnobDefinesGeometry(f);
    DD::Image::Float_knob(f, &_height, DD::Image::IRange(0.001, 100.0), "height", "Height");
    DD::Image::Tooltip(f, "Height of the light surface in scene units.");
    KnobDefinesGeometry(f);

    DD::Image::Spacer(f, 10);

    DD::Image::Float_knob(f, &_intensity, DD::Image::IRange(0.0, 100.0), "intensity", "Intensity");
    DD::Image::Tooltip(f, "Base brightness of the light.\n"
                          "In Grid mode this is divided equally across all sub-lights, "
                          "so total illumination stays constant regardless of Shadow Samples.");
    KnobModifiesAttribValues(f);
    DD::Image::Float_knob(f, &_exposure, DD::Image::IRange(-10.0, 10.0), "exposure", "Exposure");
    DD::Image::Tooltip(f, "Exposure bias in stops. Final intensity = intensity * 2^exposure.");
    KnobModifiesAttribValues(f);
    DD::Image::Color_knob(f, _color, "color", "Color");
    DD::Image::Tooltip(f, "Light color. Multiplied with intensity.");
    KnobModifiesAttribValues(f);
    DD::Image::Bool_knob(f, &_normalize, "normalize", "Normalize");
    DD::Image::Tooltip(f, "Scale intensity inversely with light area so brightness stays "
                          "consistent when resizing the light.");
    KnobModifiesAttribValues(f);

    DD::Image::Tab_knob(f, "Shadow");
    static const char* const kShadowModes[] = {"Stochastic", "Grid", nullptr};
    DD::Image::Enumeration_knob(f, &_shadowMode, kShadowModes, "shadow_mode", "Shadow Mode");
    DD::Image::Tooltip(f, "Stochastic (default): one USD RectLight, shaded by SoftBox's slrRectLight "
                          "shader. Shadow rays go to random points on the light surface, like a path "
                          "tracer, so the penumbra is a smooth gradient that converges with more "
                          "samples.\n\n"
                          "Grid: the v1.0/v1.1 method, a grid of hard-shadow DiskLights. The "
                          "penumbra is built from overlapping copies of the shadow and can band.");
    KnobDefinesGeometry(f);
    DD::Image::Int_knob(f, &_shadowSamples, "shadow_samples", "Shadow Samples");
    DD::Image::Tooltip(f, "Stochastic: shadow rays per shading point, spread over the light in a "
                          "jittered grid. 1 = noisy (let ScanlineRender2 camera samples clean it "
                          "up), 16 = smooth preview, 64 = final quality.\n\n"
                          "Grid: number of sub-lights (rounded up to a square grid). Render time "
                          "scales linearly with it.\n\n"
                          "Either way, shadow softness comes from the light's Width and Height; "
                          "samples only control smoothness.");
    KnobDefinesGeometry(f);

    DD::Image::Tab_knob(f, "USD");
    DD::Image::String_knob(f, &_primPath, "usd_prim_path", "Prim Path");
    DD::Image::Tooltip(f, "USD stage path for the light prim.\n"
                          "Stochastic writes one RectLight here. Grid with more than one sample\n"
                          "writes DiskLight children (e.g. /Lights/AreaLight/sub_0_0).");
    KnobDefinesGeometry(f);

    DD::Image::Tab_knob(f, "About");
    static const char* const kAbout[] = {
        "<b><font size='5'>AreaLight</font></b>",
        "<font color='#888'>Soft shadow area light for ScanlineRender2</font>",
        " ",
        "SoftBox v" SOFTBOX_VERSION,
        "Created by <b>Marten Blumen</b>",
        " ",
        "<b>How it works</b>",
        "Nuke's own lights send one shadow ray to the light's centre, so",
        "ScanlineRender2 shadows are hard. In Stochastic mode this node writes",
        "a USD RectLight, and SoftBox's slrRectLight shader sends each shadow",
        "ray to a random point on the light's surface instead. The penumbra",
        "comes out as a smooth, physically based gradient, the way a path",
        "tracer renders it, from one light.",
        " ",
        "Grid mode keeps the v1.0 method: a grid of hard-shadow DiskLights",
        "whose overlapping shadows fake the penumbra.",
        " ",
        "<b>Tips</b>",
        "- Shadow softness comes from the light's Width and Height",
        "- Shadow Samples controls smoothness, not penumbra width",
        "- 16 samples for previews, 64 for finals and close-ups",
        "- At low samples, raising the render's camera samples also",
        "  smooths the shadow (and anti-aliases everything else)",
        "- Stochastic is faster than Grid for the same sample count",
        nullptr};
    for (const char* const* line = kAbout; *line; ++line) {
        DD::Image::Text_knob(f, *line);
        DD::Image::SetFlags(f, DD::Image::Knob::STARTLINE);
    }
}

int GeoAreaLightOp::knob_changed(DD::Image::Knob* k)
{
    return GeomOp::knob_changed(k);
}

void GeoAreaLightOp::build_handles(DD::Image::ViewerContext* ctx)
{
    GeomOp::build_handles(ctx);
    build_knob_handles(ctx);
    add_draw_handle(ctx);
}

void GeoAreaLightOp::draw_handle(DD::Image::ViewerContext* ctx)
{
    if (!ctx) return;
    const float hw = _width  * 0.5f;
    const float hh = _height * 0.5f;

    glPushMatrix();
    glMultMatrixd(_xform.array());

    // Rectangle outline
    glColor3f(1.0f, 0.55f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex3f(-hw, -hh, 0.f); glVertex3f( hw, -hh, 0.f);
        glVertex3f( hw,  hh, 0.f); glVertex3f(-hw,  hh, 0.f);
    glEnd();

    // Centre crosshair
    glBegin(GL_LINES);
        glVertex3f(-hw, 0.f, 0.f); glVertex3f(hw,  0.f, 0.f);
        glVertex3f(0.f, -hh, 0.f); glVertex3f(0.f, hh,  0.f);
    glEnd();

    // Direction arrow
    glColor3f(1.0f, 1.0f, 0.0f);
    const float arrowLen = std::max(_width, _height) * 0.6f;
    const float tipZ  = -arrowLen;
    const float headL =  arrowLen * 0.2f;
    glBegin(GL_LINES);
        glVertex3f(0.f, 0.f, 0.f);  glVertex3f(0.f, 0.f, tipZ);
        glVertex3f(0.f, 0.f, tipZ); glVertex3f( headL, 0.f,   tipZ + headL);
        glVertex3f(0.f, 0.f, tipZ); glVertex3f(-headL, 0.f,   tipZ + headL);
        glVertex3f(0.f, 0.f, tipZ); glVertex3f(0.f,    headL, tipZ + headL);
        glVertex3f(0.f, 0.f, tipZ); glVertex3f(0.f,   -headL, tipZ + headL);
    glEnd();

    // Centre dot (light icon)
    glColor3f(1.0f, 0.9f, 0.3f);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
        glVertex3f(0.f, 0.f, 0.f);
    glEnd();

    glPopMatrix();
}

// =========================================================================
// Engine
// =========================================================================

GeoAreaLightOp::Engine::Engine(ndk::GeomOpNode* parent)
    : DD::Image::GeomOpEngine(parent)
    , _defineTarget(usg::Token("light-define"))
    , _modifyTarget(usg::Token("light-modify"))
{
    assignDefineGeometryTarget(_defineTarget);
    assignModifyValuesTarget(_modifyTarget);
}

void GeoAreaLightOp::Engine::processScenegraph(usg::GeomSceneContext& context)
{
    DD::Image::GeomOpEngine::processScenegraph(context);

    if (inputEngine0()) {
        buildStageFromInput(context, 0);
        mergeFromInputs(context, {0});
    }

    if (context.doGeometryProcessing()) {
        writeLightPrims(context);
    }
}

void GeoAreaLightOp::Engine::writeLightPrims(usg::GeomSceneContext& context)
{
    if (!_editLayer) return;

    auto getFloat = [&](const char* name, float def) -> float {
        if (DD::Image::Knob* k = firstOp()->knob(name))
            return static_cast<float>(k->get_value());
        return def;
    };
    auto getBool = [&](const char* name, bool def) -> bool {
        if (DD::Image::Knob* k = firstOp()->knob(name))
            return k->get_value() != 0.0;
        return def;
    };

    const float width     = getFloat("width",     Defaults::WIDTH);
    const float height    = getFloat("height",    Defaults::HEIGHT);
    const float intensity = getFloat("intensity", Defaults::INTENSITY);
    const float exposure  = getFloat("exposure",  Defaults::EXPOSURE);
    const bool  norm      = getBool ("normalize", Defaults::NORMALIZE);

    int shadowSamples = [&]() -> int {
        if (DD::Image::Knob* k = firstOp()->knob("shadow_samples"))
            return static_cast<int>(k->get_value());
        return Defaults::SHADOW_SAMPLES;
    }();

    fdk::Vec3f color(1.f, 1.f, 1.f);
    if (DD::Image::Knob* ck = firstOp()->knob("color")) {
        color.x = static_cast<float>(ck->get_value(0));
        color.y = static_cast<float>(ck->get_value(1));
        color.z = static_cast<float>(ck->get_value(2));
    }

    // Read full transform from Axis_knob -- includes translate, rotate, scale, pivot, etc.
    const GeoAreaLightOp* op = static_cast<const GeoAreaLightOp*>(firstOp());
    fdk::Mat4d xform = op->_xform;

    std::string primPathStr = "/Lights/AreaLight";
    if (DD::Image::Knob* pk = firstOp()->knob("usd_prim_path")) {
        const char* t = pk->get_text();
        if (t && t[0]) primPathStr = t;
    }

    const int shadowMode = [&]() -> int {
        if (DD::Image::Knob* k = firstOp()->knob("shadow_mode"))
            return static_cast<int>(k->get_value());
        return Defaults::SHADOW_MODE;
    }();

    if (shadowMode == 0) {
        auto slash = primPathStr.rfind('/');
        if (slash != std::string::npos && slash > 0)
            _editLayer->defineScopeParents(usg::Path(primPathStr.substr(0, slash)));
        writeRectLight(context, usg::Path(primPathStr), width, height, intensity, exposure,
                       norm, color, xform, std::max(1, shadowSamples));
        return;
    }

    const float radius = std::max(width, height) * 0.5f;

    // =====================================================================
    // Soft shadow grid
    // =====================================================================
    if (shadowSamples < 1) shadowSamples = 1;
    const int gridN      = std::max(1, (int)std::ceil(std::sqrt((float)shadowSamples)));
    const int totalLights = gridN * gridN;
    const float subIntensity = intensity / (float)totalLights;

    // Extract axes from the matrix (column-major):
    //   col0 = local X,  col1 = local Y,  col2 = local Z,  col3 = translation
    const double* m = xform.array();

    // Create parent scope prims
    {
        auto slash = primPathStr.rfind('/');
        if (slash != std::string::npos && slash > 0) {
            _editLayer->defineScopeParents(usg::Path(primPathStr.substr(0, slash)));
        }
    }
    if (totalLights > 1) {
        _editLayer->defineScopeParents(usg::Path(primPathStr));
    }

    for (int jj = 0; jj < gridN; ++jj) {
        for (int ii = 0; ii < gridN; ++ii) {

            std::string subPath;
            if (totalLights == 1) {
                subPath = primPathStr;
            } else {
                std::ostringstream oss;
                oss << primPathStr << "/sub_" << ii << "_" << jj;
                subPath = oss.str();
            }
            const usg::Path path(subPath);

            // Local-space offset on the rect surface (XY plane)
            const double localX = (double(ii) - double(gridN - 1) * 0.5)
                                * (double(width) / double(gridN));
            const double localY = (double(jj) - double(gridN - 1) * 0.5)
                                * (double(height) / double(gridN));

            // Build per-sub-light matrix: start with the full xform,
            // then offset translation along local X and Y axes
            fdk::Mat4d subXform = xform;
            double* sm = subXform.array();
            // col3 (translation) += localX * col0 + localY * col1
            sm[12] += m[0] * localX + m[4] * localY;
            sm[13] += m[1] * localX + m[5] * localY;
            sm[14] += m[2] * localX + m[6] * localY;

            // Define pass
            if (context.defineGeometry()) {
                usg::lux::DiskLightPrim diskLight =
                    usg::lux::DiskLightPrim::defineInLayer(_editLayer, path);
                if (!diskLight.validate()) continue;

                diskLight.createRadiusAttr();
                diskLight.createColorAttr();
                diskLight.createIntensityAttr();
                diskLight.createExposureAttr();
                diskLight.createNormalizeAttr();

                usg::lux::ShadowAPI shadowApi(diskLight);
                shadowApi.apply();
                shadowApi.createShadowEnableAttr();

                // Single matrix xformOp instead of decomposed translate/rotate/scale
                diskLight.createXformOpOrderAttr();
                diskLight.createIntrinsicAttr(usg::Token("xformOp:transform"), usg::Value::Matrix4d, usg::TimeVariability::Varying);
                usg::TokenArray xformOrder;
                xformOrder.push_back(usg::Token("xformOp:transform"));
                diskLight.setXformOpOrder(xformOrder);
            }

            // Value pass
            usg::lux::DiskLightPrim diskLight =
                usg::lux::DiskLightPrim::defineInLayer(_editLayer, path);
            if (!diskLight.validate()) continue;

            diskLight.setRadius(radius);
            diskLight.setColor(color);
            diskLight.setIntensity(subIntensity);
            diskLight.setExposure(exposure);
            diskLight.setNormalize(norm);

            usg::lux::ShadowAPI shadowApi(diskLight);
            shadowApi.setShadowEnable(true);

            diskLight.setAttr(usg::Token("xformOp:transform"), subXform);
        }
    }
}

void GeoAreaLightOp::Engine::writeRectLight(usg::GeomSceneContext& context, const usg::Path& path,
                                            float width, float height, float intensity, float exposure,
                                            bool normalize, const fdk::Vec3f& color,
                                            const fdk::Mat4d& xform, int shadowSamples)
{
    if (context.defineGeometry()) {
        usg::lux::RectLightPrim rect = usg::lux::RectLightPrim::defineInLayer(_editLayer, path);
        if (!rect.validate()) return;

        rect.createWidthAttr();
        rect.createHeightAttr();
        rect.createColorAttr();
        rect.createIntensityAttr();
        rect.createExposureAttr();
        rect.createNormalizeAttr();

        usg::lux::ShadowAPI shadowApi(rect);
        shadowApi.apply();
        shadowApi.createShadowEnableAttr();

        rect.createIntrinsicAttr(usg::Token("softbox:shadowSamples"), usg::Value::Int, usg::TimeVariability::Varying);

        rect.createXformOpOrderAttr();
        rect.createIntrinsicAttr(usg::Token("xformOp:transform"), usg::Value::Matrix4d, usg::TimeVariability::Varying);
        usg::TokenArray xformOrder;
        xformOrder.push_back(usg::Token("xformOp:transform"));
        rect.setXformOpOrder(xformOrder);
    }

    usg::lux::RectLightPrim rect = usg::lux::RectLightPrim::defineInLayer(_editLayer, path);
    if (!rect.validate()) return;

    rect.setWidth(width);
    rect.setHeight(height);
    rect.setColor(color);
    rect.setIntensity(intensity);
    rect.setExposure(exposure);
    rect.setNormalize(normalize);

    usg::lux::ShadowAPI shadowApi(rect);
    shadowApi.setShadowEnable(true);

    rect.setAttr(usg::Token("softbox:shadowSamples"), int32_t(shadowSamples));
    rect.setAttr(usg::Token("xformOp:transform"), xform);
}
