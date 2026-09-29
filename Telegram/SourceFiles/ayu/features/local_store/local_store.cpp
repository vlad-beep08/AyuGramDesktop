// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/local_store/local_store.h"

#include "history/history_item.h"
#include "logs.h"
#include "settings.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSaveFile>

namespace AyuFeatures::LocalStore {
namespace {

constexpr auto kPreviewLength = 120;

[[nodiscard]] QString StoreFolder() {
	return cWorkingDir() + u"tdata/ayu/"_q;
}

[[nodiscard]] QString StorePath(const QString &name) {
	return StoreFolder() + name + u".json"_q;
}

} // namespace

nlohmann::json Read(const QString &name) {
	auto file = QFile(StorePath(name));
	if (!file.open(QIODevice::ReadOnly)) {
		return nlohmann::json::object();
	}
	const auto bytes = file.readAll();
	try {
		auto result = nlohmann::json::parse(
			bytes.constData(),
			bytes.constData() + bytes.size());
		return result.is_object() ? result : nlohmann::json::object();
	} catch (...) {
		LOG(("AyuGram: could not parse local store %1").arg(name));
		return nlohmann::json::object();
	}
}

void Write(const QString &name, const nlohmann::json &data) {
	QDir().mkpath(StoreFolder());
	auto file = QSaveFile(StorePath(name));
	if (!file.open(QIODevice::WriteOnly)) {
		LOG(("AyuGram: could not open local store %1").arg(name));
		return;
	}
	const auto dumped = data.dump();
	file.write(dumped.data(), qint64(dumped.size()));
	if (!file.commit()) {
		LOG(("AyuGram: could not write local store %1").arg(name));
	}
}

QString PreviewText(not_null<HistoryItem*> item) {
	auto text = item->notificationText().text.simplified();
	if (text.size() > kPreviewLength) {
		text = text.mid(0, kPreviewLength - 1) + QChar(0x2026);
	}
	return text;
}

} // namespace AyuFeatures::LocalStore
