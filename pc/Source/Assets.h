// Finds and loads the shared data (cabinet IRs, rooms, band samples) installed next to the plugin.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>

namespace jw {

/** Source files are UTF-8; JUCE treats plain char literals as ASCII, so Polish text goes through this. */
inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
inline juce::StringArray U (std::initializer_list<const char*> l) { juce::StringArray a; for (auto* s : l) a.add (U (s)); return a; }

inline juce::File dataDir()
{
    juce::Array<juce::File> candidates {
        juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory).getChildFile ("Jesienna Warszawa"),
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory().getChildFile ("data"),
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory().getParentDirectory().getChildFile ("Resources").getChildFile ("data"),
        juce::File::getSpecialLocation (juce::File::currentApplicationFile).getParentDirectory().getChildFile ("data"),
        juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Jesienna Warszawa"),
        juce::File::getCurrentWorkingDirectory().getChildFile ("data"),
    };
    if (auto env = juce::SystemStats::getEnvironmentVariable ("JESIENNA_DATA", {}); env.isNotEmpty())
        candidates.insert (0, juce::File (env));
    // portable copy: data/ next to the app or next to the .vst3 bundle
    auto up = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    for (int i = 0; i < 5 && up.exists(); ++i, up = up.getParentDirectory()) candidates.add (up.getChildFile ("data"));
    for (auto& c : candidates)
        if (c.getChildFile ("cab").getChildFile ("cab.json").existsAsFile())
            return c;
    return {};
}

struct IrEntry { juce::String name, group; juce::File file; };

/** Captured amps / pedals (Neural Amp Modeler profiles). */
struct NamEntry { juce::String name, style, look; juce::File file; float trimDb = 0; };
/** The player's own profiles: Documents/Jesienna Warszawa/NAM (amps) and NAM/Kostki (pedals). */
inline juce::File userNamDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Jesienna Warszawa").getChildFile ("NAM");
}

inline juce::Array<NamEntry> listNam (const juce::String& kind)
{
    juce::Array<NamEntry> out;
    auto dir = dataDir();
    if (dir.exists())
    {
        auto json = juce::JSON::parse (dir.getChildFile ("nam/nam.json"));
        if (auto* arr = json.getArray())
            for (auto& v : *arr)
                if (v["kind"].toString() == kind)
                    out.add ({ v["name"].toString(), v["style"].toString(), v["look"].toString(), dir.getChildFile (v["file"].toString()), (float) v["trim"] });
    }
    // own files, levelled automatically when loaded (trim = NaN)
    auto user = kind == "pedal" ? userNamDir().getChildFile ("Kostki") : userNamDir();
    if (! userNamDir().exists())
    {
        userNamDir().getChildFile ("Kostki").createDirectory();
        userNamDir().getChildFile ("CZYTAJ.txt").replaceWithText (juce::String::fromUTF8 (
            "Wrzuć tu własne profile wzmacniaczy w formacie .nam (np. z tone3000.com), a profile kostek do podfolderu Kostki.\r\n"
            "Po ponownym otwarciu wtyczki pojawią się na końcu listy wzmacniaczy / kostek. Głośność wyrównuje się sama.\r\n"
            "Używaj profili samego wzmacniacza (bez kolumny) – kolumnę wybierasz osobno.\r\n"));
    }
    auto files = user.findChildFiles (juce::File::findFiles, false, "*.nam");
    files.sort();
    for (auto& f : files)
        out.add ({ f.getFileNameWithoutExtension(), juce::String::fromUTF8 ("twój profil"), kind == "pedal" ? "black" : "boutique", f, std::numeric_limits<float>::quiet_NaN() });
    return out;
}

inline juce::Array<IrEntry> listCabs()
{
    juce::Array<IrEntry> out;
    auto dir = dataDir(); if (! dir.exists()) return out;
    auto json = juce::JSON::parse (dir.getChildFile ("cab/cab.json"));
    if (auto* arr = json.getArray())
        for (auto& v : *arr)
            out.add ({ v["name"].toString(), v["pack"].toString(), dir.getChildFile (v["file"].toString()) });
    return out;
}

inline juce::Array<IrEntry> listRooms()
{
    juce::Array<IrEntry> out;
    auto dir = dataDir(); if (! dir.exists()) return out;
    auto json = juce::JSON::parse (dir.getChildFile ("rooms/ir.json"));
    if (auto* arr = json.getArray())
        for (auto& v : *arr)
            if (v["kind"].toString() == "space")
                out.add ({ v["name"].toString(), v["group"].toString(),
                           dir.getChildFile ("rooms").getChildFile (juce::File (v["file"].toString()).getFileName()) });
    auto tube = dir.getChildFile ("rooms/sprezyna_lampowa.wav");
    if (tube.existsAsFile()) out.add ({ U ("Sprężyna lampowa (Fisher K-10)"), U ("Pogłosy sprzętowe"), tube });
    return out;
}

inline juce::Array<IrEntry> listColours()
{
    juce::Array<IrEntry> out;
    auto dir = dataDir(); if (! dir.exists()) return out;
    auto json = juce::JSON::parse (dir.getChildFile ("rooms/ir.json"));
    if (auto* arr = json.getArray())
        for (auto& v : *arr)
            if (v["kind"].toString() == "color")
                out.add ({ v["name"].toString(), v["group"].toString(),
                           dir.getChildFile ("rooms").getChildFile (juce::File (v["file"].toString()).getFileName()) });
    return out;
}

/** Reads any wav/mp3 into a buffer (thread-safe: own format manager). */
inline bool readAudio (const juce::File& f, juce::AudioBuffer<float>& out, double& sampleRate, double maxSeconds = 30.0)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr) return false;
    auto len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (maxSeconds * r->sampleRate));
    out.setSize ((int) juce::jmax (1u, r->numChannels), len);
    r->read (&out, 0, len, 0, true, true);
    sampleRate = r->sampleRate;
    return len > 0;
}

/** Trims leading silence (MP3 encoder delay) so an IR does not delay the signal. */
inline void trimLeadingSilence (juce::AudioBuffer<float>& b)
{
    float peak = 0; for (int c = 0; c < b.getNumChannels(); ++c) peak = juce::jmax (peak, b.getMagnitude (c, 0, b.getNumSamples()));
    if (peak <= 0) return;
    int start = 0;
    for (; start < b.getNumSamples(); ++start)
    {
        bool hit = false;
        for (int c = 0; c < b.getNumChannels(); ++c) if (std::abs (b.getSample (c, start)) > peak * 0.05f) hit = true;
        if (hit) break;
    }
    start = juce::jmax (0, start - 4);
    if (start == 0) return;
    juce::AudioBuffer<float> t (b.getNumChannels(), b.getNumSamples() - start);
    for (int c = 0; c < b.getNumChannels(); ++c) t.copyFrom (c, 0, b, c, start, t.getNumSamples());
    b = std::move (t);
}

} // namespace jw
