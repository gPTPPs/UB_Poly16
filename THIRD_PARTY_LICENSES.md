# Third-party licenses

UB_Poly16 is licensed under the **GNU AGPLv3** (see [LICENSE](LICENSE)).
None of the dependencies below are vendored in this repository — they are
fetched or installed separately at build time.

## JUCE 8

- **What**: C++ application/plugin framework (audio engine, GUI, plugin wrappers).
- **License**: dual-licensed; used here under the **AGPLv3** option.
- **Where**: not included — point CMake at a local checkout with `-DUB_JUCE_DIR=<path>`.
- **Source**: https://github.com/juce-framework/JUCE

## VST3 SDK (Steinberg)

- **What**: VST3 plugin format support, bundled inside JUCE.
- **License**: dual-licensed **GPLv3** / Steinberg VST 3 Licensing Agreement;
  used here under the GPLv3 option (compatible with AGPLv3).
- **Source**: https://github.com/steinbergmedia/vst3sdk
- VST® is a registered trademark of Steinberg Media Technologies GmbH.

## Steinberg ASIO SDK — NOT included

- **What**: optional low-latency Windows audio for the standalone app.
- **License**: **proprietary Steinberg license, not redistributable and not
  compatible with (A)GPL distribution.** For that reason the SDK is not part
  of this repository and public/release binaries are built **without** ASIO
  (`UB_ASIO=OFF`, the default).
- If you want ASIO in your own local build: download the SDK yourself from
  https://www.steinberg.net/developers/ and configure with
  `-DUB_ASIO=ON -DUB_ASIO_SDK_DIR=<sdk>/common`. The resulting binary is for
  personal use and must not be distributed.
- ASIO is a trademark and software of Steinberg Media Technologies GmbH.

## Trademarks / preset names

Factory preset names reference classic hardware synthesizers (Juno, Jupiter,
Minimoog, Prophet, OB-Xa, CS-80, SH-101, ARP…) as a homage. All trademarks
belong to their respective owners; no affiliation or endorsement is implied,
and no sampled content from any of these instruments is used — everything is
synthesized from scratch.
