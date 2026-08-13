/*
 *  Arstro Artboard — Clipboard: the text clipboard seam (FR-44).
 *
 *  The core is platform-free, so it cannot read a system clipboard — that would be an OS
 *  call. Instead it owns an in-process default, so copy/paste works in tests, headless
 *  builds, and any adapter that never binds anything; a host with a real clipboard calls
 *  install() once at startup and every TextBox picks it up.
 *
 *  Deliberately NOT part of IRenderTarget or the input HAL: it is neither drawing nor
 *  pointer/key input, and it carries no per-frame cost.
 */
#pragma once
#include <functional>
#include <string>

namespace artboard
{
    class Clipboard
    {
    public:
        using Reader = std::function<std::string()>;
        using Writer = std::function<void(const std::string &)>;

        /** Bind the host's clipboard. Passing an empty function for either side leaves that
         *  direction on the in-process default. */
        static void install(Reader reader, Writer writer);
        /** Back to the in-process default (and clear it) — what a test uses to stay isolated. */
        static void reset();

        static std::string read();
        static void write(const std::string &text);
    };
}
