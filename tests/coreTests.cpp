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

// ───────────────────────── input: GestureRecognizer ─────────────────────────
using GT = Gesture::Type;
using PB = PointerButton;

static std::vector<Gesture> recordGestures(std::function<void(GestureRecognizer &)> drive)
{
    GestureRecognizer r;
    std::vector<Gesture> out;
    r.setSink([&](const Gesture &g) { out.push_back(g); });
    drive(r);
    return out;
}
static int countG(const std::vector<Gesture> &v, GT t)
{
    int n = 0; for (auto &g : v) if (g.type == t) ++n; return n;
}
static int countOf(const std::vector<GT> &v, GT t)
{
    int n = 0; for (auto x : v) if (x == t) ++n; return n;
}

TEST(Gesture_click)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 120});
    });
    CHECK(countG(g, GT::Down) == 1 && countG(g, GT::Up) == 1 && countG(g, GT::Click) == 1);
    CHECK(countG(g, GT::DoubleClick) == 0);
}

TEST(Gesture_double_click_then_reset)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.setDoubleClickMs(300);
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 120});   // Click
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 250});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 280});   // DoubleClick (within window)
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 320});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 340});   // Click again (reset)
    });
    CHECK(countG(g, GT::DoubleClick) == 1);
    CHECK(countG(g, GT::Click) == 2);
}

TEST(Gesture_not_double_when_far_or_late)
{
    auto far = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 110});
        r.feed({RawPointer::Kind::Down, {99, 99}, PB::Left, 120});
        r.feed({RawPointer::Kind::Up,   {99, 99}, PB::Left, 130}); // far -> not double
    });
    CHECK(countG(far, GT::DoubleClick) == 0 && countG(far, GT::Click) == 2);

    auto late = recordGestures([](GestureRecognizer &r) {
        r.setDoubleClickMs(100);
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 110});
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Left, 900});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Left, 910});   // late -> not double
    });
    CHECK(countG(late, GT::DoubleClick) == 0 && countG(late, GT::Click) == 2);
}

TEST(Gesture_right_click)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Down, {5, 5}, PB::Right, 100});
        r.feed({RawPointer::Kind::Up,   {5, 5}, PB::Right, 110});
    });
    CHECK(countG(g, GT::RightClick) == 1 && countG(g, GT::Click) == 0);
}

TEST(Gesture_drag_and_drop)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.setDragThreshold(5);
        r.feed({RawPointer::Kind::Down, {0, 0}, PB::Left, 0});
        r.feed({RawPointer::Kind::Move, {20, 0}, PB::Left, 10});  // crosses threshold: DragStart + Drag
        r.feed({RawPointer::Kind::Move, {40, 0}, PB::Left, 20});  // Drag
        r.feed({RawPointer::Kind::Up,   {40, 0}, PB::Left, 30});  // Up + Drop (no Click)
    });
    CHECK(countG(g, GT::DragStart) == 1);
    CHECK(countG(g, GT::Drag) == 2);
    CHECK(countG(g, GT::Drop) == 1);
    CHECK(countG(g, GT::Click) == 0);
}

TEST(Gesture_hover_move)
{
    auto g = recordGestures([](GestureRecognizer &r) {
        r.feed({RawPointer::Kind::Move, {3, 3}, PB::Left, 5}); // no press -> hover Move
    });
    CHECK(countG(g, GT::Move) == 1);
    // a recognizer with no sink must not crash:
    GestureRecognizer silent;
    silent.feed({RawPointer::Kind::Down, {0, 0}, PB::Left, 0});
}

// ───────────────────────── input: InputRouter ─────────────────────────
TEST(InputRouter_topmost_capture_and_miss)
{
    int aHits = 0;
    std::vector<GT> bSeq;
    RectTarget a(Rect{0, 0, 10, 10}, [&](const Gesture &) { ++aHits; });
    RectTarget b(Rect{0, 0, 10, 10}, [&](const Gesture &g) { bSeq.push_back(g.type); });
    InputRouter router;
    router.add(&a);
    router.add(&b); // b added last -> on top

    // Press sequence captures the topmost (b); drag leaves bounds but still routes to b.
    router.route({GT::Down, {5, 5}, {5, 5}, PB::Left});
    CHECK(router.captured() == &b);
    router.route({GT::DragStart, {50, 50}, {5, 5}, PB::Left});
    router.route({GT::Drag, {80, 80}, {5, 5}, PB::Left});
    router.route({GT::Drop, {80, 80}, {5, 5}, PB::Left});
    CHECK(router.captured() == nullptr);
    CHECK(bSeq.size() == 4 && aHits == 0); // Down,DragStart,Drag,Drop all to b; a never hit

    // Hover (no capture) routes to topmost; a miss routes to nobody.
    bSeq.clear();
    router.route({GT::Move, {5, 5}, {5, 5}, PB::Left});
    CHECK(bSeq.size() == 1);
    router.route({GT::Move, {500, 500}, {500, 500}, PB::Left}); // miss -> nobody
    router.route({GT::Click, {5, 5}, {5, 5}, PB::Left});        // fresh hit-test -> b
    router.route({GT::Click, {500, 500}, {500, 500}, PB::Left}); // miss -> nobody (topAt nullptr)
    CHECK(countOf(bSeq, GT::Move) == 1 && countOf(bSeq, GT::Click) == 1);

    // Down on empty space: no capture, no crash.
    router.route({GT::Down, {999, 999}, {999, 999}, PB::Left});
    CHECK(router.captured() == nullptr);
    router.route({GT::Up, {999, 999}, {999, 999}, PB::Left}); // up with no capture
    router.clear();
}

int main() { return mini::runAll(); }
