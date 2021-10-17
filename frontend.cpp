#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include <sndfile.h>

#include "tclap/CmdLine.h"

#include "CDSkip.hpp"

int main(int argc, char* argv[])
{
    std::string inFileName;
    std::string outFileName;
    float amplify;

    try {
        TCLAP::CmdLine cmd("safety-limiter frontend", ' ', "0.0.1");

        TCLAP::UnlabeledValueArg<std::string> inFileArg("inFile", "Input sound file", true, "", "string");
        cmd.add(inFileArg);

        TCLAP::UnlabeledValueArg<std::string> outFileArg("outFile", "Output WAV file", true, "", "string");
        cmd.add(outFileArg);

        cmd.parse(argc, argv);

        inFileName = inFileArg.getValue();
        outFileName = outFileArg.getValue();
    } catch (TCLAP::ArgException e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    }

    SF_INFO sfInfo;
    sfInfo.format = 0;
    auto soundFile = sf_open(inFileName.c_str(), SFM_READ, &sfInfo);

    if (soundFile == nullptr) {
        std::cerr << "Audio loading failed: " << sf_strerror(soundFile);
        exit(1);
    }

    int sampleRate = sfInfo.samplerate;
    int frames = sfInfo.frames;
    int channels = sfInfo.channels;

    if (channels != 2) {
        std::cerr << "Input channels must be 2" << std::endl;
        exit(1);
    }

    float* audio = new float[frames * channels];

    sf_readf_float(soundFile, audio, frames);
    sf_close(soundFile);

    float maxDelay = 0.3f;
    int memoryLength = CDSkip::CDSkip::getMemoryLength(sampleRate, maxDelay);
    float* memory = new float[memoryLength];

    CDSkip::CDSkip cdSkip(sampleRate, maxDelay, memory);
    for (int i = 0; i < frames; i++) {
        auto out = cdSkip.process(
            std::make_pair(audio[i * channels], audio[i * channels + 1])
        );
        audio[i * channels] = out.first;
        audio[i * channels + 1] = out.second;
    }

    SF_INFO sfInfo2;
    sfInfo2.samplerate = sampleRate;
    sfInfo2.channels = channels;
    sfInfo2.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    sfInfo2.sections = 1;
    sfInfo2.seekable = 0;
    auto soundFile2 = sf_open(outFileName.c_str(), SFM_WRITE, &sfInfo2);
    if (soundFile2 == nullptr) {
        std::cerr << "Audio saving failed: " << sf_strerror(soundFile2);
        exit(1);
    }
    sf_writef_float(soundFile2, audio, frames);
    sf_close(soundFile2);

    delete[] audio;

    return 0;
}
