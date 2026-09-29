// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "api/api_common.h"

namespace ChatHelpers {
class Show;
} // namespace ChatHelpers

namespace AyuFeatures::UndoSend {

[[nodiscard]] bool ShouldDelay(
	const Api::SendOptions &options,
	bool ephemeral);

void Delay(
	std::shared_ptr<ChatHelpers::Show> show,
	Api::MessageToSend &&message,
	Fn<bool(const TextWithTags&)> restore);

} // namespace AyuFeatures::UndoSend
