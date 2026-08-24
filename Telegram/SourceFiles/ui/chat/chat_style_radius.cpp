/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/chat_style_radius.h"
#include "ui/chat/chat_style.h"
#include "base/options.h"

#include "ui/chat/chat_theme.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "styles/style_chat.h"
#include "custom_features/custom_settings.hpp"

namespace Ui {
namespace {

base::options::toggle UseSmallMsgBubbleRadius({
	.id = kOptionUseSmallMsgBubbleRadius,
	.name = "Use small message bubble radius",
	.description = "Makes most message bubbles square-ish.",
	.restartRequired = true,
});

} // namespace

const char kOptionUseSmallMsgBubbleRadius[] = "use-small-msg-bubble-radius";

int BubbleRadiusSmall() {
	if (CustomFeatures::GetConfig().modernRoundedStyle) {
		return std::max(int(st::bubbleRadiusSmall), style::ConvertScale(10));
	}
	return st::bubbleRadiusSmall;
}

int BubbleRadiusLarge() {
	if (CustomFeatures::GetConfig().modernRoundedStyle) {
		return std::max(int(st::bubbleRadiusLarge), style::ConvertScale(18));
	}
	static const auto result = [] {
		if (UseSmallMsgBubbleRadius.value()) {
			return st::bubbleRadiusSmall;
		} else {
			return st::bubbleRadiusLarge;
		}
	}();
	return result;
}

int MsgFileThumbRadiusSmall() {
	if (CustomFeatures::GetConfig().modernRoundedStyle) {
		return std::max(int(st::msgFileThumbRadiusSmall), style::ConvertScale(8));
	}
	return st::msgFileThumbRadiusSmall;
}

int MsgFileThumbRadiusLarge() {
	if (CustomFeatures::GetConfig().modernRoundedStyle) {
		return std::max(int(st::msgFileThumbRadiusLarge), style::ConvertScale(14));
	}
	static const auto result = [] {
		if (UseSmallMsgBubbleRadius.value()) {
			return st::msgFileThumbRadiusSmall;
		} else {
			return st::msgFileThumbRadiusLarge;
		}
	}();
	return result;
}

} // namespace Ui
