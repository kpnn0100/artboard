/*
 *  Arstro Artboard — Drawable: base of every scene object.
 *
 *  Holds a local transform + visibility. render() brackets the subclass onDraw()
 *  with save()/setTransform/restore so a parent transform composes cleanly. The
 *  app/shapes depend only on IRenderTarget (DIP).
 */
#pragma once
#include "../render/RenderTarget.h"

namespace artboard
{
    class Drawable
    {
    public:
        virtual ~Drawable() = default;

        Transform transform = Transform::identity();
        bool visible = true;

        /** Render under `parent` transform (parent * local). No-op if hidden. */
        void render(IRenderTarget &t, const Transform &parent = Transform::identity()) const
        {
            if (!visible)
                return;
            t.save();
            t.setTransform(parent.mul(transform));
            onDraw(t);
            t.restore();
        }

    protected:
        virtual void onDraw(IRenderTarget &t) const = 0;
    };
}
