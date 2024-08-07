# v1.3.0

- Added macOS support (thx [ninxout](https://github.com/ninXout), [raydeeux](https://github.com/RayDeeUx), and [hiimjustin000](https://github.com/hiimjustin000))
- Added support for `.wav`, `.ogg`/`.oga`, and `.flac` files for custom songs
- Included artist name for all "Now Playing" notifications
- Set Z order of the "Now Playing" notification to 200 for easier readability against most MenuLayer mods (such as Overcharged Main Menu)
- Shuffling the song now generates a new "Now Playing" notification
- Increase "Now Playing" notification maximum duration to 5 seconds
- Add better slider control for adjusting notification duration
- Added Node IDs to nodes added by this mod
- Squash a few bugs, optimize a few things, minimize likelihood of potential crashes

# v1.2.2

- Added an option to enable the shuffle button (thx [reinmar](https://github.com/Reinmmar))

# v1.2.1

- Fixed crash caused by Newgrounds manager trying to gather info of default menuLoop.mp3

# v1.2.0

- Added `Randomize on editor exit` setting.
- Added `Notification time` setting.
- Added `Use custom songs path` setting.
- Added `SongManager.hpp`
- Added custom songs support.
- Added `stoi` to `Utils` (prolly it crashed because of Autonong)
- Added a button to add custom songs (present in the settings tab)
- Added a button for randomizing the song manually.
- Most of the functions related to the `m_songs` vector are now in `SongManager`
- Fixed crash when there are no songs (original menuLoop plays instead)
- Fixed crash caused by switching the songs location.
- Modified `PlayingCard` to fit the new system.
- Now the song randomizes every time you change `Use custom songs path`

# v1.1.0

- Fixed crash when opening the game (missing textures crash)
- Fixed random song picker being on a loop for every song downloaded.
- Moved song class to its own file.
- The mod should now check for the songs path when loaded, not when executed.
- Added setting to randomize song on level exit.
- Fixed Android crash caused by not able to access the memory (f*** pointers)
- Added `Utils.hpp`

# v1.1.0-alpha.5

- Updated Geode target version to `v3.0.0-beta.1`

# v1.1.0-alpha.4

- Updated notification card & position.
- Added the property `name` to the `Song` class.

# v1.1.0-alpha.3

- Added a notification animation for the card.
- The position is now added in `MainLayer` instead of the layer itself.

# v1.1.0-alpha.2

- Fixed crash on startup caused by a missing dependency.

# v1.1.0-alpha.1

- Added a card showing which song is playing (you can disable it in settings)
- Changed a little bit how songs are stored in the vector.
- Removed MacOS support (sorry, i don't wanna deal with MacOS bindings.)

### Notes

- This version is for `Geode v3.0.0-alpha.2` only.

# v1.0.0

- Initial release
