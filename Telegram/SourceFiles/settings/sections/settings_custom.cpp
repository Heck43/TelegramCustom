/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_custom.h"

#include "custom_features/custom_settings.hpp"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
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
	.title = &tr::lng_settings_section_general,
	.icon = &st::menuIconCustomize,
}, [](SectionBuilder &builder) {
	builder.addSubsectionTitle(rpl::single(u"Ссылки и сеть"_q));

	builder.addCheckbox({
		.id = u"custom/clean_urls"_q,
		.title = rpl::single(u"Авто-очистка трекеров и UTM-меток из ссылок"_q),
		.checked = CustomFeatures::GetConfig().cleanTrackingUrls,
		.keywords = { u"clean"_q, u"url"_q, u"utm"_q, u"tracking"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().cleanTrackingUrls = checked;
	}, builder.container()->lifetime());

	builder.addCheckbox({
		.id = u"custom/direct_links"_q,
		.title = rpl::single(u"Прямой переход по ссылкам без окон подтверждения"_q),
		.checked = CustomFeatures::GetConfig().directExternalLinks,
		.keywords = { u"links"_q, u"external"_q, u"direct"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().directExternalLinks = checked;
	}, builder.container()->lifetime());

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Стикеры и медиа"_q));

	builder.addCheckbox({
		.id = u"custom/unlimited_stickers"_q,
		.title = rpl::single(u"Бесконечные недавние стикеры (без ограничения)"_q),
		.checked = CustomFeatures::GetConfig().unlimitedRecentStickers,
		.keywords = { u"stickers"_q, u"recent"_q, u"unlimited"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().unlimitedRecentStickers = checked;
	}, builder.container()->lifetime());

	builder.addCheckbox({
		.id = u"custom/speedboost"_q,
		.title = rpl::single(u"Многопоточный SpeedBoost для скачивания файлов"_q),
		.checked = CustomFeatures::GetConfig().speedBoostEnabled,
		.keywords = { u"speed"_q, u"download"_q, u"boost"_q, u"threads"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().speedBoostEnabled = checked;
	}, builder.container()->lifetime());

	builder.addDivider();
	builder.addSubsectionTitle(rpl::single(u"Интерфейс"_q));

	builder.addCheckbox({
		.id = u"custom/compact_folders"_q,
		.title = rpl::single(u"Компактная колонка папок (стиль Discord/Web)"_q),
		.checked = CustomFeatures::GetConfig().compactFolderSidebar,
		.keywords = { u"compact"_q, u"folders"_q, u"sidebar"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().compactFolderSidebar = checked;
	}, builder.container()->lifetime());

	builder.addCheckbox({
		.id = u"custom/hide_stories"_q,
		.title = rpl::single(u"Скрыть панель историй (Stories)"_q),
		.checked = CustomFeatures::GetConfig().hideStoriesBar,
		.keywords = { u"stories"_q, u"hide"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().hideStoriesBar = checked;
	}, builder.container()->lifetime());

	builder.addCheckbox({
		.id = u"custom/hide_ads"_q,
		.title = rpl::single(u"Скрыть спонсированные рекламные посты"_q),
		.checked = CustomFeatures::GetConfig().hideSponsoredAds,
		.keywords = { u"ads"_q, u"hide"_q, u"sponsored"_q },
	})->checkedChanges(
	) | rpl::start_with_next([=](bool checked) {
		CustomFeatures::GetConfig().hideSponsoredAds = checked;
	}, builder.container()->lifetime());
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
