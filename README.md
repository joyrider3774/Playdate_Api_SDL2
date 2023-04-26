# Playdate Api SDL2
An attempt to reimplement the Playdate Handheld C Api in SDL2

i've used this to create windows (and linux) binaries to for my playdate game's from unaltered source code for the playdate. I attempted to reimplement the Playdate API headers in SDL2. It's probably done in a very bad way but it does seem to work for my games and a tetris game i tested from someone else on github. A Lot is still not implemented and probably never will. A basic sprite class has been implemented as well as collision detection using bump.hpp (https://github.com/Polynominal/bump.hpp), The C Sprite example is working but a lot of the other games i tried do not work correctly. Audio is also only basic implementations for File and SamplePlayer, File routines have been implemented as well as well as some of the display and graphics functions. There might still be some issues with some of the graphics functions compared to playdate but for my game's i'm happy with it

# how to
1. place your playdate game's unaltered source code files inside src/srcgame
2. place your playdate game's Source directory (containing assets) inside Source
3. Convert fonts (fnt+png) files to TTF files by using [bitsnpicas](https://github.com/kreativekorp/bitsnpicas) to export to BDF file, then open BDF file in [fontforge studio](https://fontforge.org/en-US/) and export to ttf. So that the ttf font files match exactly as on the playdate, the export from bitsnpicas to ttf does not help here as it does not export in same size fonts as used on playdate
4. Compile using the provided makefile.
5. if all goes well compiled binary should be named game(.exe) in Source directory

# credits
Playdate api headers (contained in PD_API directory) are copyright by panic inc
bump.hpp by Polynominal (https://github.com/Polynominal/bump.hpp)

