// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/link_privacy/link_privacy.h"

#include "ayu/ayu_settings.h"
#include "base/flat_set.h"

#include <QtCore/QUrl>

namespace AyuFeatures::LinkPrivacy {
namespace {

[[nodiscard]] bool HostMatches(const QString &host, const QString &domain) {
	return (host == domain) || host.endsWith(u"."_q + domain);
}

[[nodiscard]] bool IsTrackingKey(const QString &key, const QString &host) {
	static const auto kAlways = base::flat_set<QString>{
		u"fbclid"_q,
		u"gclid"_q,
		u"dclid"_q,
		u"gbraid"_q,
		u"wbraid"_q,
		u"msclkid"_q,
		u"yclid"_q,
		u"ysclid"_q,
		u"igshid"_q,
		u"igsh"_q,
		u"mc_cid"_q,
		u"mc_eid"_q,
		u"_hsenc"_q,
		u"_hsmi"_q,
		u"mkt_tok"_q,
		u"twclid"_q,
		u"ttclid"_q,
		u"li_fat_id"_q,
		u"rb_clickid"_q,
		u"s_cid"_q,
		u"vero_id"_q,
		u"oly_anon_id"_q,
		u"oly_enc_id"_q,
	};
	const auto lower = key.toLower();
	if (lower.startsWith(u"utm_"_q) || kAlways.contains(lower)) {
		return true;
	}
	const auto shareId = (lower == u"si"_q)
		&& (HostMatches(host, u"youtube.com"_q)
			|| HostMatches(host, u"youtu.be"_q)
			|| HostMatches(host, u"spotify.com"_q));
	const auto xShare = (lower == u"s"_q || lower == u"t"_q)
		&& (HostMatches(host, u"x.com"_q)
			|| HostMatches(host, u"twitter.com"_q));
	return shareId || xShare;
}

} // namespace

bool ConfirmAllLinks() {
	return AyuSettings::getInstance().confirmAllLinks();
}

QString CleanUrl(const QString &url) {
	if (!AyuSettings::getInstance().stripTrackingParameters()) {
		return url;
	}
	auto parsed = QUrl(url, QUrl::TolerantMode);
	const auto scheme = parsed.scheme().toLower();
	if (!parsed.isValid()
		|| !parsed.hasQuery()
		|| (scheme != u"http"_q && scheme != u"https"_q)) {
		return url;
	}
	const auto host = parsed.host().toLower();
	const auto parts = parsed.query(QUrl::FullyEncoded).split(u'&');
	auto kept = QStringList();
	for (const auto &part : parts) {
		const auto key = part.section(u'=', 0, 0);
		if (IsTrackingKey(QUrl::fromPercentEncoding(key.toUtf8()), host)) {
			continue;
		}
		kept.push_back(part);
	}
	if (kept.size() == parts.size()) {
		return url;
	}
	parsed.setQuery(
		kept.isEmpty() ? QString() : kept.join(u'&'),
		QUrl::TolerantMode);
	return parsed.toString();
}

} // namespace AyuFeatures::LinkPrivacy
