#include "SongManager.hpp"
#include "Utils.hpp"
#include "ui/PlayingCard.hpp"
#include <Geode/loader/SettingEvent.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/loader/Dirs.hpp>

using namespace geode::prelude;

SongManager &songManager = SongManager::get();

struct GameManagerHook : Modify<GameManagerHook, GameManager> {
	gd::string getMenuMusicFile() {
		return songManager.getCurrentSong();
	}
};

struct PlayLayerHook : Modify<PlayLayerHook, PlayLayer> {
	void onQuit() {
		if (Mod::get()->getSettingValue<bool>("randomizeWhenExitingLevel"))
			songManager.pickRandomSong();

		PlayLayer::onQuit();
	}
};

struct EditorPauseLayerHook : Modify<EditorPauseLayerHook, EditorPauseLayer> {
	#ifndef __APPLE__
	void onExitEditor(CCObject *sender) {
		if (Mod::get()->getSettingValue<bool>("randomizeWhenExitingEditor"))
			songManager.pickRandomSong();

		EditorPauseLayer::onExitEditor(sender);
	}
	#else
	/*
	this section is for macOS (both intel and ARM). remarks:
	- don't hook onSaveAndPlay; that goes to playlayer
	- don't hook onSave or the FLAlertLayer from it; that does not exit the editor
	- can't hook onExitEditor for macOS, due to aggressive inlining from robtop/appleclang
		(yes, nin. i know the address exists; justin found those addresses for me.
		but i keep getting the same song five times in a row if i hook onExitEditor
		for macos, and i don't think any of us have the same RNG seed as Dream.)
		[for the record, i had 29 possible audio files total while doing this.
		1/29 = 0.0344827586207. (1/29)^5 = 4.87539727785E-8 = 1/20511149.
		let that sink in for a moment.]
	-- raydeeux
	*/
	void onSaveAndExit(CCObject *sender) {
		if (Mod::get()->getSettingValue<bool>("randomizeWhenExitingEditor"))
			songManager.pickRandomSong();

		EditorPauseLayer::onSaveAndExit(sender);
	}
	void FLAlert_Clicked(FLAlertLayer* p0, bool btnTwo) {
		bool isQualifedAlert = false;
		// determine if the FLAlertLayer being clicked on is the one from onExitNoSave
		/*
		hooking FLAlertLayer::init() and then storing its desc param is also an option,
		but i'm not sure what this mod's stance is with global variables.
		consider this overkill, but it gets the job done.

		i wanted to use getChildOfType to reduce the line count but ran into one of those
		C-Tidy/Clang red squiggly lines about typeinfo_cast or whatever and got worried,
		so all you get is this.

		yep, nested forloops and typeinfo_cast calls for days.

		if anyone has a shorter solution that still hooks this function, go ahead.

		for reference, unformatted FLAlertLayer main text is:
		R"(Exit without saving? All unsaved changes will be lost!)"
		-- raydeeux
		*/
		auto tArea = p0->m_mainLayer->getChildByIDRecursive("content-text-area");
		if (auto textArea = typeinfo_cast<TextArea*>(tArea)) {
			for (auto node : CCArrayExt<CCNode*>(textArea->getChildren())) {
				if (typeinfo_cast<MultilineBitmapFont*>(node)) {
					for (auto nodeTwo : CCArrayExt<CCNode*>(node->getChildren())) {
						if (auto label = typeinfo_cast<CCLabelBMFont*>(nodeTwo)) {
							auto labelString = std::string(label->getString());
							isQualifedAlert = labelString == R"(Exit without saving? All unsaved changes will be lost!)";
							log::debug("labelString: {}", labelString); // log::debug calls since that's kinda this mod's thing
							break;
						}
					}
				}
			}
		}

		log::debug("isQualifedAlert: {}", isQualifedAlert); // log::debug calls since that's kinda this mod's thing
		log::debug("btnTwo: {}", btnTwo); // log::debug calls since that's kinda this mod's thing

		if (Mod::get()->getSettingValue<bool>("randomizeWhenExitingEditor") && isQualifedAlert && btnTwo)
			songManager.pickRandomSong();

		EditorPauseLayer::FLAlert_Clicked(p0, btnTwo);
	}
	#endif
};

struct MenuLayerHook : Modify<MenuLayerHook, MenuLayer> {
	bool init() {
		if (!MenuLayer::init())
			return false;

		// return early if notif card setting is disabled, reducing indentation
		if (!Mod::get()->getSettingValue<bool>("enableNotification"))
			return true;

		MenuLayerHook::generateNotifcation();

		// add a shuffle button
		if (!Mod::get()->getSettingValue<bool>("enableShuffleButton"))
			return true;

		MenuLayerHook::addShuffleButton();

		return true;
	}

