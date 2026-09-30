// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/ocr/ocr.h"

#include "lang_auto.h"
#include "data/data_photo.h"
#include "data/data_photo_media.h"
#include "main/main_session.h"
#include "ui/image/image.h"
#include "ui/widgets/popup_menu.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"

#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>

#ifdef Q_OS_WIN
#include "base/platform/win/base_windows_winrt.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>
#endif // Q_OS_WIN

namespace AyuFeatures::Ocr {
namespace {

#ifdef Q_OS_WIN

void Recognize(QImage image, Fn<void(QString)> done) {
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Graphics::Imaging;
	using namespace winrt::Windows::Media::Ocr;
	using namespace winrt::Windows::Storage::Streams;

	const auto started = base::WinRT::Try([&] {
		const auto engine = OcrEngine::TryCreateFromUserProfileLanguages();
		if (!engine) {
			done(QString());
			return;
		}
		const auto limit = int(OcrEngine::MaxImageDimension());
		if (image.width() > limit || image.height() > limit) {
			image = image.scaled(
				limit,
				limit,
				Qt::KeepAspectRatio,
				Qt::SmoothTransformation);
		}
		image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
		const auto size = uint32(image.width() * image.height() * 4);
		auto buffer = Buffer(size);
		buffer.Length(size);
		for (auto y = 0; y != image.height(); ++y) {
			memcpy(
				buffer.data() + (y * image.width() * 4),
				image.constScanLine(y),
				image.width() * 4);
		}
		auto bitmap = SoftwareBitmap(
			BitmapPixelFormat::Bgra8,
			image.width(),
			image.height(),
			BitmapAlphaMode::Premultiplied);
		bitmap.CopyFromBuffer(buffer);
		engine.RecognizeAsync(bitmap).Completed([=](
				IAsyncOperation<OcrResult> that,
				AsyncStatus status) {
			auto text = QString();
			if (status == AsyncStatus::Completed) {
				base::WinRT::Try([&] {
					auto lines = QStringList();
					for (const auto &line : that.GetResults().Lines()) {
						const auto value = line.Text();
						lines.push_back(QString::fromWCharArray(
							value.c_str(),
							int(value.size())));
					}
					text = lines.join(u'\n');
				});
			}
			crl::on_main([=] {
				done(text);
			});
		});
	});
	if (!started) {
		done(QString());
	}
}

#else // Q_OS_WIN

void Recognize(QImage image, Fn<void(QString)> done) {
	done(QString());
}

#endif // Q_OS_WIN

} // namespace

void AddMenuAction(
		not_null<Ui::PopupMenu*> menu,
		not_null<PhotoData*> photo) {
#ifdef Q_OS_WIN
	const auto media = photo->activeMediaView();
	if (!media || !media->loaded()) {
		return;
	}
	const auto session = &photo->session();
	menu->addAction(tr::ayu_OcrCopyText(tr::now), [=] {
		const auto media = photo->activeMediaView();
		const auto image = media
			? media->image(Data::PhotoSize::Large)
			: nullptr;
		if (!image) {
			return;
		}
		Recognize(image->original(), crl::guard(session, [=](QString text) {
			const auto window = session->tryResolveWindow();
			if (text.trimmed().isEmpty()) {
				if (window) {
					window->showToast(tr::ayu_OcrNothingFound(tr::now));
				}
				return;
			}
			QGuiApplication::clipboard()->setText(text);
			if (window) {
				window->showToast(tr::ayu_OcrCopied(tr::now));
			}
		}));
	}, &st::menuIconCopy);
#endif // Q_OS_WIN
}

} // namespace AyuFeatures::Ocr
