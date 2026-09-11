/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/chat/chat_style_radius.h"
#include "ui/chat/chat_style.h"
#include "base/options.h"
#include "custom_features/custom_settings.hpp"

#include "ui/chat/chat_theme.h"
#include "ui/painter.h"
#include "ui/ui_utility.h"
#include "styles/style_chat.h"

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
	// CUSTOM: Use custom message border radius
	const int customRadius = CustomFeatures::GetConfig().messageBorderRadius;
	if (customRadius > 0 && customRadius <= 24) {
		return customRadius;
	}
	return st::bubbleRadiusSmall;
}

int BubbleRadiusLarge() {
	// CUSTOM: Use custom message border radius (slightly larger)
	const int customRadius = CustomFeatures::GetConfig().messageBorderRadius;
	if (customRadius > 0 && customRadius <= 24) {
		return std::min(customRadius + 4, 24); // Large = small + 4px, max 24px
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
	return st::msgFileThumbRadiusSmall;
}

int MsgFileThumbRadiusLarge() {
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
