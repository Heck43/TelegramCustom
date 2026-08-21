/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "lang/lang_keys.h"
#include "settings/sections/settings_custom.h"

#include "custom_features/custom_settings.hpp"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "styles/style_boxes.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

class CustomSection : public Section<CustomSection> {
public:
	CustomSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

const auto kMeta = BuildHelper({
	.id = CustomSection::Id(),
	.parentId = MainId(),
	.title = &tr::lng_settings_features,
	.icon = &st::menuIconCustomize,
}, [](SectionBuilder &builder) {
	builder.addSubsectionTitle(rpl::single(u"Ссылки и сеть"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/clean_urls"_q,
		.title = rpl::single(u"Авто-очистка трекеров и UTM-меток из ссылок"_q),
		.checked = CustomFeatures::GetConfig().cleanTrackingUrls,
		.keywords = { u"clean"_q, u"url"_q, u"utm"_q, u"tracking"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().cleanTrackingUrls = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/direct_links"_q,
		.title = rpl::single(u"Прямой переход по ссылкам без окон подтверждения"_q),
		.checked = CustomFeatures::GetConfig().directExternalLinks,
		.keywords = { u"links"_q, u"external"_q, u"direct"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().directExternalLinks = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/speedboost"_q,
		.title = rpl::single(u"Многопоточный SpeedBoost для скачивания файлов"_q),
		.checked = CustomFeatures::GetConfig().speedBoostEnabled,
		.keywords = { u"speed"_q, u"download"_q, u"boost"_q, u"threads"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().speedBoostEnabled = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/downloads_router"_q,
		.title = rpl::single(u"Умная сортировка файлов по системным папкам (Фото, Музыка, Документы)"_q),
		.checked = CustomFeatures::GetConfig().enableDownloadsRouter,
		.keywords = { u"downloads"_q, u"router"_q, u"folders"_q, u"music"_q, u"pictures"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableDownloadsRouter = checked;
		}, check->lifetime());
	}

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Стикеры и медиа"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/unlimited_stickers"_q,
		.title = rpl::single(u"Расширенный список недавних стикеров (до 1000 шт.)"_q),
		.checked = CustomFeatures::GetConfig().unlimitedRecentStickers,
		.keywords = { u"stickers"_q, u"recent"_q, u"unlimited"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().unlimitedRecentStickers = checked;
		}, check->lifetime());
	}

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Гейминг и Windows"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/game_overlay"_q,
		.title = rpl::single(u"Внутриигровой оверлей поверх игр (Shift + ~)"_q),
		.checked = CustomFeatures::GetConfig().enableInGameOverlay,
		.keywords = { u"overlay"_q, u"game"_q, u"shift"_q, u"tilda"_q, u"steam"_q, u"discord"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableInGameOverlay = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/game_status"_q,
		.title = rpl::single(u"Авто-статус игры в профиле (CS2, Dota 2, Cyberpunk и др.)"_q),
		.checked = CustomFeatures::GetConfig().enableGameStatus,
		.keywords = { u"game"_q, u"status"_q, u"activity"_q, u"cs2"_q, u"dota"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableGameStatus = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/auto_lock"_q,
		.title = rpl::single(u"Авто-блокировка Telegram при блокировке экрана (Win + L)"_q),
		.checked = CustomFeatures::GetConfig().autoLockOnWindowsLock,
		.keywords = { u"lock"_q, u"security"_q, u"win+l"_q, u"passcode"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().autoLockOnWindowsLock = checked;
		}, check->lifetime());
	}

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Интерфейс"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/compact_folders"_q,
		.title = rpl::single(u"Компактная колонка папок (стиль Discord/Web)"_q),
		.checked = CustomFeatures::GetConfig().compactFolderSidebar,
		.keywords = { u"compact"_q, u"folders"_q, u"sidebar"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().compactFolderSidebar = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/mica_glass"_q,
		.title = rpl::single(u"Стеклянный эффект Windows 11 (Mica / Acrylic Glass)"_q),
		.checked = CustomFeatures::GetConfig().enableMicaBackdrop,
		.keywords = { u"mica"_q, u"glass"_q, u"acrylic"_q, u"blur"_q, u"windows 11"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableMicaBackdrop = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/hide_stories"_q,
		.title = rpl::single(u"Скрыть панель историй (Stories)"_q),
		.checked = CustomFeatures::GetConfig().hideStoriesBar,
		.keywords = { u"stories"_q, u"hide"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().hideStoriesBar = checked;
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/hide_ads"_q,
		.title = rpl::single(u"Скрыть спонсированные рекламные посты"_q),
		.checked = CustomFeatures::GetConfig().hideSponsoredAds,
		.keywords = { u"ads"_q, u"hide"_q, u"sponsored"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().hideSponsoredAds = checked;
		}, check->lifetime());
	}
});

const SectionBuildMethod kCustomSection = kMeta.build;

CustomSection::CustomSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> CustomSection::title() {
	return rpl::single(u"Кастомные функции"_q);
}

void CustomSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	build(content, kCustomSection);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type CustomSettingsId() {
	return CustomSection::Id();
}

} // namespace Settings
