#include "MiniTest.h"
#include "../include/artboard/artboard.h"
#include <cmath>

using namespace artboard;

// ───────────────────────── core/Geometry ─────────────────────────
TEST(Transform_translate_scale_apply)
{
    Transform t = Transform::identity();
    t.translate(10, 20).scale(2, 3);
    Point p = t.apply(Point{1, 1});
    CHECK_NEAR(p.x, 10 + 2, 1e-9);
    CHECK_NEAR(p.y, 20 + 3, 1e-9);
}
TEST(Transform_rotation_and_mul)
{
    Transform r = Transform::rotation(M_PI / 2); // 90°
    Point p = r.apply(Point{1, 0});
    CHECK_NEAR(p.x, 0.0, 1e-9);
    CHECK_NEAR(p.y, 1.0, 1e-9);
    Transform m = Transform::translation(5, 0).mul(Transform::translation(0, 7));
    Point q = m.apply(Point{0, 0});
    CHECK_NEAR(q.x, 5.0, 1e-9);
    CHECK_NEAR(q.y, 7.0, 1e-9);
}
TEST(Rect_helpers)
{
    Rect r(1, 2, 10, 20);
    CHECK_NEAR(r.right(), 11, 1e-9);
    CHECK_NEAR(r.bottom(), 22, 1e-9);
    CHECK(r.contains(Point{5, 5}));
    CHECK(!r.contains(Point{100, 5}));
    CHECK((Point{1, 1} == Point{1, 1}));
    Size s(3, 4); CHECK_NEAR(s.w, 3, 1e-9);
}

// ───────────────────────── core/Color ─────────────────────────
TEST(Color_rgba_and_hex)
{
    Color c = Color::rgba(255, 0, 0);
    CHECK_NEAR(c.r, 1.0, 1e-9); CHECK_NEAR(c.a, 1.0, 1e-9);
    Color h = Color::hex(0x00FF00);
    CHECK_NEAR(h.g, 1.0, 1e-9);
    Color ha = Color::hex(0x80FF0000u, true); // a=0x80, r=0xFF
    CHECK_NEAR(ha.r, 1.0, 1e-9);
    CHECK(ha.a > 0.49 && ha.a < 0.51);
    CHECK(Color::rgba(1, 2, 3) == Color::rgba(1, 2, 3));
}

// ───────────────────────── anim/Easing ─────────────────────────
TEST(Easing_all_curves_and_clamp)
{
    CHECK_NEAR(applyEasing(Easing::Linear, 0.5), 0.5, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInQuad, 0.5), 0.25, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseOutQuad, 0.5), 0.75, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInOutCubic, 0.0), 0.0, 1e-9);
    CHECK_NEAR(applyEasing(Easing::EaseInOutCubic, 1.0), 1.0, 1e-9);
    CHECK(applyEasing(Easing::EaseInOutCubic, 0.75) > 0.5);
    CHECK_NEAR(applyEasing(Easing::Linear, -1.0), 0.0, 1e-9); // clamp low
    CHECK_NEAR(applyEasing(Easing::Linear, 2.0), 1.0, 1e-9);  // clamp high
    CHECK_NEAR(applyEasing((Easing)99, 0.3), 0.3, 1e-9);      // defensive default
}

// ───────────────────────── anim/Animation ─────────────────────────
TEST(Animation_value_and_clamp)
{
    Animation a(0, 100, 1000, Easing::Linear);
    CHECK_NEAR(a.value(500), 50, 1e-9);
    CHECK_NEAR(a.value(-10), 0, 1e-9);   // clamp low
    CHECK_NEAR(a.value(99999), 100, 1e-9); // clamp high
    CHECK(a.finished(1000));
    CHECK(!a.finished(999));
    Animation z(5, 9, 0); // zero duration -> jumps to target
    CHECK_NEAR(z.value(0), 9, 1e-9);
}
TEST(AnimatedProperty_lifecycle)
{
    AnimatedProperty p(0.0);
    CHECK_NEAR(p.update(0), 0.0, 1e-9);   // not animating branch
    CHECK(!p.isAnimating());
    p.animateTo(10.0, 100.0, Easing::Linear, 1000.0);
    CHECK(p.isAnimating());
    CHECK_NEAR(p.update(1050.0), 5.0, 1e-9); // halfway
    CHECK_NEAR(p.update(2000.0), 10.0, 1e-9); // finished -> snaps to target
    CHECK(!p.isAnimating());
    p.set(3.0);
    CHECK_NEAR(p.value(), 3.0, 1e-9);
}

// ───────────────────────── render + scene ─────────────────────────
using K = DrawOp::Kind;

