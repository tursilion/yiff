19980114

YIFF! v1.0

by M.Brent (c)1998 All rights reserved, and the infamous "yadda yadda yadda"

The onomatopoeia of the word 'yiff' has no bearing on this game whatsoever. :)

Basic gameplay is as such: you get a grid of 6 columns by 4 rows. Two players (red and blue foxes) take turns placing pieces on the board. When a square is critical, one more fox will cause the foxes to YIFF! and scatter to adjoining squares, and all turn the colour of the current player. This may lead to a chain reaction. ;)

You can also place a wall if the square is empty... walls stop YIFF reactions.. but each wall can only take 3 hits from Yiffy foxes. :)

Use the arrow keys to select a square, and Enter or Space to select it. Press 'w' to place a wall. You can not place a wall in non-empty
squares, and you can not place a fox on a wall.

If you press ESC, you can adjust the volume settings, return to the main menu, or quit to the operating system.

The winner is the first one to have all the foxes on the board in his own colour.

The limits for critical mass is as such: corners go critical at 1 fox, sides go critical at 2 foxes, and the middle goes critical at 3 foxes (to scatter to adjoining squares). Any square that is critical with have a glowing border around it. You'll see after a game or two, or watch a demo.

The story has something to do with foxes at cons and what happens when you get too many foxes in the same room. :)

Just to be completely weird, a couple of foxes battle it out at the bottom of the screen. It's for effect only. :) Right now they randomly select between the four different foxes. They are: Yiff Li (the vixen), Ryiff (the guy with the karate suit), Zang'yiff (the BIG fox), and Bob (the fox with the hair.) ;)

To install: just unzip into a convenient directory.

To RUN it: in Windows 95, start a DOS session, go to the directory and type 'YIFF'.

IF you don't have Win95, install the CSDPMI program and run CWSDPMI before you run YIFF.EXE.

Someone once ported it to Linux, but I don't possess that port anymore.

You will first be presented with the main menu, with the following options:

D)emo - watch the computer play against itself. Wait for the end or press ESC and 'R' to return to the menu.

1) Player Game - start a 1-player game against the computer. The human is always red and always goes first. The AI is not terrific, but it works.

2) Player Game - start a 2-player game. Red and Blue take turns placing pieces.

H)elp - display a single page reference to the game

A)bout - show who made the game, who contributed, and what all. :)

O)ptions - bring up the options menu - same as in-game, though return to game and return to menu do the same thing.

Q)uit to OS - exits the program

To quit the program, either select 'QUIT TO OS' from the main menu, or press <ESC> in game and then press 'Q'.

When you start the game, you will have to select your character. Player one selects first, then player two. (The computer will select it's own character). Use the left and right arrows to select, and press SPACE or ENTER to select.

That's all, I hope! For more notes, skim the updates below.

Update 31 Aug 97:

- command line arguments, do YIFF ? to see them, but in a nutshell:
-- V - 320x200 VGA mode (ugly)
-- 1 - single player mode
-- 0 - no player mode :)
-- ! - no sound
-- + - Yiff forever! (debugging animations, really. Ctrl-Alt-End to escape)
- Put spaces between arguments: YIFF 1 ! V
- Got sprite positioning in place, so animations are more or less in the right places now
- Reduced size of background files (by cutting their resolution in half)

Update 14 Sep 97:

- New font file drawn - used for winner display and options menu
- Added Options menu (access with ESC) to adjust volume settings
- Added INI file to save last volume settings
- Added YIFF.PIF file so Win95 won't complain when you run it. It may not do anything under Win3.1, and you shouldn't use it there. You may have to edit the paths in the PIF file to point to where you run it from - remember that it expects it's data files to be in the current working directory.

Update 14 Dec 97:

- Added Ryiff and Zang'yiff characters, set up random character select
- Added keypress to restart game with same options - use space to replay or esc to abort.
- Added different "voices" (really just pitch changes) for each character
- Fixed my grabber program to stop leaving holes in the sprites, but that's not part of this program :)
- New Allegro library eliminates Win95 complaining.

Update 17 Dec 97:

- Added Bob character - all characters in place, but hafta fix Yiff Li
- Added check, holding F2 during computer play will restart with new backdrop and characters

Update 3 Jan 98:

- Yiff Li and Ryiff cleaned up - sprites now solid. :)
- Added glow around 'critical' squares so that learning the game is a little easier to figure out.
- Lotsa internal changes to make things more flexible
- Modified player and computer routines to use a timer, not repeated loops, in order to control the speed of things. (Dunno *what* I was thinking.) This means that the game should be a lot more playable and certainly closer to the proper speed on slower machines than it was, especially when it's the computer's turn.

Update 11 Jan 98:

- Cleaned up several little things internally
- Removed many debugging options - we're nearly done. All command-line arguments except '!' (no sound) are now gone. Likewise, <F2> no longer has a function.
- Removed support for 320x200 VGA mode (get UniVBE)
- Added Title Screen/Main menu
- Added Help screen and 'About' page
- Added check for <ESC> during computer's turn for option menu (mostly to support the demo (0 player) mode.)
- Modified the documentation a bit to hopefully make more sense :)

Update 13/14 Jan 98:

- Added character select screen
- Little more cleaning up in places
- Replaced the 'YIFF4EVER' flag with a key-cheat: if you hold SHIFT while placing a piece (either player), you can watch the character go through it's animations until you release SHIFT.
- Changed the 'Red Fox' and 'Blue Fox' to the character names
- Added O)ptions to the main menu
- Finally released as a working game. I don't think there are any more bugs, but am outta time. :)

