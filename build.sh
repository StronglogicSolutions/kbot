#!/usr/bin/env bash

third_party/gn/out/gn gen out --args='is_debug=true'
ninja -C out -v
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/youtube_bot
patchelf --replace-needed libkstodon_lib.so third_party/kstodon/out/libkstodon_lib.so out/mastodon_bot
patchelf --replace-needed libmatrix_client.so.0.9.2 third_party/katrix/third_party/mtxclient/build/libmatrix_client.so.0.9.2 out/matrix_bot
patchelf --replace-needed libkscord_lib.so third_party/kscord/out/libkscord_lib.so out/discord_bot
patchelf --replace-needed libkstodon_lib.so third_party/kstodon/out/libkstodon_lib.so out/broker
patchelf --replace-needed libkscord_lib.so third_party/kscord/out/libkscord_lib.so out/broker
patchelf --replace-needed libknlp.so third_party/knlp/out/libknlp.so out/broker
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/broker
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/youtube_bot
patchelf --replace-needed libcpr.so.1 /usr/local/lib/libcpr.so.1 out/broker
patchelf --replace-needed libmatrix_client.so.0.9.2 third_party/katrix/third_party/mtxclient/build/libmatrix_client.so.0.9.2 out/broker
patchelf --replace-needed libcoeurld.so third_party/katrix/third_party/mtxclient/build/_deps/coeurl-build/libcoeurld.so out/broker
patchelf --replace-needed libolmd.so.3 third_party/katrix/third_party/mtxclient/build/libolmd.so.3 out/broker
patchelf --replace-needed libTgBot.so.1 third_party/keleqram/third_party/tgbot-cpp/libTgBot.so.1 out/broker
patchelf --replace-needed libTgBot.so.1 third_party/keleqram/third_party/tgbot-cpp/libTgBot.so.1 out/telegram_bot
patchelf --replace-needed libktube_lib.so third_party/ktube/out/libktube_lib.so out/ut_kbot
patchelf --replace-needed libg3log.so.2 ./third_party/klogger/build/third_party/g3log/libg3log.so.2 out/broker