TEST(Rectangle_sharp_and_rounded)
{
    RecordingTarget t;
    Rectangle sharp(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0)));
    sharp.render(t);
    CHECK(t.count(K::FillPath) == 1);
    CHECK(t.count(K::QuadTo) == 0);     // no corners
    CHECK(t.count(K::SetTransform) == 1);

    t.clear();
    // radius larger than half -> clamp branch; stroked border too.
    Rectangle round(Rect{0, 0, 10, 20}, Paint::filledStroked(Color::rgba(0, 0, 0), Color::rgba(255, 255, 255), 2), 999);
    round.render(t);
    CHECK(t.count(K::QuadTo) == 4);     // four rounded corners
    CHECK(t.count(K::FillPath) == 1 && t.count(K::StrokePath) == 1);
}

TEST(Line_and_Polyline)
{
    RecordingTarget t;
    Line ln(Point{0, 0}, Point{5, 5}, Color::rgba(0, 0, 0), 1.0);
    ln.render(t);
    CHECK(t.count(K::StrokePath) == 1 && t.count(K::LineTo) == 1);

    t.clear();
    Polyline empty; empty.render(t);          // empty -> early return
    CHECK(t.count(K::BeginPath) == 0);

    t.clear();
    Polyline poly;
    poly.points = {{0, 0}, {1, 1}, {2, 0}};
    poly.closed = true;
    poly.paint = Paint::stroked(Color::rgba(0, 0, 0), 1);
    poly.render(t);
    CHECK(t.count(K::LineTo) == 2 && t.count(K::ClosePath) == 1 && t.count(K::StrokePath) == 1);
}

TEST(Ellipse_four_cubics)
{
    RecordingTarget t;
    Ellipse e(Point{50, 50}, 30, 20, Paint::filled(Color::rgba(10, 20, 30)));
    e.render(t);
    CHECK(t.count(K::CubicTo) == 4);
    CHECK(t.count(K::FillPath) == 1);
}

TEST(Path_segments_spline_and_no_paint)
{
    RecordingTarget t;
    Path p;
    p.moveTo(0, 0).lineTo(1, 1).quadTo(2, 2, 3, 3).cubicTo(4, 4, 5, 5, 6, 6).close();
    // default Paint: neither fill nor stroke -> applyPaint does nothing.
    p.render(t);
    CHECK(t.count(K::MoveTo) == 1 && t.count(K::LineTo) == 1);
    CHECK(t.count(K::QuadTo) == 1 && t.count(K::CubicTo) == 1 && t.count(K::ClosePath) == 1);
    CHECK(t.count(K::FillPath) == 0 && t.count(K::StrokePath) == 0);

    t.clear();
    Path sp;
    sp.spline({}); // <2 points -> early return, no ops
    sp.render(t);
    CHECK(t.count(K::MoveTo) == 0);

    t.clear();
    Path sp2;
    sp2.paint = Paint::stroked(Color::rgba(0, 0, 0), 1);
    sp2.spline({{0, 0}, {10, 10}, {20, 0}, {30, 10}}); // exercises boundary clamps
    sp2.render(t);
    CHECK(t.count(K::MoveTo) == 1 && t.count(K::CubicTo) == 3);
}

TEST(Text_uses_fill_and_drawText)
{
    RecordingTarget t;
    Text txt("hi", Point{4, 8}, 16, Color::rgba(255, 255, 255));
    txt.render(t);
    CHECK(t.count(K::DrawText) == 1 && t.count(K::SetFill) == 1);
    std::string drawn;
    for (const auto &op : t.ops())
        if (op.kind == K::DrawText) drawn = op.text;
    CHECK(drawn == "hi");
}

TEST(Drawable_visibility)
{
    RecordingTarget t;
    Rectangle r(Rect{0, 0, 1, 1}, Paint::filled(Color::rgba(1, 1, 1)));
    r.visible = false;
    r.render(t);
    CHECK(t.ops().empty());  // hidden -> nothing recorded
}

TEST(Artboard_background_children_and_onframe)
{
    RecordingTarget t;
    Artboard ab(Size{100, 80});
    // no background, no onFrame
    ab.render(t, 0);
    CHECK(t.ops().empty());

    ab.setBackground(Color::rgba(0, 0, 0));
    int frames = 0;
    ab.setOnFrame([&](double) { ++frames; });
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0))));
    CHECK(ab.childCount() == 1);
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 1, 1}, Paint::filled(Color::rgba(0, 255, 0))));
    CHECK(ab.childCount() == 2);
    ab.clear();
    CHECK(ab.childCount() == 0);
    ab.add(std::make_shared<Rectangle>(Rect{0, 0, 10, 10}, Paint::filled(Color::rgba(255, 0, 0))));
    ab.render(t, 16.0);
    CHECK(frames == 1);
    CHECK(t.count(K::FillPath) == 2); // background + child
    CHECK(t.count(K::SetTransform) >= 2);

    // explicit save/restore/setStroke coverage via direct calls
    RecordingTarget t2;
    t2.save(); t2.restore();
    t2.setStroke(Color::rgba(1, 1, 1), 3);
    CHECK(t2.count(K::Save) == 1 && t2.count(K::Restore) == 1 && t2.count(K::SetStroke) == 1);
}

int main() { return mini::runAll(); }
