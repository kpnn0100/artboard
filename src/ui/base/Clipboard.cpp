#include "Clipboard.h"

namespace artboard
{
    namespace
    {
        struct Slots
        {
            Clipboard::Reader reader;
            Clipboard::Writer writer;
            std::string local;   // the in-process default's storage
        };

        Slots &slots()
        {
            static Slots s;
            return s;
        }
    }

    void Clipboard::install(Reader reader, Writer writer)
    {
        slots().reader = std::move(reader);
        slots().writer = std::move(writer);
    }

    void Clipboard::reset()
    {
        slots().reader = nullptr;
        slots().writer = nullptr;
        slots().local.clear();
    }

    std::string Clipboard::read()
    {
        Slots &s = slots();
        return s.reader ? s.reader() : s.local;
    }

    void Clipboard::write(const std::string &text)
    {
        Slots &s = slots();
        if (s.writer)
            s.writer(text);
        else
            s.local = text;
    }
}
