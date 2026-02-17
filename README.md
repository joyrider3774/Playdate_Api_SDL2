# Playdate Api SDL2
An attempt to reimplement the Playdate Handheld C Api in SDL2

i've used this to create windows (and linux) binaries to for my playdate game's from unaltered source code for the playdate. I attempted to reimplement the Playdate API headers in SDL2. It's probably done in a very bad way but it does seem to work for my games and a tetris game i tested from someone else on github. A Lot is still not implemented and probably never will. A basic sprite class has been implemented as well as collision detection using bump.hpp (https://github.com/Polynominal/bump.hpp), The C Sprite example is working but a lot of the other games i tried do not work correctly. Audio is also only basic implementations for File and SamplePlayer, File routines have been implemented as well as well as some of the display and graphics functions. There might still be some issues with some of the graphics functions compared to playdate but for my game's i'm happy with it

# how to
1. place your playdate game's unaltered source code files inside src/srcgame
2. place your playdate game's Source directory (containing assets) inside Source + make sure it contains a pdxinfo file with a bundleID set (it's used to determine unique save folder)
3. Convert fonts (fnt+png) files to TTF files by using [bitsnpicas](https://github.com/kreativekorp/bitsnpicas) to export to BDF file, then open BDF file in [fontforge studio](https://fontforge.org/en-US/) and export to ttf. So that the ttf font files match exactly as on the playdate, the export from bitsnpicas to ttf does not help here as it does not export in same size fonts as used on playdate. Another more easier way to convert the playdate fnt files to loadable ttf files with same size is to open the playdate font in bitsnpicas and then inside bitsnpicase export the font as a windows 3.1 fnt file. Then rename the fnt files to .ttf. This is what i'm doing lately and (recent) SDL2_ttf (versions) seems to be able to load these fonts as well.
4. Compile using the provided makefile.
5. if all goes well compiled binary should be named game(.exe) in Source directory

# colorize your games
if you place a "Source1" or "Source2", ... folder inside the "Source" folder you can swap source folders by press F3 when the game is running.
inside these additional source folders, you replicate the same folder structure as in the original Source folder but you can use different files for example colorized graphics.
in order for this to work correctly you need to make sure that you implemented the kEventTerminate event and free your game's assets / alloced memory when it's called, otherwise you'll have memory leaks.
It works by first calling kEventTerminate and then kEventInit again so you need to make sure that you also reset global variables inside kEventInit.
you can also provide a colors.ini file to determine the colors used for black and white. You need to make sure that the clear color lies between black and white. For an example on this i suggest to check some of my playdate game repo's.

# credits
- Playdate api headers (contained in PD_API directory) are copyright by panic inc
- bump.hpp by Polynominal (https://github.com/Polynominal/bump.hpp)

# example games
- collection of playdate games, including mine compiled with this api for html: https://joyrider3774.github.io/playdate_games_html/
- Poker Poker Magic: https://rapcal.itch.io/poker-poker-magic

