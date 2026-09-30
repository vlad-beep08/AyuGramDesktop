// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

namespace Main {
class Session;
} // namespace Main

namespace AyuFeatures::SelfDestruct {

void Track(not_null<Main::Session*> session, FullMsgId id, int seconds);

[[nodiscard]] QString DurationText(int seconds);
[[nodiscard]] std::vector<int> DurationOptions();

} // namespace AyuFeatures::SelfDestruct