	void generateNotifcation() {
		auto downloadManager = MusicDownloadManager::sharedState();
		// create notif card stuff
		auto screenSize = CCDirector::get()->getWinSize();

		auto songFileName = std::filesystem::path(songManager.getCurrentSong()).filename();

		std::string notifString = "Now playing: ";

		if (Mod::get()->getSettingValue<bool>("useCustomSongs")) {
			notifString = notifString.append(songFileName.string());
		} else {
			// in case that the current file selected is the original menuloop, don't gather any info
			if (songManager.isOriginalMenuLoop()) {
				notifString = notifString.append("Original Menu Loop by RobTop");
			} else {
				log::info("attempting to play {}", songFileName.string());
				// if its not menuLoop.mp3, then get info
				size_t dotPos = songFileName.string().find_last_of('.');

				if (dotPos == std::string::npos) {
					notifString = notifString.append("Unknown");
				} else {
					songFileName = songFileName.string().substr(0, dotPos);

					auto songInfo = downloadManager->getSongInfoObject(Utils::stoi(songFileName.string()));

					// sometimes songInfo is nullptr, so improvise
					if (songInfo) {
						notifString = notifString.append(fmt::format("{} by {} ({})", songInfo->m_songName, songInfo->m_artistName, songInfo->m_songID));
					} else {
						notifString = notifString.append(songFileName.string());
					}
				}
			}
		}

		auto card = PlayingCard::create(notifString);

		card->position.x = screenSize.width / 2.0f;
		card->position.y = screenSize.height;

		auto defaultPos = card->position;
		auto posx = defaultPos.x;
		auto posy = defaultPos.y;

		card->setPosition(defaultPos);
		card->setZOrder(200);
		card->setID("now-playing"_spr);
		this->addChild(card);

		auto sequence = CCSequence::create(
			CCEaseInOut::create(CCMoveTo::create(1.5f, {posx, posy - 24.0f}), 2.0f),
			CCDelayTime::create(Mod::get()->getSettingValue<double>("notificationTime")),
			CCEaseInOut::create(CCMoveTo::create(1.5f, {posx, posy}), 2.0f),
			nullptr
		);
		card->runAction(sequence);
	}

	void addShuffleButton() {
		auto menu = getChildByID("right-side-menu");

		auto btn = CCMenuItemSpriteExtra::create(
			CircleButtonSprite::create(
				CCSprite::create("shuffle-btn-sprite.png"_spr)
			),
			this,
			menu_selector(MenuLayerHook::onShuffleBtn)
		);
		btn->setID("shuffle-button"_spr);

		menu->addChild(btn);
		menu->updateLayout();
	}

	void onShuffleBtn(CCObject *sender) {
		if (auto card = getChildByIDRecursive("now-playing"_spr)) {
			card->removeMeAndCleanup();
		}
		FMODAudioEngine::sharedEngine()->m_backgroundMusicChannel->stop();
		songManager.pickRandomSong();
		GameManager::sharedState()->playMenuMusic();
		MenuLayerHook::generateNotifcation();
	}
};

struct OptionsLayerHook : Modify<OptionsLayerHook, OptionsLayer> {
	void customSetup() {
		OptionsLayer::customSetup();

		// add the folder btn to the settings layer
		auto menu = CCMenu::create();

		auto btn = CCMenuItemSpriteExtra::create(
			geode::CircleButtonSprite::create(
				CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png"),
				geode::CircleBaseColor::Pink
			),
			this,
			menu_selector(OptionsLayerHook::onSongsBtn)
		);

		btn->setPosition({-144.0f, -60.0f});
		btn->m_scaleMultiplier = 1.1f;
		btn->setID("songs-button"_spr);

		menu->addChild(btn);
		menu->setID("songs-button-menu"_spr);

		m_mainLayer->addChild(menu);
	}

	void onSongsBtn(CCObject *sender) {
		utils::file::openFolder(Mod::get()->getConfigDir());
	}
};

bool isSupportedExtension(std::string path) {
	return path.ends_with(".mp3") || path.ends_with(".wav") || path.ends_with(".ogg") || path.ends_with(".oga") || path.ends_with(".flac");
}

void populateVector(bool customSongs) {
	/*
		if custom songs are enabled search for files in the config dir
		if not, just use the newgrounds songs
	*/
	if (customSongs) {
		auto configPath = geode::Mod::get()->getConfigDir();

		for (auto file : std::filesystem::directory_iterator(configPath)) {
			auto filePathString = file.path().string();
			if (isSupportedExtension(filePathString)) {
				log::debug("Adding custom song: {}", file.path().filename().string());
				songManager.addSong(filePathString);
			}
		}
	} else {
		auto downloadManager = MusicDownloadManager::sharedState();

		// for every downloaded song push it to the m_songs vector
		/*
		getDownloadedSongs() function call binding for macOS found
		from ninXout (ARM) and hiimjustin000 (Intel + verification)
		*/
		CCArrayExt<SongInfoObject *> songs = downloadManager->getDownloadedSongs();
		for (auto song : songs) {
			std::string songPath = downloadManager->pathForSong(song->m_songID);

			if (!isSupportedExtension(songPath))
				continue;

			log::debug("Adding Newgrounds song: {}", songPath);
			songManager.addSong(songPath);
		}
		// same thing as NG but for music library as well --ninXout
		std::filesystem::path mlsongs = geode::dirs::getGeodeDir().parent_path() / "Resources" / "songs";
		for (const std::filesystem::path& dirEntry : std::filesystem::recursive_directory_iterator(mlsongs)) {
			std::string songPath = dirEntry.string();

			if (!isSupportedExtension(songPath))
				continue;

			log::debug("Adding Music Library song: {}", songPath);
			songManager.addSong(songPath);
		}
	}
}

$on_mod(Loaded) {
	populateVector(Mod::get()->getSettingValue<bool>("useCustomSongs"));

	songManager.pickRandomSong();
}

$execute {
	listenForSettingChanges<bool>("useCustomSongs", [](bool value) {
		// make sure m_songs is empty, we don't want to make a mess here
		songManager.clearSongs();

		/*
			for every custom song file, push its path to m_songs
			if they're ng songs also push the path bc we're going to use getPathForSong
		*/
		populateVector(value);

		// change the song when you click apply, stoi will not like custom names.

		FMODAudioEngine::sharedEngine()->m_backgroundMusicChannel->stop();
		songManager.pickRandomSong();
		GameManager::sharedState()->playMenuMusic();
	});
}