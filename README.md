# Playdate Api SDL2
An attempt to reimplement the Playdate Handheld C Api in SDL2

i've used this to create windows (and linux) binaries to for my playdate game's from unaltered source code for the playdate. I attempted to reimplement the Playdate API headers in SDL2. It's probably done in a very bad way but it does seem to work for my games and a tetris game i tested from someone else on github. A Lot is still not implemented and probably never will. For example games using sprites api won't work. Audio is also only basic implementations for File and SamplePlayer, File routines have been implemented as well as well as some of the display and graphics functions. There are still issues with some of the graphics functions compared to playdate but for my game's i'm happy with it

# how to
1. place your playdate game's unaltered source code files inside src/srcgame
2. place your playdate game's Source directory (containing assets) inside Source
3. Convert fonts (fnt+png) files to TTF files by using [bitsnpicas](https://github.com/kreativekorp/bitsnpicas) to export to BDF file, then open BDF file in [fontforge studio](https://fontforge.org/en-US/) so that the ttf font files match exactly as on the playdate. 
4. Compile using the provided makefile.

# credits
## Playdate api headers (contained in PD_API directory) are copyright by panic inc


