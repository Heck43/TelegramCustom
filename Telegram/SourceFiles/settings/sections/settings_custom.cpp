/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_custom.h"

#include "custom_features/custom_settings.hpp"
#include "custom_features/clean_urls.hpp"
#include "settings/settings_builder.h"
#include "settings/settings_common_session.h"
#include "ui/widgets/checkbox.h"
#include "ui/vertical_list.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

using namespace Builder;

class Section : public AbstractSection {
public:
	Section(
		QWidget *parent,
		not_null<Window::SessionController*> controller)
	: AbstractSection(parent)
	, _controller(controller) {
		setupContent();
	}

	[[nodiscard]] rpl::producer<QString> title() override {
		return rpl::single(u"Кастомные функции"_q);
	}

private:
	void setupContent() {
		const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

		SectionBuilder builder(WidgetContext{
			.container = content,
			.controller = _controller,
			.showOther = [=](Type type) { _controller->showSettings(type); },
			.isPaused = [=] { return _controller->isPaused(); },
		});

		buildSection(builder);

		Ui::ResizeFitChild(this, content);
	}

	void buildSection(SectionBuilder &builder) {
		auto &cfg = CustomFeatures::GetConfig();

		// --- Раздел 1: Ссылки и Приватность ---
		builder.addSubsectionTitle(rpl::single(u"Ссылки и безопасность"_q));

		auto cleanUrls = builder.addCheckbox({
			.id = u"custom_clean_urls"_q,
			.title = rpl::single(u"Авто-очистка UTM и трекинг-меток из ссылок"_q),
			.checked = cfg.cleanTrackingUrls,
		});
		cleanUrls->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.cleanTrackingUrls = checked;
		}, builder.container()->lifetime());

		auto directLinks = builder.addCheckbox({
			.id = u"custom_direct_links"_q,
			.title = rpl::single(u"Прямой переход по внешним ссылкам без окна подтверждения"_q),
			.checked = cfg.directExternalLinks,
		});
		directLinks->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.directExternalLinks = checked;
		}, builder.container()->lifetime());

		builder.addDivider();

		// --- Раздел 2: Стикеры и Сеть ---
		builder.addSubsectionTitle(rpl::single(u"Стикеры и загрузка файлов"_q));

		auto unlimitedStickers = builder.addCheckbox({
			.id = u"custom_unlimited_stickers"_q,
			.title = rpl::single(u"Бесконечные недавние стикеры (без вытеснения)"_q),
			.checked = cfg.unlimitedRecentStickers,
		});
		unlimitedStickers->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.unlimitedRecentStickers = checked;
		}, builder.container()->lifetime());

		auto speedBoost = builder.addCheckbox({
			.id = u"custom_speedboost"_q,
			.title = rpl::single(u"Многопоточный SpeedBoost загрузки (8+ потоков)"_q),
			.checked = cfg.speedBoostEnabled,
		});
		speedBoost->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.speedBoostEnabled = checked;
		}, builder.container()->lifetime());

		builder.addDivider();

		// --- Раздел 3: Навигация в чатах ---
		builder.addSubsectionTitle(rpl::single(u"Навигация и чаты"_q));

		auto firstMsg = builder.addCheckbox({
			.id = u"custom_first_message"_q,
			.title = rpl::single(u"Кнопка быстрого перехода «К первому сообщению»"_q),
			.checked = cfg.showFirstMessageButton,
		});
		firstMsg->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.showFirstMessageButton = checked;
		}, builder.container()->lifetime());

		auto collapsePins = builder.addCheckbox({
			.id = u"custom_collapse_pins"_q,
			.title = rpl::single(u"Возможность сворачивать закрепленные сообщения"_q),
			.checked = cfg.collapsePinnedMessages,
		});
		collapsePins->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.collapsePinnedMessages = checked;
		}, builder.container()->lifetime());

		builder.addDivider();

		// --- Раздел 4: Внешний вид (UI) ---
		builder.addSubsectionTitle(rpl::single(u"Интерфейс"_q));

		auto compactFolders = builder.addCheckbox({
			.id = u"custom_compact_folders"_q,
			.title = rpl::single(u"Компактный вертикальный сайдбар папок (стиль Discord)"_q),
			.checked = cfg.compactFolderSidebar,
		});
		compactFolders->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.compactFolderSidebar = checked;
		}, builder.container()->lifetime());

		auto hideStories = builder.addCheckbox({
			.id = u"custom_hide_stories"_q,
			.title = rpl::single(u"Скрыть панель Историй (Stories) сверху"_q),
			.checked = cfg.hideStoriesBar,
		});
		hideStories->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.hideStoriesBar = checked;
		}, builder.container()->lifetime());

		auto hideAds = builder.addCheckbox({
			.id = u"custom_hide_ads"_q,
			.title = rpl::single(u"Отключить официальную рекламу Telegram"_q),
			.checked = cfg.hideSponsoredAds,
		});
		hideAds->checkedChanges() | rpl::start_with_next([&](bool checked) {
			cfg.hideSponsoredAds = checked;
		}, builder.container()->lifetime());
	}

	const not_null<Window::SessionController*> _controller;
};

struct SectionFactory : AbstractSectionFactory {
	object_ptr<AbstractSection> create(
			QWidget *parent,
			not_null<Window::SessionController*> controller,
			Type type,
			const SectionArgs &args) const override {
		return object_ptr<Section>(parent, controller);
	}
	[[nodiscard]] bool hasCustomTopBar() const override {
		return false;
	}
};

} // namespace

Type CustomSettingsId() {
	static const auto result = std::make_shared<SectionFactory>();
	return result;
}

} // namespace Settings
