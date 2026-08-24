/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "lang/lang_keys.h"
#include "settings/sections/settings_custom.h"

#include "custom_features/custom_settings.hpp"
#include "custom_features/in_game_overlay.hpp"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "settings/settings_common_session.h"
#include "settings/sections/settings_main.h"
#include "main/main_session.h"
#include "main/main_domain.h"
#include "storage/storage_domain.h"
#include "settings/sections/settings_local_passcode.h"
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
	builder.addSubsectionTitle(rpl::single(u"Ссылки и приватность"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/clean_urls"_q,
		.title = rpl::single(u"Очистка UTM-меток и трекеров"_q),
		.checked = CustomFeatures::GetConfig().cleanTrackingUrls,
		.keywords = { u"clean"_q, u"url"_q, u"utm"_q, u"tracking"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().cleanTrackingUrls = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/direct_links"_q,
		.title = rpl::single(u"Прямой переход по ссылкам"_q),
		.checked = CustomFeatures::GetConfig().directExternalLinks,
		.keywords = { u"links"_q, u"external"_q, u"direct"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().directExternalLinks = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/downloads_router"_q,
		.title = rpl::single(u"Умная сортировка загрузок по категориям"_q),
		.checked = CustomFeatures::GetConfig().enableDownloadsRouter,
		.keywords = { u"downloads"_q, u"router"_q, u"folders"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableDownloadsRouter = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Стикеры и медиа"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/unlimited_stickers"_q,
		.title = rpl::single(u"300 недавних стикеров"_q),
		.checked = CustomFeatures::GetConfig().unlimitedRecentStickers,
		.keywords = { u"stickers"_q, u"recent"_q, u"unlimited"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().unlimitedRecentStickers = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Гейминг и система"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/game_overlay"_q,
		.title = rpl::single(u"Игровой оверлей (Shift + ~)"_q),
		.checked = CustomFeatures::GetConfig().enableInGameOverlay,
		.keywords = { u"overlay"_q, u"game"_q, u"shift"_q, u"tilda"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableInGameOverlay = checked;
			CustomFeatures::GetConfig().save();
			CustomFeatures::InGameOverlayManager::Instance().updateState();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/game_status"_q,
		.title = rpl::single(u"Авто-статус запущенных игр"_q),
		.checked = CustomFeatures::GetConfig().enableGameStatus,
		.keywords = { u"game"_q, u"status"_q, u"activity"_q, u"cs2"_q, u"dota"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableGameStatus = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/auto_lock"_q,
		.title = rpl::single(u"Блокировка по Win + L"_q),
		.checked = CustomFeatures::GetConfig().autoLockOnWindowsLock,
		.keywords = { u"lock"_q, u"security"_q, u"win+l"_q, u"passcode"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().autoLockOnWindowsLock = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	builder.addButton({
		.title = rpl::single(u"Настроить код-пароль блокировки"_q),
		.icon = &st::menuIconLock,
		.onClick = [=] {
			if (builder.session()->domain().local().hasLocalPasscode()) {
				builder.showOther()(LocalPasscodeCheckId());
			} else {
				builder.showOther()(LocalPasscodeCreateId());
			}
		},
		.keywords = { u"passcode"_q, u"pin"_q, u"lock"_q, u"password"_q },
	});

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Интерфейс и стиль"_q));

	if (const auto check = builder.addCheckbox({
		.id = u"custom/mica_glass"_q,
		.title = rpl::single(u"Стеклянный эффект Windows 10/11 (Acrylic / Mica)"_q),
		.checked = CustomFeatures::GetConfig().enableMicaBackdrop,
		.keywords = { u"mica"_q, u"glass"_q, u"acrylic"_q, u"blur"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().enableMicaBackdrop = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/modern_rounded"_q,
		.title = rpl::single(u"Современные скругленные облачка сообщений"_q),
		.checked = CustomFeatures::GetConfig().modernRoundedStyle,
		.keywords = { u"style"_q, u"rounded"_q, u"bubble"_q, u"modern"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().modernRoundedStyle = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/hide_stories"_q,
		.title = rpl::single(u"Скрыть истории (Stories)"_q),
		.checked = CustomFeatures::GetConfig().hideStoriesBar,
		.keywords = { u"stories"_q, u"hide"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().hideStoriesBar = checked;
			CustomFeatures::GetConfig().save();
		}, check->lifetime());
	}

	if (const auto check = builder.addCheckbox({
		.id = u"custom/hide_ads"_q,
		.title = rpl::single(u"Скрыть рекламу в каналах и ботах"_q),
		.checked = CustomFeatures::GetConfig().hideSponsoredAds,
		.keywords = { u"ads"_q, u"hide"_q, u"sponsored"_q },
	})) {
		check->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			CustomFeatures::GetConfig().hideSponsoredAds = checked;
			CustomFeatures::GetConfig().save();
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
