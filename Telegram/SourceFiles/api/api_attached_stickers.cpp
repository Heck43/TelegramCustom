/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "api/api_attached_stickers.h"

#include "apiwrap.h"
#include "ui/boxes/confirm_box.h"
#include "boxes/sticker_set_box.h"
#include "boxes/stickers_box.h"
#include "data/data_document.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "data/stickers/data_stickers.h"
#include "lang/lang_keys.h"
#include "main/main_session.h"
#include "window/window_session_controller.h"

namespace Api {

AttachedStickers::AttachedStickers(not_null<ApiWrap*> api)
: _api(&api->instance()) {
}

void AttachedStickers::request(
		not_null<Window::SessionController*> controller,
		MTPmessages_GetAttachedStickers &&mtpRequest) {
	const auto weak = base::make_weak(controller);
	_api.request(_requestId).cancel();
	_requestId = _api.request(
		std::move(mtpRequest)
	).done([=](const MTPVector<MTPStickerSetCovered> &result) {
		_requestId = 0;
		const auto strongController = weak.get();
		if (!strongController) {
			return;
		}
		if (result.v.isEmpty()) {
			strongController->show(
				Ui::MakeInformBox(tr::lng_stickers_not_found()));
			return;
		} else if (result.v.size() > 1) {
			strongController->show(
				Box<StickersBox>(strongController->uiShow(), result.v));
			return;
		}
		// Single attached sticker pack.
		const auto data = result.v.front().match([&](const auto &data) {
			return &data.vset().data();
		});

		const auto setId = (data->vid().v && data->vaccess_hash().v)
			? StickerSetIdentifier{
				.id = data->vid().v,
				.accessHash = data->vaccess_hash().v }
			: StickerSetIdentifier{ .shortName = qs(data->vshort_name()) };
		strongController->show(Box<StickerSetBox>(
			strongController->uiShow(),
			setId,
			(data->is_emojis()
				? Data::StickersType::Emoji
				: data->is_masks()
				? Data::StickersType::Masks
				: Data::StickersType::Stickers)));
	}).fail([=] {
		_requestId = 0;
		if (const auto strongController = weak.get()) {
			strongController->show(
				Ui::MakeInformBox(tr::lng_stickers_not_found()));
		}
	}).send();
}

void AttachedStickers::requestAttachedStickerSets(
		not_null<Window::SessionController*> controller,
		not_null<PhotoData*> photo) {
	request(
		controller,
		MTPmessages_GetAttachedStickers(
			MTP_inputStickeredMediaPhoto(photo->mtpInput())));
}

void AttachedStickers::requestAttachedStickerSets(
		not_null<Window::SessionController*> controller,
		not_null<DocumentData*> document) {
	request(
		controller,
		MTPmessages_GetAttachedStickers(
			MTP_inputStickeredMediaDocument(document->mtpInput())));
}

void AttachedStickers::requestSimilarStickerSets(
		not_null<Window::SessionController*> controller,
		not_null<DocumentData*> document) {
	const auto sticker = document->sticker();
	if (!sticker) {
		return;
	}

	QString query = sticker->alt.trimmed();
	if (query.isEmpty()) {
		if (const auto emojiList = document->owner().stickers().getEmojiListFromSet(document)) {
			if (!emojiList->empty()) {
				query = (*emojiList)[0]->text();
			}
		}
	}
	if (query.isEmpty()) {
		query = u"stickers"_q;
	}

	const auto weak = base::make_weak(controller);
	_api.request(_requestId).cancel();
	_requestId = _api.request(MTPmessages_SearchStickerSets(
		MTP_flags(0),
		MTP_string(query),
		MTP_long(0)
	)).done([=](const MTPmessages_FoundStickerSets &result) {
		_requestId = 0;
		const auto strongController = weak.get();
		if (!strongController) {
			return;
		}

		result.match([&](const MTPDmessages_foundStickerSetsNotModified &) {
			strongController->show(Ui::MakeInformBox(tr::lng_stickers_not_found()));
		}, [&](const MTPDmessages_foundStickerSets &data) {
			auto &owner = strongController->session().data();
			std::vector<uint64> foundSetIds;
			for (const auto &setData : data.vsets().v) {
				const auto set = owner.stickers().feedSet(setData);
				if (!set->stickers.empty() || !set->covers.empty()) {
					foundSetIds.push_back(set->id);
				}
			}

			if (foundSetIds.empty()) {
				strongController->show(Ui::MakeInformBox(tr::lng_stickers_not_found()));
			} else {
				const auto title = QString(u"Похожие стикерпаки (%1)"_q).arg(query);
				strongController->show(Box<StickersBox>(
					strongController->uiShow(),
					foundSetIds,
					title));
			}
		});
	}).fail([=] {
		_requestId = 0;
		if (const auto strongController = weak.get()) {
			strongController->show(Ui::MakeInformBox(tr::lng_stickers_not_found()));
		}
	}).send();
}

} // namespace Api
