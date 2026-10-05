#include "ManualManager.h"

namespace nfd42
{
void ManualManager::openManual (const char* data, int size, const juce::String& filename)
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("NF Audio Tools").getChildFile ("NF Delay 42").getChildFile ("Manuals");
    dir.createDirectory();
    auto file = dir.getChildFile (filename);
    if (! file.existsAsFile() || file.getSize() != (juce::int64) size)
    {
        juce::TemporaryFile temp (file);
        if (temp.getFile().replaceWithData (data, (size_t) size)) temp.overwriteTargetFileWithTemporary();
    }
    if (file.existsAsFile()) file.startAsProcess();
}
}
