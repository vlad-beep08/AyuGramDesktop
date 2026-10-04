// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/translations/translations.h"

#include "ayu/features/branding/branding.h"
#include "base/flat_set.h"
#include "lang/lang_instance.h"

namespace AyuFeatures::Translations {
namespace {

struct Entry {
	const char *key = nullptr;
	const char *value = nullptr;
};

[[nodiscard]] bool IsRussian() {
	const auto &instance = Lang::GetInstance();
	return instance.id().startsWith(u"ru"_q)
		|| (instance.baseId() == u"ru"_q);
}

[[nodiscard]] const std::vector<Entry> &Russian() {
	static const auto result = std::vector<Entry>{
		{ "ayu_SendingHeader", "Отправка" },
		{ "ayu_UndoSendDelay", "Задержка отправки" },
		{ "ayu_UndoSendDelayOff", "Выкл." },
		{ "ayu_UndoSendDelaySeconds#one", "{count} сек" },
		{ "ayu_UndoSendDelaySeconds#few", "{count} сек" },
		{ "ayu_UndoSendDelaySeconds#many", "{count} сек" },
		{ "ayu_UndoSendDelaySeconds#other", "{count} сек" },
		{ "ayu_UndoSendDelayDescription", "Текстовые сообщения ждут выбранное время перед отправкой, и их можно отменить из уведомления. Если закрыть приложение во время задержки, сообщение не отправится." },
		{ "ayu_UndoSendToast#one", "Отправка через {count} секунду." },
		{ "ayu_UndoSendToast#few", "Отправка через {count} секунды." },
		{ "ayu_UndoSendToast#many", "Отправка через {count} секунд." },
		{ "ayu_UndoSendToast#other", "Отправка через {count} секунды." },
		{ "ayu_UndoSendButton", "Отменить" },
		{ "ayu_UndoSendCancelled", "Сообщение не отправлено." },
		{ "ayu_UndoSendCancelledCopied", "Сообщение не отправлено. Текст скопирован в буфер обмена." },
		{ "ayu_SaveDraftsHistory", "Сохранять историю черновиков" },
		{ "ayu_SaveDraftsHistoryDescription", "Хранит текст, который вы набрали, но не отправили. Вернуть его можно через меню Чикенграм в чате. Хранится только на этом устройстве." },
		{ "ayu_DraftsHistoryMenuText", "История черновиков" },
		{ "ayu_DraftsHistoryTitle", "История черновиков" },
		{ "ayu_DraftsHistoryAbout", "Нажмите на черновик, чтобы скопировать его." },
		{ "ayu_DraftsHistoryEmpty", "В этом чате нет сохранённых черновиков." },
		{ "ayu_DraftsHistoryCopied", "Черновик скопирован." },
		{ "ayu_DraftsHistoryClear", "Очистить" },
		{ "ayu_BookmarkAdd", "В закладки" },
		{ "ayu_BookmarkRemove", "Убрать из закладок" },
		{ "ayu_BookmarkAdded", "Закладка добавлена." },
		{ "ayu_BookmarkRemoved", "Закладка удалена." },
		{ "ayu_BookmarksMenuText", "Закладки" },
		{ "ayu_AllBookmarksMenuText", "Все закладки" },
		{ "ayu_BookmarksTitle", "Закладки" },
		{ "ayu_BookmarksEmpty", "Закладок пока нет. Нажмите правой кнопкой по сообщению и выберите «В закладки»." },
		{ "ayu_BookmarksNoText", "Сообщение без текста" },
		{ "ayu_BookmarksClear", "Очистить" },
		{ "ayu_RemindMe", "Напомнить" },
		{ "ayu_RemindIn30Minutes", "Через 30 минут" },
		{ "ayu_RemindIn1Hour", "Через 1 час" },
		{ "ayu_RemindIn3Hours", "Через 3 часа" },
		{ "ayu_RemindTomorrowMorning", "Завтра в 9:00" },
		{ "ayu_ReminderSet", "Напоминание на {date}." },
		{ "ayu_ReminderTitle", "Напоминание" },
		{ "ayu_ReminderOpen", "Открыть" },
		{ "ayu_ConfirmAllLinks", "Подтверждать все внешние ссылки" },
		{ "ayu_StripTrackingParameters", "Убирать трекинг из ссылок" },
		{ "ayu_StripImageMetadata", "Удалять метаданные фото" },
		{ "ayu_StripImageMetadataDescription", "Удаляет геопозицию, модель камеры и другие скрытые данные из JPEG и PNG, отправленных файлом. Ориентация изображения сохраняется." },
		{ "ayu_SendWithWatermark", "Отправить с невидимой меткой" },
		{ "ayu_WatermarkSentTo", "Невидимая метка: отправлено {name}" },
		{ "ayu_WatermarkUnknownRecipient", "Неизвестный получатель" },
		{ "ayu_QuickPhraseButton", "Кнопка быстрой фразы" },
		{ "ayu_QuickPhraseButtonAbout", "Кнопка рядом с полем ввода отправляет быструю фразу в один клик." },
		{ "ayu_ExtrasHeader", "Дополнительно" },
		{ "ayu_ContactCardMenuText", "Карточка контакта" },
		{ "ayu_ContactCardBirthday", "День рождения: {date}" },
		{ "ayu_ContactCardLastSentDays#one", "Вы писали {count} день назад" },
		{ "ayu_ContactCardLastSentDays#few", "Вы писали {count} дня назад" },
		{ "ayu_ContactCardLastSentDays#many", "Вы писали {count} дней назад" },
		{ "ayu_ContactCardLastSentDays#other", "Вы писали {count} дня назад" },
		{ "ayu_ContactCardLastSentToday", "Вы писали сегодня" },
		{ "ayu_ContactCardLastSentNever", "Вы ещё не писали сюда из этого приложения" },
		{ "ayu_ContactCardNotes", "Заметки" },
		{ "ayu_ContactCardTags", "Теги через запятую" },
		{ "ayu_DataExport", "Экспорт данных Чикенграм" },
		{ "ayu_DataImport", "Импорт данных Чикенграм" },
		{ "ayu_DataExported", "Данные Чикенграм экспортированы." },
		{ "ayu_DataTransferFailed", "Не удалось прочитать или записать файл резервной копии." },
		{ "ayu_DataTransferAbout", "Сохраняет настройки Чикенграм, закладки, напоминания, историю черновиков, карточки контактов и скрытые чаты в один файл, чтобы перенести их на другой компьютер." },
		{ "ayu_DurationSeconds#one", "{count} сек" },
		{ "ayu_DurationSeconds#few", "{count} сек" },
		{ "ayu_DurationSeconds#many", "{count} сек" },
		{ "ayu_DurationSeconds#other", "{count} сек" },
		{ "ayu_DurationMinutes#one", "{count} мин" },
		{ "ayu_DurationMinutes#few", "{count} мин" },
		{ "ayu_DurationMinutes#many", "{count} мин" },
		{ "ayu_DurationMinutes#other", "{count} мин" },
		{ "ayu_DurationHours#one", "{count} ч" },
		{ "ayu_DurationHours#few", "{count} ч" },
		{ "ayu_DurationHours#many", "{count} ч" },
		{ "ayu_DurationHours#other", "{count} ч" },
		{ "ayu_HiddenChatsAbout", "Скрыть чат можно в его меню Чикенграм. Скрытые чаты пропадают из списка, и уведомления из них не приходят, пока вы не покажете их через Ctrl+Shift+P → «Показать скрытые чаты». Ctrl+Shift+H мгновенно прячет их снова. Это только скрытие в этом приложении, а не шифрование." },
		{ "ayu_HiddenChatsPinTitle", "PIN скрытых чатов" },
		{ "ayu_HiddenChatsCurrentPin", "Текущий PIN" },
		{ "ayu_HiddenChatsNewPin", "Новый PIN (4–12 цифр)" },
		{ "ayu_HiddenChatsRepeatPin", "Повторите новый PIN" },
		{ "ayu_HiddenChatsPinAbout", "PIN нужен, чтобы показать скрытые чаты. Если забудете его, удалите файл tdata/ayu/hidden_chats.json." },
		{ "ayu_HiddenChatsPinSaved", "PIN сохранён." },
		{ "ayu_HiddenChatsHideChat", "Скрыть чат" },
		{ "ayu_HiddenChatsUnhideChat", "Больше не скрывать" },
		{ "ayu_HiddenChatsHidden", "Чат скрыт. Чтобы увидеть его, нажмите Ctrl+Shift+P и выберите «Показать скрытые чаты»." },
		{ "ayu_HiddenChatsShownAgain", "Чат больше не скрыт." },
		{ "ayu_HiddenChatsShow", "Показать скрытые чаты" },
		{ "ayu_HiddenChatsLock", "Спрятать скрытые чаты" },
		{ "ayu_HiddenChatsUnlockTitle", "Показать скрытые чаты" },
		{ "ayu_HiddenChatsEnterPin", "PIN" },
		{ "ayu_HiddenChatsUnlockButton", "Показать" },
		{ "ayu_HiddenChatsUnlocked", "Скрытые чаты видны, пока вы не нажмёте Ctrl+Shift+H или не перезапустите приложение." },
		{ "ayu_HiddenChatsLocked", "Скрытые чаты снова спрятаны." },
		{ "ayu_KeywordAlertsTitle", "Слова-триггеры" },
		{ "ayu_KeywordAlertsPlaceholder", "Слова через запятую или с новой строки" },
		{ "ayu_KeywordAlertsAbout", "Вы получите уведомление, если в сообщении есть одно из этих слов, даже в заглушённых чатах." },
		{ "ayu_OcrCopyText", "Скопировать текст с картинки" },
		{ "ayu_OcrCopied", "Текст скопирован в буфер обмена." },
		{ "ayu_OcrNothingFound", "На этой картинке не найден текст." },
		{ "ayu_PaletteTitle", "Команды" },
		{ "ayu_PalettePlaceholder", "Начните вводить команду" },
		{ "ayu_PaletteAyuSettings", "Настройки Чикенграм" },
		{ "ayu_PaletteGhostMode", "Режим призрака вкл/выкл" },
		{ "ayu_PaletteGhostModeOn", "Режим призрака включён." },
		{ "ayu_PaletteGhostModeOff", "Режим призрака выключен." },
		{ "ayu_PalettePanic", "Паника: спрятать скрытые чаты" },
		{ "ayu_PaletteSearch", "Поиск по чатам и сообщениям" },
		{ "ayu_PaletteNewChat", "Новый чат" },
		{ "ayu_PaletteThemePrefix", "Тема" },
		{ "ayu_PaletteSwitchAccount", "Переключиться на" },
		{ "ayu_PaletteLock", "Заблокировать Чикенграм" },
		{ "ayu_PaletteKindChat", "Чат" },
		{ "ayu_PaletteKindGroup", "Группа" },
		{ "ayu_PaletteKindChannel", "Канал" },
		{ "ayu_PaletteKindBot", "Бот" },
		{ "ayu_ShortcutGlobalSearch", "Поиск по чатам и сообщениям" },
		{ "ayu_ShortcutNewChat", "Новый чат" },
		{ "ayu_SelfDestructTimer", "Таймер самоуничтожения" },
		{ "ayu_SelfDestructAbout", "Нажмите правой кнопкой на кнопку отправки и выберите «Отправить и удалить», чтобы сообщение удалилось у всех через это время. Работает, только пока приложение запущено." },
		{ "ayu_SendSelfDestruct", "Отправить и удалить через {duration}" },
		{ "ayu_ShortcutPanic", "Мгновенно спрятать скрытые чаты" },
		{ "ayu_ShortcutPalette", "Открыть команды Чикенграм" },
		{ "ayu_DesignHeader", "Дизайн" },
		{ "ayu_DesignThemeWebLight", "Web — светлая" },
		{ "ayu_DesignThemeWebDark", "Web — тёмная" },
		{ "ayu_DesignThemeWebDarkOrange", "Web — тёмная оранжевая" },
		{ "ayu_DesignThemeDawn", "Рассвет" },
		{ "ayu_DesignThemeSunset", "Закат" },
		{ "ayu_DesignThemeMidnight", "Полночь" },
		{ "ayu_DesignThemeGraphite", "Графит" },
		{ "ayu_DesignThemesAbout", "Темы новой дизайн-системы. Стандартные темы Telegram находятся в Настройки → Настройки чатов." },
		{ "ayu_DesignLayout", "Стиль интерфейса" },
		{ "ayu_DesignLayoutWeb", "Telegram Web" },
		{ "ayu_DesignLayoutClassic", "Классический" },
		{ "ayu_DesignGlass", "Эффект стекла" },
		{ "ayu_DesignGlassTint", "Плотность стекла" },
		{ "ayu_DesignGlassTintDense", "Плотнее" },
		{ "ayu_DesignGlassTintNormal", "Обычная" },
		{ "ayu_DesignGlassTintClear", "Прозрачнее" },
		{ "ayu_DesignGlassBlur", "Размытие стекла" },
		{ "ayu_DesignGlassBlurLight", "Слабое" },
		{ "ayu_DesignGlassBlurNormal", "Среднее" },
		{ "ayu_DesignGlassBlurStrong", "Сильное" },
		{ "ayu_PrivacyMode", "Приватный режим" },
		{ "ayu_PrivacyHiddenText", "Сообщение скрыто" },
		{ "ayu_PalettePrivacyOn", "Приватный режим включён." },
		{ "ayu_PalettePrivacyOff", "Приватный режим выключен." },
		{ "ayu_SectionsAdd", "Разделы" },
		{ "ayu_SectionPersonal", "Личные" },
		{ "ayu_SectionGroups", "Группы" },
		{ "ayu_SectionChannels", "Каналы" },
		{ "ayu_SectionBots", "Боты" },
		{ "ayu_SectionsAdded", "Разделы добавлены." },
		{ "ayu_SectionsLimit", "Достигнут лимит папок." },
		{ "ayu_PaletteSections", "Добавить разделы: Личные, Группы, Каналы, Боты" },
		{ "ayu_DesignSidebarTools", "«Избранное» и «Закладки» в панели папок" },
		{ "ayu_DesignSectionsButton", "Кнопка «Разделы» в панели папок" },
		{ "ayu_DesignAccent", "Цвет акцента" },
		{ "ayu_DesignAccentTheme", "Как в теме" },
		{ "ayu_DesignAccentBlue", "Синий" },
		{ "ayu_DesignAccentPurple", "Фиолетовый" },
		{ "ayu_DesignAccentGreen", "Зелёный" },
		{ "ayu_DesignAccentOrange", "Оранжевый" },
		{ "ayu_DesignAccentRed", "Красный" },
		{ "ayu_DesignAccentPink", "Розовый" },
		{ "ayu_DesignAccentCyan", "Бирюзовый" },
		{ "ayu_DesignEffects", "Эффекты Чикенграма" },
		{ "ayu_WebMenuMore", "Ещё" },
		{ "ayu_WebMenuClassic", "Классическое меню" },
		{ "ayu_WebSettingsAyuAbout", "Режим призрака, история сообщений, оформление" },
		{ "ayu_WebSettingsAccountAbout", "Имя, юзернейм, о себе" },
		{ "ayu_WebSettingsNotifyAbout", "Звуки, звонки, значки" },
		{ "ayu_WebSettingsPrivacyAbout", "Время захода, код-пароль, чёрный список" },
		{ "ayu_WebSettingsChatAbout", "Обои, темы, стикеры" },
		{ "ayu_WebSettingsFoldersAbout", "Сортировка чатов по папкам" },
		{ "ayu_WebSettingsAdvancedAbout", "Данные, память, подключение" },
		{ "ayu_WebSettingsDevicesAbout", "Микрофон, камера, звонки" },
		{ "ayu_WebSettingsPowerAbout", "Анимации и экономия заряда" },
		{ "ayu_DesignDensity", "Плотность интерфейса" },
		{ "ayu_DesignDensityCompact", "Компактная" },
		{ "ayu_DesignDensityNormal", "Обычная" },
		{ "ayu_DesignDensityComfortable", "Просторная" },
		{ "ayu_DesignCorners", "Скругления" },
		{ "ayu_DesignCornersTelegram", "Как в Telegram" },
		{ "ayu_DesignCornersSharp", "Острые" },
		{ "ayu_DesignCornersSoft", "Мягкие" },
		{ "ayu_DesignCornersRound", "Круглые" },
		{ "ayu_DesignMotion", "Анимации" },
		{ "ayu_DesignMotionFull", "Полные" },
		{ "ayu_DesignMotionReduced", "Сокращённые" },
		{ "ayu_DesignMotionOff", "Выключены" },
		{ "ayu_DesignWindowOpacity", "Прозрачность окна" },
		{ "ayu_DesignAbout", "Стиль интерфейса, плотность и скругления применяются после перезапуска. Прозрачность окна ниже 100% может немного увеличить нагрузку на процессор." },
		{ "ayu_SettingsWatermark", "Чикенграм основан на AyuGram от Radolyn Labs." },
	};
	return result;
}

} // namespace

QString Rebrand(const QString &key, QString value) {
	static const auto keepOriginal = base::flat_set<QString>{
		u"ayu_FirstLaunchAlert"_q,
		u"ayu_ExteraChatsAlert"_q,
		u"ayu_SupporterPopup"_q,
		u"ayu_OfficialResourcePopup"_q,
		u"ayu_SettingsWatermark"_q,
	};
	if (keepOriginal.contains(key.section(u'#', 0, 0))) {
		return value;
	}
	return value.replace(u"AyuGram"_q, Branding::Name());
}

void ApplyBuiltIn() {
	if (!IsRussian()) {
		return;
	}
	auto &instance = Lang::GetInstance();
	for (const auto &entry : Russian()) {
		instance.resetValue(entry.key);
		instance.applyValue(entry.key, entry.value);
	}
	instance.updatePluralRules();
}

} // namespace AyuFeatures::Translations
