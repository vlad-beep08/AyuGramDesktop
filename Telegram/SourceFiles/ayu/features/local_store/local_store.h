// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ayu/libs/json.hpp"

class HistoryItem;

namespace AyuFeatures::LocalStore {

[[nodiscard]] nlohmann::json Read(const QString &name);
void Write(const QString &name, const nlohmann::json &data);

[[nodiscard]] QString PreviewText(not_null<HistoryItem*> item);

} // namespace AyuFeatures::LocalStore
