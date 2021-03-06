#!/usr/bin/env bash

third_party/gn/out/gn gen out
ninja -C out
patchelf --replace-needed libcpr.so.1 third_party/cpr/build/lib/libcpr.so out/youtube_bot
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/youtube_bot
patchelf --replace-needed libkstodon_lib.so third_party/kstodon/out/libkstodon_lib.so out/mastodon_bot
patchelf --replace-needed libkscord_lib.so third_party/kscord/out/libkscord_lib.so out/discord_bot
patchelf --replace-needed libkstodon_lib.so third_party/kstodon/out/libkstodon_lib.so out/broker
patchelf --replace-needed libkscord_lib.so third_party/kscord/out/libkscord_lib.so out/broker
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/broker
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/ut_kbot

