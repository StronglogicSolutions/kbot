#!/usr/bin/env bash

third_party/gn/out/gn gen out
ninja -C out
patchelf --replace-needed libcpr.so.1 third_party/cpr/build/lib/libcpr.so out/ut_kbot
