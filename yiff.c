
/* yiff! - Original Release - 13 Jan 98 */
/* Updated 5 Dec 99 for Win32 */
/* by M.Brent (Tursi) */
/* Updated 19 April 99 - Added better comments to assist with porting.. */
/* view with a non-proportional font for best results, 80 columns */
/* this was written for Allegro 2.x... you may need some changes for */
/* 3.x, specifically you may need to add color_depth settings. */
/* Yiff runs at 640x480x256 colours (8 bit colour depth) */

/* released for Weyfour WWWWolf (a.k.a. Urpo Lankinen) to do his */
/* Unix port - not intended for public viewing. Certainly not intended */
/* as any kind of example, except maybe 'what not to do'. ;) */
/* Weyfour, do what you  must, but try to leave my messages intact */
/* (add your own, of course!) */
/* and I want a copy when it's done. ;) */

/**/

/* standard includes */
#include <stdio.h>
/*#include <crt0.h>*/       /* need this one for the CWSDPMI patch below */
#include <allegro.h>
#include <string.h>
//#include <winalleg.h>
#include "images.h"     /* header with the names of the data images */

#define TILEX 32        /* tile X pixel size */
#define TILEY 31        /* tile Y pixel size. No, they aren't square */
#define Xcells 6        /* number of cells across */
#define Ycells 4        /* number of cells down */
#define SETSPEED 1      /* speed control for animations */
#define DEFY 440        /* default Y position for sprites */
#define YIFFON 10       /* arbitrary number larger than 4 used to flag */
                        /* that a particular cell must YIFF */
#define OK 0            /* used in doplayer() */
#define RESTART 1       /* value to restart a game */
#define QUIT 2          /* value to quit a game */
#define S_P1_OFF 0      /* unused */
#define S_P2_OFF 8      /* used in charselectr(), offset to the player 2 */
                        /* colors of the sprites */
#define S_SAM_OFF 16    /* offset to audio data in charselectr() data */

/**/

/* set some flags to fix CWSDPMI problems (?)*/
/* If these flags are not set, CWSDPMI will die on some DOS systems */
/* not needed if not DOS and not CWSDPMI */
/*int _crt0_startup_flags=_CRT0_FLAG_NULLOK | _CRT0_FLAG_FILL_SBRK_MEMORY;*/

/**/

/* global variables - nearly all variables are global */

DATAFILE *data;                         /* main data file */
DATAFILE *character0,*character1;       /* character data files */
FILE *fp;                               /* generic file pointer */
char charanim[2][50];                   /* player sprite animation strings */
char charyoff[2][50];                   /* y offset strings for animation */
char playname[2][10];                   /* name of each player */
int charpitch[2];                       /* audio playback rate for player */
int deady[2];                           /* y co-ordinate for 'dead' sprite */
int charx[2],chary[2];                  /* current X and Y positions */
int chars[2];                           /* current sprite number */
int xoff=16, yoff=0;                    /* screen offset for tile drawing */
int x,y,z,drfl,tx,ty;                   /* mostly generic */
int cycle_red,cycle_blue,cycle_dir;     /* used in color cycling routine */
int black,white,nosound;                /* color index for black, white */
                                        /* flag for disabled sound */
RGB col;                                /* generic color variable */
int score[3],walls[3],grid[Xcells][Ycells]; /* current score, current walls */
                                            /* set to 3 for ease of use (1,2) */
                                            /* grid information */
int iloaded;                            /* interrupt loaded flag */
int max[Xcells][Ycells]={2,3,3,2, 3,4,4,3,  /* max counters for grid */
     3,4,4,3, 3,4,4,3, 3,4,4,3, 2,3,3,2 };  /* this sets the YIFF limits */
int plgrid[3][Xcells][Ycells];          /* individual player counts (0 unused) */
int animspeed;                          /* used for animation timing */
int players;                            /* number of players (0-2) */
int MIDIvol,DIGIvol,i;                  /* volume settings, generic counter */
int charp0,charp1;                      /* character # for each player */
                                        /* 0 = Yiff Li, etc */
int *intp;                              /* used to load from INI file */
char string[80];                        /* temp string */
BITMAP *background,*work,*picture;      /* background image space (grid), */
                                        /* work space (mixer) */
                                        /* picture space (far background) */

/**/

/* function prototypes */
void fail(char *);              /* bad exit */
void cycle254(void);            /* interrupt routine to cycle colour 254 */
void main(int,char*[]);         /* main function */
int  game(void);                /* play one game */
int  doplayer(int);             /* move player */
int  doyiffs(int);              /* perform yiffing */
int  screenx(int);              /* calculate tile->pixel screen X coordinate */
int  screeny(int);              /* same thing, but for Y coordinate */
void drawfox(int,int,int,int);  /* draw a fox on the grid, update counters */
void erasecell(int,int);        /* erase a grid cell with the background */
void display(void);             /* draw the screen */
void drawwall(int,int);         /* draw a wall in a grid */
void draw_sprites(void);        /* draw the sprites at the bottom */
void options(int *);            /* options window */
void title(void);               /* perform the title menu */
void charselectr(void);         /* player select */
void help(void);                /* help screen from menu */
void about(void);               /* about screen from menu */
void greet(void);               /* greet page from command line */
void leave(void);               /* end the game, write the ini, say bye */

/**/

/* functions */

void fail(char *s)
{ /* something went wrong, return to text mode and print an error */

  allegro_exit();       /* shut down allegro */
  printf("%s\n",s);     /* print message */

  set_volume(255,255);  /* reset sound card */

  /* now unload anything we loaded */
  if (background) destroy_bitmap(background);
  if (work) destroy_bitmap(work);
  if (picture) destroy_bitmap(picture);
  if (data) unload_datafile(data);
  if (character0) unload_datafile(character0);
  if (character1) unload_datafile(character1);

  /* arbitrary exit level to signal an error if anyone cares ;) */
  exit(5);
}

void leave(void)
{ /* proper exit from the game */

  allegro_exit();       /* shut down Allegro */

  /* say goodbye */
  printf("Yiff! V1.0 - 13 Jan 98\nThanks for playing!\n(c)1998 M.Brent\nEmail:tursi@neteng.bc.ca\n\nHappy Birthday Foxx! :)");

  /* write the INI file for audio settings */
  fp=fopen("yiff.ini","w");
  if (fp)       /* if it didn't open, just forget about it */
  { fprintf(fp,"; Automatically Generated file - Will be overwritten\n");
    fprintf(fp,"\nMIDI = %d\n",MIDIvol);
    fprintf(fp,"DIGI = %d\n",DIGIvol);
    fprintf(fp,"; EOF");
    fclose(fp); /* if it did open, we need to close it here */
  }

  set_volume(255,255);  /* restore volume */

  /* unload anything we loaded */
  if (background) destroy_bitmap(background);
  if (work) destroy_bitmap(work);
  if (picture) destroy_bitmap(picture);
  if (data) unload_datafile(data);
  if (character0) unload_datafile(character0);
  if (character1) unload_datafile(character1);

  /* all done, no error */
  exit(0);
}

void cycle254()
{ /* -- Interrupt Routine -- */
  /* colour cycle colour 254 - called 50 times/sec */
RGB col;

/* we really just bounce red and blue back and forth */

/* adjust the colour values */
cycle_red=cycle_red+cycle_dir;
cycle_blue=cycle_blue-cycle_dir;

/* check limits... if either one hits zero, we bounce the other way */
/* this works well because they start appropriately, and go opposite */
/* directions */
if (cycle_red==0) cycle_dir=1;
if (cycle_blue==0) cycle_dir=-1;

/* assign the colours to colour 254. Not using the inline version */
col.r=cycle_red;
col.g=0;
col.b=cycle_blue;
set_color(254,&col);
}
/* signal to Allegro the end of the interrupt function */
END_OF_FUNCTION(cycle254);

void main(int argc, char *argv[])
{ /* main function, we start here */
  BITMAP *tmp2;         /* temp stuff for messy screen work */
  PALLETE pal;          /* temp palette to go with it */

 if (*argv[1]=='?')     /* check for help request, fail out if so */
 { fail("YIFF.EXE [!]\n  ! - no sound (use if it locks)\n");
 }

 /* print starting notice */
 printf("\n\nInitialize Yiff! 1.0 (13 Jan 98)...\n");

 background=work=picture=NULL;          /* set structures to NULL so we */
 data=character0=character1=NULL;       /* know what to unload later */
 animspeed=SETSPEED;                    /* set animation speed counter */
 allegro_init();                        /* initialize Allegro */
 install_timer();                       /* load Allegro timer routines */
 install_keyboard();                    /* load Allegro keyboard routines */
 set_color_depth(8);
 LOCK_VARIABLE(cycle_red);              /* lock the variables and function */
 LOCK_VARIABLE(cycle_blue);             /* for Cycle254 so that they are */
 LOCK_VARIABLE(cycle_dir);              /* not allowed to swap out to disk, */
 LOCK_FUNCTION(cycle254);               /* which would be bad. ;) */

 if (set_gfx_mode(GFX_AUTODETECT,640,480,640,480))      /* set gfx mode */
 { 
     fail("Can not set SVGA 640x480x256 graphics mode\n");  /* fail if can't */
 }

 players=2;     /* default number of players */
 iloaded=0;     /* is Cycle254 loaded? (no) */
 nosound=0;     /* Is sound disabled? (no) */

 /* now we parse the arguments */
 /* there's not much here because we really don't care much about */
 /* the commandline, but we did once. */

 for (x=1; x<argc; x++)                 /* go through all arguments */
 { 
   if (*argv[x]=='!')                   /* '!' means disable sound */
   { nosound=1;                         /* set it as such */
     printf("No sound!\n");             /* and say we found it */
   }
   if (strcmp(argv[x],"-greet")==0) greet();    /* run greet() if we find */
 }                                              /* -greet */

 /* only install the sound drivers if it's okay to, */
 /* otherwise use the NONE drivers for no sound with no effort ;) */
 if (!nosound)
   install_sound(DIGI_AUTODETECT,MIDI_AUTODETECT,NULL);
 else
   install_sound(DIGI_NONE,MIDI_NONE,NULL);

 /* default values for volume (0-255) */
 MIDIvol=220;
 DIGIvol=220;

 /* try to read the .ini file */
 /* this function is sensitive to non-space (ascii 32) whitespace, */
 /* and cares somewhat about column alignment. So don't edit the INI */
 /* file by hand. */
 fp=fopen("yiff.ini","r");
 if (fp)                                /* if no, we don't care, we have */
                                        /* defaults set already */
 { while (!feof(fp))                    /* read the whole file */
   { fgets(string,80,fp);               /* one line... */
     i=0;                               /* skip over any leading spaces */
     while (string[i]==' ') i++;
     intp=NULL;                         /* get ready to see what to set */
     if ((string[i]=='M')||(string[i]=='m'))
     { intp=&MIDIvol;                   /* lets set MIDI volume */
       printf("MIDI volume:");          /* print half the line */
     }
     if ((string[i]=='D')||(string[i]=='d'))
     { intp=&DIGIvol;                   /* lets set DIGI volume */
       printf("DIGI volume:");          /* print half the line */
     }
     if (intp)                          /* so if we recognize the keyword */
     { while ((string[i]!='=')&&(string[i]!=0))
         i++;                           /* skip to the equals sign... */
       if (string[i])
       { i++;                           /* plus 1... */
         *intp=atoi(&string[i]);        /* and set the variable with the */
                                        /* numeric value there */
         printf(" %d\n",*intp);         /* then print it */
         /* note no error checking for value! */
       }
     }
   }
   fclose(fp);                          /* close the file */
 }                                      /* done reading .ini */

 data=load_datafile("images.dat");      /* load generic images */
 if (data==NULL)
 { fail("Cannot load images.dat");      /* fail if can't load */
 }
 background=create_bitmap(640,480);     /* make room for background */
 if (background==NULL)
 { fail("Cannot reserve data memory."); /* fail if can't */
 }
 work=create_bitmap(640,480);           /* make room for workspace */
 if (work==NULL)
 { fail("Cannot reserve data memory."); /* fail if can't */
 }

 set_palette(data[PALLETE_001].dat);    /* set palette from datafile */

 /* find black */
 black=0;                               /* defaults to index 0 */

 for (y=0; y<254; y++)
 { get_color(y,&col);
   if ((col.r==0)&&(col.g==0)&&(col.b==0))      /* any colour set to 0,0,0 */
     black=y;                                   /* we don't care which */
 }

 /* find white (as close as possible) */
 white=0;                               /* also defaults to index 0 */
                                        /* ok because I know the color I want */
                                        /* is definately in there */
 for (y=0; y<254; y++)
 { get_color(y,&col);
   if ((col.r>50)&&(col.g>50)&&(col.b>50))      /* any colour brighter than */
     white=y;                                   /* 50,50,50, we don't care which */
 }                                              /* (0-63 on colours) */

/**** Select characters!  - Restart point! ****/
/* label */
/* the game will in fact GOTO back here, sorry ;) */
/* used to restart a game */
charselect:

/* call the title page */
title();

/* we're starting now, so call the character select routine */
charselectr();

/* load interrupt routine */
if (iloaded==0)         /* only if not active */
{ if (install_int(cycle254,50))
    fail("Can not install interrupt routine cycle254()\n"); /* fail if can't */
  else iloaded=1;       /* flag active if we could */
}

/* this switch loads the various settings for each character, for player 1 */
/* unfortunately, player 1 is referred to as '0' in some places, and '1' in */
/* others (this is why many of the arrays have room for 3 players, zero is */
/* not used...) */
switch (charp0)
{ case 0: /* load settings for Yiff Li */
          character0=load_datafile("yiffli.dat"); /* load her datafile */
          if (character0==NULL)
            fail("Cannot load character");      /* fail if can't */
          /* set animation string for frames */
          /* each character represents a frame number, with 'A' (uppercase) */
          /* representing frame 0. Check the data files to understand this  */
          /* one character of the string represents one 'Yiff', so you need */
          /* a fair chain to complete it. If you DO, it just repeats.       */
          /* In this case, 'G' is the punch frame, I, J, and K are the      */
          /* rapid leg kick frames, and L, M, N, O, P correspond to the spin*/
          /* kick frames. */
          strcpy(charanim[0],"GGIJKIJKIJKIJKLMNOPMNOPMNOPMNOPMNOPMNOPMNOP");
          /* set the offset string for each frame. This corresponds         */
          /* exactly to the animation string. Each character is an          */
          /* offset on the Y axis. '0' means zero. 'a' and up mean negative */
          /* values, starting at 0, multiplied by 4. 'A' and up mean        */
          /* positive values, starting at zero, multiplied by 4             */
          /* it's used to move the character up and down with the frames    */
          strcpy(charyoff[0],"000000000000000hhhhhhhhhhhhhhhhhhhhhhhhhhhh");
          /* this is the Y offset for the dead (laying on the ground)       */
          /* sprite. It turned out to vary for the characters and is such a */
          /* variable.                                                      */
          deady[0]=8;
          /* charpitch is the playback rate for samples when this character */
          /* moves, with 1000 being normal pitch. Yiff Li has a higher voice*/
          /* and so a higher value */
          charpitch[0]=1200;
          /* done. The rest of the characters follow the same format and so */
          /* get no extra documentation, except the one exception below */
          break;
  case 1: character0=load_datafile("ryiff.dat");
          if (character0==NULL)
            fail("Cannot load character");
          strcpy(charanim[0],"GGHIJHIJHIJHIJHIJHIJGGKLKKLLKKKLLL"); 
          /* yes, in the next line is 1 untypeable character|       */
          /* Sorry about that....                           |       */
          /* I got it by holding ALT and typing 0xxx on the |       */
          /* number pad, but I forget what is is...         V       */
          strcpy(charyoff[0],"00hhhhhhhhhhhhhhhhhh00phpxphpxxph");
          deady[0]=8;
          charpitch[0]=1000;
          break;
  case 2: character0=load_datafile("zangyiff.dat");
          if (character0==NULL)
            fail("Cannot load character");
          strcpy(charanim[0],"GGHHIAJJJJJKLMNKLMNKLMNKLMNKLMNKLMNKLMNKLMN"); 
          strcpy(charyoff[0],"000000qvzvq00000000000000000000000000000000");
          deady[0]=4;
          charpitch[0]=800;
          break;
  case 3: character0=load_datafile("bob.dat");
          if (character0==NULL)
            fail("Cannot load character");
          /* note that the length can vary */
          strcpy(charanim[0],"GGHHHIJKIJKIJKLMNOLMNOLMNO");
          strcpy(charyoff[0],"00hjh000000000dhjfdhjfdhjf");
          deady[0]=8;
          charpitch[0]=900;
          break;
}

/**********************player 2 ****************************/
/* same thing, but now for Player 2. */

switch (charp1)
{ case 0: character1=load_datafile("yiffli2.dat");
          if (character1==NULL)
            fail("Cannot load character");
          strcpy(charanim[1],"GGIJKIJKIJKIJKLMNOPMNOPMNOPMNOPMNOPMNOPMNOP"); 
          strcpy(charyoff[1],"000000000000000hhhhhhhhhhhhhhhhhhhhhhhhhhhh");
          deady[1]=8;
          charpitch[1]=1200;
          break;
  case 1: character1=load_datafile("ryiff2.dat");
          if (character1==NULL)
            fail("Cannot load character");
          strcpy(charanim[1],"GGHIJHIJHIJHIJHIJHIJGGKLKKLLKKKLLL"); 
          /* that same untypable character is here again    |       */
          /* I just needed extra height ;)                  V       */
          strcpy(charyoff[1],"00hhhhhhhhhhhhhhhhhh00phpxphpxxph");
          deady[1]=8;
          charpitch[1]=1000;
          break;
  case 2: character1=load_datafile("zangyif2.dat");
          if (character1==NULL)
            fail("Cannot load character");
          strcpy(charanim[1],"GGHHIAJJJJJKLMNKLMNKLMNKLMNKLMNKLMNKLMNKLMN"); 
          strcpy(charyoff[1],"000000qvzvq00000000000000000000000000000000");
          deady[1]=4;
          charpitch[1]=800;
          break;
  case 3: character1=load_datafile("bob2.dat");
          if (character1==NULL)
            fail("Cannot load character");
          strcpy(charanim[1],"GGHHHIJKIJKIJKLMNOLMNOLMNO"); 
          strcpy(charyoff[1],"00hjh000000000dhjfdhjfdhjf");
          deady[1]=8;
          charpitch[1]=900;
          break;
}

/*** Done loading characters... ***/

/* this stuff prolly should be BEFORE starting the interrupt routine, duh */
/* set the default colour for index 254 (cycling colour) */
 col.r=0;
 col.g=0;
 col.b=63;
 set_color(254,&col);

/* now set the cycle254() variables to reflect it */
 cycle_red=0;
 cycle_blue=63;
 cycle_dir=1;

/* prepare workspace */
 clear_to_color(work,black);

 /* choose background */
 /* we have 6 random backgrounds, named BACKx.PCX, so this is easy */
 y=(rand()%6)+1;                        /* static number.. what's the term? */
 sprintf(string,"back%d.pcx",y);        /* oh yeah! "Magic" numbers... */
 tmp2=load_pcx(string,pal);             /* try to load the image into the */
 if (!tmp2)                             /* temporary buffer */
 { tmp2=create_bitmap(320,240);         /* if not, we try to make space */
   if (tmp2==NULL) fail("Can't reserve data memory"); /* fail if can't */
   clear_to_color(tmp2,black);          /* if we can, colour it black */
 }                                      /* this way, we can live without pics */

/* now, the background is loaded (or created), at only 320x240, but we */
/* are using 640x480 on the display. The images are smaller for disk */
/* space reasons, and it doesn't hurt much. For performance, we now */
/* scale the image up into a full-sized buffer */
 picture=create_bitmap(640,480);        /* reserve space... */
 if (picture==NULL) fail("Can't reserve data memory");  /* fail if can't */
 stretch_blit(tmp2,picture,0,0,320,240,0,0,640,480);    /* stretch pic into buffer */
 destroy_bitmap(tmp2);                  /* remove temporary buffer */

 blit(picture,background,0,0,0,0,640,480); /* copy into background buffer */

 /* draw screen */
 /* now we want to draw the grid onto the background */
 /* these lines draw the rows and columns of the wall sprite onto it */
 for (x=0; x<7; x++)
   for (y=0; y<13; y++)
     blit(data[WALL].dat,background,0,0,(x*3)*TILEX+xoff,y*TILEY+yoff,TILEX,TILEY);

 for (y=0; y<5; y++)
   for (x=0; x<19; x++)
     blit(data[WALL].dat,background,0,0,x*TILEX+xoff,(y*3)*TILEY+yoff,TILEX,TILEY);

 /* dump the background into the workspace */
 blit(background,work,0,0,0,0,640,480);

 /* set up the default location and sprite for player 1 */
 charx[0]=248;          /* magic numbers again.. */
 chary[0]=DEFY;
 chars[0]=0;

 /* do the same for Player 2 */
 charx[1]=392;
 chary[1]=DEFY;
 chars[1]=1;

 /* draw the sprites onto the workspace */
 draw_sprites();

 /* copy the workspace to the screen */
 blit(work,screen,0,0,0,0,640,480);

 /* fade the image in now */
 fade_in(data[PALLETE_001].dat,4);
 /* pause to give it time to finish */
 rest(500);
 /* this was supposed to fix a bug where sometimes the palette sets */
 /* improperly. It didn't work 100%. Maybe this is because we fade in */
 /* while the interrupt routine cycle254() is running? */
 set_palette(data[PALLETE_001].dat);

 /* play one game. If it exits true, go back to charselect: and play again */
 if (game())
   goto charselect;

 /* else we are done, clean up and return */
 leave();
}
END_OF_MAIN()

int game()
{ /* do one game - return true to play again */
int i1,i2,fl;   /* indexes and flags */
int s,d,r,fl2;  /* more misc variables. r is the return code */
char buf[80];   /* temporary string */
BITMAP *tbmp;   /* temporary bitmap pointer */

score[1]=0;     /* set initial scores (0 each) */
score[2]=0;
walls[1]=3;     /* each player starts with 3 walls */
walls[2]=3;

/* erase grids in memory. We have a generic counter grid which doesn't */
/* care about colours (grid[][]), and one grid for each player which */
/* tracks their pieces only per square (plgrid[][]) */
for (i1=0; i1<Xcells; i1++)
{ for (i2=0; i2<Ycells; i2++)
  { grid[i1][i2]=0;
    plgrid[1][i1][i2]=0;
    plgrid[2][i1][i2]=0;
  }
}

/* prepare flag - it's on for now. */
fl=1;

/* set desired volume levels */
set_volume(DIGIvol,MIDIvol);

/* start background music, make it loop*/
play_midi(data[MIDI_BGM].dat,TRUE);

/* display screen */
display();

/* now we play the game until FL(ag) is cleared, indicating a stop */
/* note this loop also exits the function itself if restart or quit */
/* is chosen from the options menu */
while (fl)
{ fl2=doplayer(1);              /* play player 1's turn */
  if (fl2==RESTART) return(1);  /* check for restart */
  if (fl2==QUIT) return(0);     /* check for quit */
  fl=doyiffs(1);                /* check for new yiffs (player 1 moved) */
  if (fl)                       /* if we're still going... ;) */
  { fl2=doplayer(2);            /* play player 2 */
    if (fl2==RESTART) return(1);/* check restart.. */
    if (fl2==QUIT) return(0);   /* and quit... */
    fl=doyiffs(2);              /* do his yiffs. fl goes to 0 if game over */
  }                             /* done player 2 */
}                               /* end of while loop */

remove_int(cycle254);           /* remove the interrupt */
iloaded=0;                      /* flag that it's off */

play_midi(data[MIDI_WIN].dat,FALSE);    /* play win music, no looping */

/* write the winner's name to a temp string */
sprintf(buf,"%s Wins!",score[1]==0 ? &playname[1][0] : &playname[0][0]);

/* create a temporary bitmap sized for the text string. It's going to */
/* be a sprite to scroll the name across with */
tbmp=create_bitmap(text_length(data[FONT_002].dat,buf),text_height(data[FONT_002].dat));

/* make sure it draws with transparent background, colour 0 */
text_mode(0);

/* print the text to the temporary bitmap, using index 255 */
textout(tbmp,data[FONT_002].dat,buf,0,0,255);

/* set the colour to red or blue depending on who won */
/* if player 1 lost (score to 0), then red is 0, else it's 63 (full) */
col.r=(score[1]==0 ? 0 : 63);
/* green is always 0 */
col.g=0;
/* if player 1 lost (score to 0), then blue is 63 (full), else it's 0 */
col.b=(score[1]==0 ? 63 : 0);

/* color index 255 appropriately */
set_color(255,&col);

/* again, based on who won, set the start position of the text in pixels (s) */
/* the direction and speed (d), and set the appropriate character to the */
/* fall sprite, which is fixed as sprite #3 in all datafiles */
if (score[1]==0) /* player 1 lost... */
{ s=640; d=-20; chars[0]=3; } /* start at right, move left, fall player 1 */
else             /* player 1 won... */
/* start at left, off screen by size of text, move right, fall player 2 */
{ s=-(text_length(data[FONT_002].dat,buf)); d=20; chars[1]=3; }

/* move the text onto screen with a squishy effect */
/* we're flexible about where it stops, between 100 and 140 X */
for (i1=s; (i1<100)||(i1>140); i1=i1+d)
{ /* clear retrace timer */
  retrace_count=0;
  /* stretch the text onto the workspace */
  stretch_sprite(work,tbmp,i1,135,400,170);
  /* copy to the screen */
  blit(work,screen,0,0,0,0,640,480);
  /* recopy the workspace from the background */
  blit(background,work,0,0,0,0,640,480);
  /* add the sprites */
  draw_sprites();
  /* wait till 1 frame is done, minimum */
  while (retrace_count==0);
}

/* put both characters back on ground */
chary[0]=DEFY;
chary[1]=DEFY;

/* based on who won, set the loser to the dead sprite (always number 4), */
/* the winner to the normal standing stripe (number 0), and add the dead */
/* sprite offset to the loser's y coordinate. Can you say 'patch', boys */
/* and girls? :) */
if (score[1]==0)
{ s=0; chars[0]=4; chars[1]=0; chary[0]=chary[0]+deady[0];}
else
{ s=1; chars[1]=4; chars[0]=0; chary[1]=chary[1]+deady[1];}

/* this does the squishing effect on the words */
for (i1=0; i1<100; i1=i1+10)
{ /* set timer */
  retrace_count=0;
  /* stretch the text onto the workspace */
  stretch_sprite(work,tbmp,120+i1*s,135-i1/2,400-i1,170+i1);
  /* copy the workspace to the screen */
  blit(work,screen,0,0,0,0,640,480);
  /* reset the workspace from the background */
  blit(background,work,0,0,0,0,640,480);
  /* draw in the sprites */
  draw_sprites();
  /* wait for at least 1 retrace */
  while (retrace_count==0);
}

/* now, based on who won, set the winner to the 'win' pose sprite (#5) */
/* these should have been #defines, methinks.. ;) */
if (score[1]==0)
{ chars[1]=5;
}
else
{ chars[0]=5;
}

/* this 'unstretches' the words to normal size */
/* you know the routine by now. */
for (i1=90; i1>0; i1=i1-10)
{ retrace_count=0;
  stretch_sprite(work,tbmp,120+i1*s,135-i1/2,400-i1,170+i1);
  blit(work,screen,0,0,0,0,640,480);
  blit(background,work,0,0,0,0,640,480);
  draw_sprites();
  while (retrace_count==0);
}

/* this ensures the text ends in the right size */
/* we still stretch it, cause the normal font is */
/* smaller than we want */
stretch_sprite(work,tbmp,120,135,400,170);
blit(work,screen,0,0,0,0,640,480);

/* this does a similar thing to cycle254(), but not an interrupt */
/* and with colour 255, to cycle the text green brightness back and */
/* forth. This way, we don't care who won (blue or red), and we still */
/* get a colour cycle. */
i1=1; d=1; r=-1;        /* index, direction, return value */
while (r==-1)           /* as long as 'r' is stil -1... */
{ i1=i1+d;              /* update the index with direction */
  if ((i1==63)||(i1==0)) d=-d;  /* bounce the direction if we hit a limit */
  col.g=i1;             /* 'col' is still set from above, so we just do */
                        /* the green here. */
  vsync();              /* wait for a vsync */
  set_color(255,&col);  /* set the colour, hopefully during vsync */
  rand();               /* generate random numbers to eliminate the need */
                        /* for trying to find a random seed for the generator */
  poll_keyboard();
  if (key[KEY_ESC]) r=1;        /* ESC or SPACE breaks us out of here.. */
  if (key[KEY_SPACE]) r=1;
}                       /* end while.. */

/* wipe the temporary bitmap */
destroy_bitmap(tbmp);

/* return what we got, always 1 at this point */
return(r);
}

int doplayer(int pl)
{ /* place a single player, update grid */
int ch,z,xm,ym,x,y,w,dx,dy,dw,re;       /* lotsa niggly vars                */
                                        /* ch=sprite character for fox      */
                                        /* z= flag                          */
                                        /* xm,ym= movement counters         */
                                        /* x,y= co-ordinates in maze        */
                                        /* w =what we're doing              */
                                        /* dx,dy=destination in maze        */
                                        /* dw=draw wall?                    */
                                        /* re=return, to signal end of game */

re=OK;                                  /* so far, it's OK */
if (pl==1) ch=REDFOX; else ch=BLUEFOX;  /* set player character number */
chary[0]=DEFY;                          /* new turn, both sprites on ground */
chary[1]=DEFY;
x=0; y=0;                               /* current x and y are top left */
z=100; w=0;                             /* z is the blink delay. w is a wall */
                                        /* flag used later */
if (((players==1)&&(pl==2))||(players==0)) /* is it the computer's turn? */
{ /* computer play */
  dw=0;                                 /* no wall */
  do
  { dx=rand()%Xcells;                   /* this routine randomly chooses */
    dy=rand()%Ycells;                   /* a cell and verifies that it's */
  } while (grid[dx][dy]<0);             /* a legal move */
  if (grid[dx][dy]==0)                  /* if it's completely empty... */
    if (((rand()%10)<2)&&(walls[pl])&&(score[pl]>0)) /* how about a wall? */
      dw=1;                             /* yes, we'll do a wall */
  while ((dx>x)||(dy>y))                /* now we move the piece into place */
  { retrace_count=0;                    /* timer speed control */
    /* this fun little blit draws the fox in the middle of the current cell */
    /* using the appropriate colour fox (ch) */
    blit(data[ch].dat,screen,0,0,screenx(x)+(TILEX/2),screeny(y)+(TILEY/2),TILEX,TILEY);

    /* wait for 10 refreshes */
    while (retrace_count<10);

    /* prepare to move the fox */
    xm=0; ym=0;
    /* if we have to move down, and the animation speed counter has counted */
    if ((dx>x)&&(animspeed==1))
      /* set the movement... */
    { xm=1;
      /* and play a sound, using the appropriate pitch */
      play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    }
    /* same thing for right.. */
    if ((dy>y)&&(xm==0)&&(animspeed==1))
    { ym=1;
      play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    }
    /* since we start in the top left, we never have to move */
    /* up or left */

    /* get the background into the workspace */
    blit(background,work,0,0,0,0,640,480);

    /* is it time to animate the fighters stance? */
    if (--animspeed==0)
    { if (chars[0]==0)          /* toggle them back and forth.          */
      { chars[0]=1;             /* the fighting stance is always        */
        chars[1]=0;             /* in sprites 0 and 1. Bad Tursi.       */
      } else
      { chars[0]=0;
        chars[1]=1;
      }
      animspeed=SETSPEED;       /* reset the countdown timer            */
    }

    draw_sprites();             /* draw them to the buffer */
    blit(work,screen,0,0,0,0,640,480);  /* blit to the screen */
    x=x+xm;                     /* update the coordinates */
    y=y+ym;

    while (retrace_count<15);   /* delay for 15 retraces */

	poll_keyboard();
    if (key[KEY_ESC])           /* check for ESC key for options menu */
      options(&re);
  }                             /* repeat till done movement.. */
  x=dx; y=dy;                   /* set x and y to destination, we're there */
  if (dw==0)                    /* if we're not drawing a wall... */
  { z=0;                        /* then say so for later */
    w=2;                        /* tell the routine what's up */
    play_sample(data[BARK].dat,100,128,charpitch[pl-1],FALSE); /* play the sound */
  }
  else
  { w=1;                        /* drop a wall */
    walls[pl]--;                /* count down */
  }
}                               /* done computer movement */

retrace_count=0;                /* for timing */
while ((w==0)&&(re==0))         /* this is true if it's a human's turn */
{ /* these two lines make the player's fox head cursor blink */
  if (retrace_count<10)
    blit(data[ch].dat,screen,0,0,screenx(x)+(TILEX/2),screeny(y)+(TILEY/2),TILEX,TILEY);
  if (retrace_count>10)
    blit(background,screen,screenx(x)+(TILEX/2),screeny(y)+(TILEY/2),screenx(x)+(TILEX/2),screeny(y)+(TILEY/2),TILEX,TILEY);

  xm=0; ym=0;           /* movement flags to 0 */
  rand();               /* generate a random number. This works because */
                        /* when you're waiting for user input, who knows */
                        /* how long it will take? So you never know where */
                        /* in the random string you will be when they */
                        /* finally press a key ;) */
  poll_keyboard();
  if (key[KEY_ESC])     /* check ESC key for options menu */
    options(&re);

/* if pressing up, and we can go up... */
  if ((key[KEY_UP])&&(y>0)) 
    /* set movement flag to up */
  { ym=-1;
    /* play sound */
    play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    /* wait for key release */
    while (key[KEY_UP])
		poll_keyboard();
  }

  /* the rest are the same as up */
  if ((key[KEY_DOWN])&&(y<Ycells-1))
  { ym=1;
    play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    while (key[KEY_DOWN])
		poll_keyboard();
  }
  if ((key[KEY_LEFT])&&(x>0))
  { xm=-1;
    play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    while (key[KEY_LEFT])
		poll_keyboard();
  }                        
  if ((key[KEY_RIGHT])&&(x<Xcells-1))
  { xm=1;
    play_sample(data[POC].dat,80,128,charpitch[pl-1],FALSE);
    while (key[KEY_RIGHT])
		poll_keyboard();
  }

  /* if we're moving, zero the timer flag */
  if ((ym)||(xm))
  { z=0;
  }

  /* enter or space are used to drop the piece on the board */
  if ((key[KEY_ENTER])||(key[KEY_SPACE]))
  { if (grid[x][y]>=0)  /* is it legal? */
    { z=0;              /* zero the timer */
      w=2;              /* set that we're dropping */
      /* and play the sound */
      play_sample(data[BARK].dat,100,128,charpitch[pl-1],FALSE);
    }
    else        /* it's illegal, so play the bad sound */
      play_sample(data[MURP].dat,100,128,charpitch[pl-1],FALSE);  
    /* wait for key release */
    while ((key[KEY_ENTER])||(key[KEY_SPACE]))
		poll_keyboard();
  }

  /* 'w' is used to drop a wall */
  if (key[KEY_W])
  { if ((grid[x][y]==0)&&(score[pl]>0)&&(walls[pl]>0)) /* is it legal? */
    { w=1;              /* yes, so set the flag */
      walls[pl]--;      /* count down number of walls */
    }
    else                /* illegal, play the bad sound */          
      play_sample(data[MURP].dat,100,128,charpitch[pl-1],FALSE);
    /* wait for key release */
    while (key[KEY_W])
		poll_keyboard();
  }

  /* F1 dumps the video memory into a PCX. On a 16mb video card, this makes */
  /* a BIG image. This is not intended for general use, if so, you should */
  /* make a sub-bitmap of the screen and save that, instead. */
  if (key[KEY_F1])
  { /* save screen dump */
    save_pcx("yiff_pic.pcx",screen,data[PALLETE_001].dat);
  }

  /* this just adjusts timing for the blink and stuff */
  if (retrace_count>15) z=0;

  if (z==0)     /* when the timer kicks in... */
  { blit(background,work,0,0,0,0,640,480); /* reset the background */     
    if (--animspeed==0) /* count down animation timer */
    { if (chars[0]==0)  /* animate the sprites as above if needed */
      { chars[0]=1;
        chars[1]=0;
      } else
      { chars[0]=0;
        chars[1]=1;
      }
      animspeed=SETSPEED;       /* reset anim counter */      
    }
    draw_sprites();             /* draw the sprites */
    blit(work,screen,0,0,0,0,640,480);  /* blit to the screen */
    z=-50;      /* reset the z counter */
    x=x+xm;     /* move the player sprite */
    y=y+ym;     
    retrace_count=0; /* reset the retrace timer */
  }
}

/* we have co-ordinates, now draw the shape and update the tables */
/* this is common between the human and the computer */

if (w==2)       /* this is easy, drawfox deals with the work */
  drawfox(x,y,ch,pl);

if (w==1)       /* not much harder, if it's a wall, we make the */
                /* grid counter negative. -3 means 3 hits, as */
                /* fox hits increment this until it hits 0 and */
                /* becomes usable again. Then draw the wall */
{ grid[x][y]=-3;
  drawwall(x,y);
}  

return(re);     /* return the return code */
}

void drawfox(int x,int y,int ch,int pl)
{ /* this function draws the fox head in the appropriate cell */
  /* and updates the grid counters. It also decides WHERE in */
  /* the cell to stack the fox head ;) */
  int xm,ym;    /* used to adjust offset within the cell */

if (grid[x][y]<0)       /* if we drop onto a wall... (yiffs only) */
{ (grid[x][y])++;       /* increment the counter (hits left) */
  drawwall(x,y);        /* redraw the wall */
  return;               /* and we're done. */
}                       /* note that there is no points, we lose this fox */

grid[x][y]=grid[x][y]+1; /* increment the grid count */
score[pl]++;            /* add to the player's score */
plgrid[pl][x][y]++;     /* add to the player's grid score for this cell */

xm=screenx(x);          /* initial offset within cell is just */
ym=screeny(y);          /* the cell's co-ordinates */

/* normal stacking */
/* once upon a time there were different stacking methods for different */
/* cells, thus this comment. */
if (grid[x][y]<3) ym=ym+TILEY;          /* if only 1 or 2, put in the bottom */
if (grid[x][y]%2==1) xm=xm+TILEX;       /* if an odd number, put to the right */

/* that's it, draw it into the background buffer */
blit(data[ch].dat,background,0,0,xm,ym,TILEX,TILEY);

/* check if we are at maximum for that cell.. */
if (grid[x][y]==max[x][y]-1)
  /* if yes, draw a rectange in colour 254 (which glows) around it */
  rect(background,screenx(x),screeny(y),screenx(x)+TILEX+TILEX-1,screeny(y)+TILEY+TILEY-1,254);

}

int doyiffs(int pl)
{ /* do all the yiffing! */
int yiffflag,yifframe;  /* flag for whether yiffs continue, and frame count */
int xi,yi,ch,t,en;      /* xi,yi are index counters. ch is a character temp */
                        /* variable, t is temp misc, and en is the enemy */
                        /* player number */
int r,aflag,loop;       /* r is the return, aflag and loop are misc */

r=1;                    /* initial return value is 1 */
aflag=0;                /* clear flag */
yiffflag=1;             /* we want to check for yiffs */
yifframe=0;             /* current yiff frame is 0 (none yet) */

/* continue until yiffflag reports there was a pass with no new */
/* yiffs detected. Holding SHIFT will force this loop to continue, */
/* which lets you see all the animations */
while ((yiffflag)||(key_shifts&KB_SHIFT_FLAG))
{ yiffflag=0;           /* no yiffs yet */
  retrace_count=0;      /* speed timer */
  for (xi=0; xi<Xcells; xi++)   /* we want to check the whole grid for */
    for (yi=0; yi<Ycells; yi++) /* cells to yiff in */
      if (grid[xi][yi]>=max[xi][yi])    /* if we're over the max... */
      { grid[xi][yi]+=YIFFON;           /* YIFF! Flag the cell, first */
        aflag=1;                        /* tell the next bit to animate */
      }                                 /* check it all first, we want */
                                        /* all the yiffs to happen in */
                                        /* the same pass, so we can not */
                                        /* scatter the pieces immediately */
                                        /* upon finding them, or it affects */
                                        /* the pattern */
  if ((aflag)||(key_shifts&KB_SHIFT_FLAG)) /* if we found one (or shift is on) */
  { if (pl==1)                          /* set up for player 1? */
    { chars[1]=2; chary[1]=DEFY;        /* player 2 gets hit (#2), put him on */
                                        /* the ground, just in case */
      if (charyoff[0][yifframe]!='0')   /* adjust Y offset from the string... */
      { if (charyoff[0][yifframe]>'Z')
          chary[0]=DEFY-((charyoff[0][yifframe]-'a')*4);
        else
          chary[0]=DEFY+((charyoff[0][yifframe]-'A')*4);
      } else
        chary[0]=DEFY;                  /* or put him on the ground for '0' */
      chars[0]=(int)((charanim[0][yifframe++])-'A');    /* set pattern */
      if (charanim[0][yifframe]==0) yifframe=0; /* wrap around if at end */
    }
    else                                /* player 2, the same stuff */
    { chars[0]=2; chary[0]=DEFY;
      if (charyoff[1][yifframe]!='0')
      { if (charyoff[1][yifframe]>'Z')
          chary[1]=DEFY-((charyoff[1][yifframe]-'a')*4);
        else
          chary[1]=DEFY+((charyoff[1][yifframe]-'A')*4);
      } else
        chary[1]=DEFY;
      chars[1]=(int)((charanim[1][yifframe++])-'A');
      if (charanim[1][yifframe]==0) yifframe=0;
    }
  }     /* animation sequence is complete */

  for (xi=0; xi<Xcells; xi++)   /* now we loop through and actually */
    for (yi=0; yi<Ycells; yi++) /* yiff the cells that need it. */
      if (grid[xi][yi]>YIFFON)  /* if this cell needs it... */
      { yiffflag=1;             /* we got one, set the flag to continue */
        /* then play the sound... */
        play_sample(data[YIFF].dat,180,128,charpitch[pl-1],FALSE);
        /* subtract the flag. Adding the flag value instead of just */
        /* setting it lets us save how many extras are in the cell, */
        /* from other yiffs, so we don't lose any. We could also */
        /* set a bit or something, but this works. We also have */
        /* to subtract how many pieces we're removing. */
        grid[xi][yi]=grid[xi][yi]-YIFFON-max[xi][yi];
        /* erase the cell on the screen */
        erasecell(xi,yi);

        if (pl==1)      /* this bit just sets the colour the foxes become */
        { ch=REDFOX;
        }
        else
        { ch=BLUEFOX;
        }

        for (en=1; en<3; en++)  /* 1, and 2. We have to remove the foxes */
                                /* being yiffed from the system, because */
                                /* (a) they may be changing colour, (b)  */
                                /* they may hit a wall, and (c) it lets  */
                                /* us just use drawfox() to put them back*/
        { score[en]=score[en]-plgrid[en][xi][yi];       /* remove from score */
          plgrid[en][xi][yi]=0;                         /* clear from grid */
        }

        /* loop to scatter all pieces even when some directions are blocked */
        /* once upon a time there was code in here for grids that could be */
        /* any size and any shape, as well, but it wasn't working. Some of */
        /* it applies in here. Also, more than 4 pieces in a square. */
        loop=max[xi][yi];       /* how many pieces? */
        while (loop>0)          /* start loop.. */
        { if (loop)                     /* these lines test the four */
            if (xi-1>=0)                /* directions for feasibility */
            { drawfox(xi-1,yi,ch,pl);   /* of scattering a foxhead there. */
              loop--;                   /* if okay, the new head is drawn, */
            }                           /* and loop decrements, so that */
          if (loop)                     /* we don't draw too many */
            if (xi+1<Xcells)            /* they used to test more and had */
            { drawfox(xi+1,yi,ch,pl);   /* to check more settings for */
              loop--;                   /* legality */
            }
          if (loop)
            if (yi-1>=0)
            { drawfox(xi,yi-1,ch,pl);
              loop--;
            }
          if (loop)
            if (yi+1<Ycells)
            { drawfox(xi,yi+1,ch,pl);
              loop--;
            }
        }                               /* end of loop */

        t=grid[xi][yi];                 /* save current cell value */
        grid[xi][yi]=0;                 /* erase it to zero */
        for (z=0; z<t; z++)             /* redraw the foxes in that cell */
        { /* redraw rest of grid */
          drawfox(xi,yi,ch,pl);         /* this fixes the grid[][] */
        }

        /* check for end of game? */
        if ((score[1]==0)||(score[2]==0))       /* game over if yiffed all */
        { r=0; yiffflag=0; }                    /* of one colour foxxy :)  */

  }     /* exit for */
  /* draw the screen */
  display();
  /* pause... */
  while (retrace_count<7);
  /* next yiff check */
}

/* send back the return code */
return(r);
}

int screenx(int x1)
{ /* return x screen coordinate for cell x1 */
return(x1*(TILEX*3)+xoff+TILEX);
}

int screeny(int y1)
{ /* do the same for y of y1 */
return(y1*(TILEY*3)+yoff+TILEY);
}

void erasecell(int x,int y)
{ /* black out a cell */
  /* this only affects the graphics, not the counters */
int tx,ty,mx,my;

tx=screenx(x);  /* get screen location */
ty=screeny(y);
mx=TILEX*2;     /* get size */
my=TILEY*2;
blit(picture,background,tx,ty,tx,ty,mx,my); /* blit from the picture */
}

void display()
{ /* update display */
/* this was originally supposed to be the screen display routine */
char buf[80];   /* temp string */

/* verify text mode */
text_mode(black);
/* get string of player names and scores */
sprintf(buf," %9s:  %d                                    %9s: %d      ",&playname[0][0],score[1],&playname[1][0],score[2]);
/* print it to the background */
textout_centre(background,data[FONT_001].dat,buf,320,440,0);
/* get string of wall counts */
sprintf(buf,"     Walls:  %d                                        Walls: %d      ",walls[1],walls[2]);
/* print it to the background */
textout_centre(background,data[FONT_001].dat,buf,320,450,0);
/* update the workspace from the background */
blit(background,work,0,0,0,0,640,480);
/* add the sprites */
draw_sprites();
/* copy it to the screen */
blit(work,screen,0,0,0,0,640,480);
}

void drawwall(int x,int y)
{ /* draw a wall at the appropriate cell */
int xm,ym,ch;   /* pixel location of cell, wall character to use */

xm=screenx(x);  /* get pixel location */
ym=screeny(y);

if (grid[x][y]==0)      /* if the wall is destroyed, erase the cell */
  erasecell(x,y);
else                    /* otherwise... */
{ ch=(WALLA)+(grid[x][y]+3);    /* get the correct sprite to draw */
  /* draw it into all four positions in the cell */
  blit(data[ch].dat,background,0,0,xm,ym,TILEX,TILEY);
  blit(data[ch].dat,background,0,0,xm+TILEX,ym,TILEX,TILEY);
  blit(data[ch].dat,background,0,0,xm,ym+TILEY,TILEX,TILEY);
  blit(data[ch].dat,background,0,0,xm+TILEX,ym+TILEY,TILEX,TILEY);
}
}

void draw_sprites()
{ /* draw the 2 character sprites */
/* this just uses the globals to draw the right sprites onto the */
/* workspace */

draw_sprite(work,character0[chars[0]].dat,charx[0],chary[0]-((BITMAP*)(character0[chars[0]].dat))->h);
draw_sprite(work,character1[chars[1]].dat,charx[1]-((BITMAP*)(character1[chars[1]].dat))->w,chary[1]-((BITMAP*)(character1[chars[1]].dat))->h);
}

void options(int *re)
{ char buf[80]; /* temp string */

  *re=0;        /* set return code, to variable passed by calling program */
  /* draw the 'window' for the options */
  rectfill(screen,0,96,639,284,black);
  rect(screen,0,96,639,284,0);
  text_mode(black);

  /* wait for ESC key release */
  while (key[KEY_ESC])
	  poll_keyboard();

  /* repeat till ESC is pressed (again), or quit or restart is picked */
  while ((!key[KEY_ESC])&&(*re==0))
  { /* display the text (yes, every frame) */
	  poll_keyboard();

    textout_centre(screen,data[FONT_002].dat,"Options",320,106,0);
    sprintf(buf,"MIDI Volume (use +/-): %3d",MIDIvol);
    textout_centre(screen,data[FONT_002].dat,buf,320,154,0);
    sprintf(buf,"DIGI Volume (use </>): %3d",DIGIvol);
    textout_centre(screen,data[FONT_002].dat,buf,320,178,0);
    textout_centre(screen,data[FONT_002].dat,"<ESC> To Return to Game",320,202,0);
    textout_centre(screen,data[FONT_002].dat,"<R> To Return to Menu",320,228,0);
    textout_centre(screen,data[FONT_002].dat,"<Q> To Quit to System",320,256,0);

    /* check keys, plus or equals increases midi volume... */    
    if (key[KEY_EQUALS])
    { if (MIDIvol<251) MIDIvol=MIDIvol+5;
      set_volume(DIGIvol,MIDIvol);
      while (key[KEY_EQUALS])
		  poll_keyboard();
    }
    if (key[KEY_PLUS_PAD])
    { if (MIDIvol<251) MIDIvol=MIDIvol+5;
      set_volume(DIGIvol,MIDIvol);
      while (key[KEY_PLUS_PAD])
		  poll_keyboard();
    }
    /* either minus key decreases midi volume... */
    if (key[KEY_MINUS])
    { if (MIDIvol>4) MIDIvol=MIDIvol-5;
      set_volume(DIGIvol,MIDIvol);
      while (key[KEY_MINUS])
		  poll_keyboard();
    }
    if (key[KEY_MINUS_PAD])
    { if (MIDIvol>4) MIDIvol=MIDIvol-5;
      set_volume(DIGIvol,MIDIvol);
      while (key[KEY_MINUS_PAD])
		  poll_keyboard();
    }
    /* period (STOP) or right arrow increases digital volume */
    if (key[KEY_STOP])
    { if (DIGIvol<251) DIGIvol=DIGIvol+5;
      set_volume(DIGIvol,MIDIvol);
      play_sample(data[BARK].dat,100,128,1000,FALSE);
      while (key[KEY_STOP])
		  poll_keyboard();
    }
    if (key[KEY_RIGHT])
    { if (DIGIvol<251) DIGIvol=DIGIvol+5;
      set_volume(DIGIvol,MIDIvol);
      play_sample(data[BARK].dat,100,128,1000,FALSE);
      while (key[KEY_RIGHT])
		  poll_keyboard();
    }
    /* comma or left arrow decreases digital volume.. */
    if (key[KEY_COMMA])
    { if (DIGIvol>4) DIGIvol=DIGIvol-5;
      set_volume(DIGIvol,MIDIvol);
      play_sample(data[BARK].dat,100,128,1000,FALSE);
      while (key[KEY_COMMA])
		  poll_keyboard();
    }
    if (key[KEY_LEFT])
    { if (DIGIvol>4) DIGIvol=DIGIvol-5;
      set_volume(DIGIvol,MIDIvol);
      play_sample(data[BARK].dat,100,128,1000,FALSE);
      while (key[KEY_LEFT])
		  poll_keyboard();
    }
    /* r means restart... set flags */
    if (key[KEY_R])
    { *re=RESTART; }
    /* and q means quit, same deal */
    if (key[KEY_Q])
    { *re=QUIT; }

  } /* exit loop */

  /* wait for ESC to be released (again) */
  while (key[KEY_ESC])
	  poll_keyboard();
}

void title()
{ int item,i,fader;     /* menu item, index counter, fade counter */
  BITMAP *t1,*t2;       /* two temp bitmaps */
  int yoff,of;          /* y offset of text, and options return code */

  yoff=170;             /* fixed value */
  fader=0;              /* not fading yet */

  fade_out(4);          /* fade to black */
  for (i=10; i>0; i--)  /* this fades out any music at about the same */
  { retrace_count=0;    /* speed as the screen fade */
    set_volume(DIGIvol,((MIDIvol*10)*(i*10))/1000);
    while (retrace_count<3); /* delay */
  }
  stop_midi();          /* stop the music */

  play_midi(data[MIDI_TIT].dat,1);      /* play title music */
  set_volume(DIGIvol,MIDIvol);          /* set volume */

  if (iloaded)                  /* if cycle254 is running.. */
  { remove_int(cycle254);       /* turn it off */
    iloaded=0;                  /* and flag it off */
  }

/* another evil label for an evil GOTO command */
menu:

  /* -1 is transparent */
  text_mode(-1);

  /* I cheat and grab a pixel from the logo bitmap to ensure that */
  /* the screen clears to the same colour as it's background. */
  clear_to_color(work,getpixel(data[Z_LOGO].dat,0,0));

  /* draw the logo */
  blit(data[Z_LOGO].dat,work,0,0,0,0,640,((BITMAP*)(data[Z_LOGO].dat))->h);
  /* print the text */
  textout(work,data[FONT_002].dat,"D)emo",155,yoff,white);
  textout(work,data[FONT_002].dat,"1) Player game",155,yoff+24,white);
  textout(work,data[FONT_002].dat,"2) Player game",155,yoff+48,white);
  textout(work,data[FONT_002].dat,"H)elp",155,yoff+72,white);
  textout(work,data[FONT_002].dat,"A)bout",155,yoff+96,white);
  textout(work,data[FONT_002].dat,"O)ptions",155,yoff+120,white);
  textout(work,data[FONT_002].dat,"Q)uit to OS",155,yoff+144,white);
  textout_centre(work,data[FONT_002].dat,"(c)1998 M.Brent",320,440,white);

  /* copy the workspace to the screen */
  blit(work,screen,0,0,0,0,640,480);

  if (fader==0)                         /* if not faded yet */
  { fade_in(data[PALLETE_001].dat,4);   /* fade in */
    fader=1;                            /* and set flag */
  }

  item=0;                               /* current item unselected */
  while (item==0)                       /* wait for an item */
  { rand();                             /* random number playing */
    poll_keyboard();
    if (key[KEY_D])                     /* check for Demo game */
    { item=1;                           /* item number */
      players=0;                        /* players are zero */
      while (key[KEY_D])               /* wait for key release */
		  poll_keyboard();
    }
    if (key[KEY_1])                     /* check for 1 player */
    { item=2;                           /* item number */
      players=1;                        /* 1 player */
      while (key[KEY_1])               /* wait for key.. */
		  poll_keyboard();
    }
    if (key[KEY_2])                     /* check for 2 players */
    { item=3;                           /* you know the drill */
      players=2;
      while (key[KEY_2])
		  poll_keyboard();
    }
    if (key[KEY_H])                     /* check for help */
    { item=4;
      while (key[KEY_H])
		  poll_keyboard();
    }
    if (key[KEY_A])                     /* check for about */
    { item=5;
      while (key[KEY_A])
		  poll_keyboard();
    }
    if (key[KEY_O])                     /* check for options */
    { item=6;
      while (key[KEY_O])
		  poll_keyboard();
    }
    if (key[KEY_Q])                     /* check for quit */
    { item=7;
      while (key[KEY_Q])
		  poll_keyboard();
    }
  }             /* got something.. */

  t1=create_bitmap(330,24);     /* make two small workspaces for */
  t2=create_bitmap(330,24);     /* flickering menu highlight */

  if ((t1)&&(t2))               /* if they didn't create okay, */
                                /* just skip this part. */
  { /* get the original text graphics there */
    blit(screen,t1,155,(yoff-24)+item*24,0,0,330,24);
    /* draw a shaded inverted box over it */
    drawing_mode(DRAW_MODE_XOR,NULL,0,0);
    rectfill(screen,155,(yoff-24)+item*24,485,yoff+item*24-1,black);
    /* grab a copy of this to the other buffer */
    blit(screen,t2,155,(yoff-24)+item*24,0,0,330,24);
    /* reset the drawing mode */
    solid_mode();
    /* now flicker them back and forth a bit */
    for (i=10; i>0; i--)
    { /* if we're starting a game, also fade the volume of the MIDI */
      if (item<4)
        set_volume(DIGIvol,((MIDIvol*10)*(i*10))/1000);

      retrace_count=0;  /* timer */

      /* draw one, and pause... */
      blit(t2,screen,0,0,155,(yoff-24)+item*24,330,24);
      while (retrace_count<5);

      /* then the other, and pause... */
      blit(t1,screen,0,0,155,(yoff-24)+item*24,330,24);
      while (retrace_count<7);
    }
  }

  if (t1) destroy_bitmap(t1);   /* free the memory if we used it */
  if (t2) destroy_bitmap(t2);

  if (item<4)                   /* if starting a game, */
  { stop_midi();                /* stop the music, */
    set_volume(DIGIvol,MIDIvol);/* and reset the volume. */
  }

  if (item==4)                  /* run the help menu */
  { help();
    goto menu;
  }

  if (item==5)                  /* do I really need to explain this? */
  { about();
    goto menu;
  }

  if (item==6)                  /* this one, yes. It calls the options */
  { options(&of);               /* menu, which was meant more for ingame. */
    if (of==QUIT)               /* thus it saves the return code, and */
      leave();                  /* checks for quit, exitting if given. */
    else goto menu;             /* it ignores restart. */
  }

  if (item==7)                  /* this is also quit */
    leave();                    /* clean up and bye */
}

void charselectr()
{ /* routine to select the player's characters */

  DATAFILE *temp;               /* used for the charselectr data file */
  int i,j;                      /* index counters */
  int w[4],h[4],x[4],y[4];      /* used to display the graphics right */
  char name[4][10];             /* each character's name */

/* this routine is pretty rushed.. I have about 90 minutes to finish this */
/* program before I hop on a plane to give it to Foxx... ;) */

/* note: I made it. ;) */

  clear_to_color(screen,black);         /* black screen */
  temp=load_datafile("select.dat");     /* load the datafile */
  if (temp==NULL)
  { charp0=rand()%4;    /* if we can't load this datafile, then we */
    charp1=rand()%4;    /* just select a random character for each */
    return;             /* player and be done with it. */
  }

  play_midi(data[ZZ_SELECT].dat,TRUE);  /* play the music */

  /* load names */
  strcpy(&name[0][0],"Yiff Li");
  strcpy(&name[1][0],"Ryiff");
  strcpy(&name[2][0],"Zang'yiff");
  strcpy(&name[3][0],"Bob");

  /* load arrays for character sizes and calculate positions */
  j=32;  /* start column */
  for (i=0; i<4; i++)                   /* this routine reads the sizes */
  { w[i]=((BITMAP*)(temp[i*2].dat))->w; /* of each character and figures */
    h[i]=((BITMAP*)(temp[i*2].dat))->h; /* out where to draw them on the */
    y[i]=290-h[i];                      /* screen so they don't overlap */
    x[i]=j;                             /* first X is 32, and they all */
    j=j+w[i]+20;                        /* have their feet at Y=290 */
  }                                     /* with 20 pixels in between */

  if (iloaded==0)                       /* if the cycle254 isn't loaded */
  { if (install_int(cycle254,50))       /* try to load it */
      fail("Can not install interrupt routine cycle254()\n"); /* fail if not */
    else iloaded=1;                     /* set the flag if yes */
  }

  /* display the text */
  textout_centre(screen,data[FONT_002].dat,"Character Select",320,50,white);

  /* draw characters */
  for (i=0; i<4; i++)   /* uses the tables we calculated above */
  { draw_sprite(screen,temp[i*2].dat,x[i],y[i]);        /* red side */
    draw_sprite(screen,temp[i*2+S_P2_OFF].dat,x[i]+320,y[i]); /* blue side */
  }

  text_mode(black);     /* text background black */

  /* player 1 */
  if (players<1)        /* if 0 players, pick a random player 1 */
  { charp0=rand()%4;
    textout_centre(screen,data[FONT_002].dat,&name[charp0][0],160,390,white);
  }
  else                  /* else human gets to pick */
  { charp0=0;           /* current choice */
    /* space or enter to select... */
    while ((key[KEY_SPACE]==0)&&(key[KEY_ENTER]==0))
    { /* draw a glowing rectangle, use the sizes we got above */
	  poll_keyboard();
      rect(screen,x[charp0]-5,y[charp0]-5,x[charp0]+w[charp0]+5,290+5,254);
      /* print the current character's name */
      /* note that all this goes right to the screen, not the workspace */
      /* that's because it was rushed so badly */
      textout_centre(screen,data[FONT_002].dat,&name[charp0][0],160,390,white);

      /* parse keys */
      if (key[KEY_LEFT])
      { play_sample(data[POC].dat,80,128,1000,FALSE);   /* play sound */
        while (key[KEY_LEFT])          /* wait for key release */
			poll_keyboard();

        /* erase rectangle */
        rect(screen,x[charp0]-5,y[charp0]-5,x[charp0]+w[charp0]+5,290+5,black);
        /* erase text */
        textout_centre(screen,data[FONT_002].dat,"          ",160,390,white);

        /* change selection */
        charp0--;
        if (charp0<0) charp0=3; /* wrap around if needed */
      }

      if (key[KEY_RIGHT])       /* the same idea for right */
      { play_sample(data[POC].dat,80,128,1000,FALSE);
        while (key[KEY_RIGHT])
			poll_keyboard();
        rect(screen,x[charp0]-5,y[charp0]-5,x[charp0]+w[charp0]+5,290+5,black);
        textout_centre(screen,data[FONT_002].dat,"          ",160,390,white);
        charp0++;
        if (charp0>3) charp0=0;
      }
    }
  }     /* now we have one... */

  /* announce the name */
  play_sample(temp[charp0+S_SAM_OFF].dat,80,128,1000,FALSE);
  /* erase the rectangle */
  rectfill(screen,x[charp0]-5,y[charp0]-5,x[charp0]+w[charp0]+5,290+5,black);
  /* select the sprite number for the character's win pose */
  j=charp0*2+1;
  /* and draw it, overwriting the old sprite */
  draw_sprite(screen,temp[j].dat,x[charp0],290-((BITMAP*)(temp[j].dat))->h);

  /* save the name */
  strcpy(&playname[0][0],&name[charp0][0]);

  /* wait about a second */
  rest(1000);

  /* wait for key release */
  while ((key[KEY_SPACE])||(key[KEY_ENTER]))
	  poll_keyboard();

  /* print the word 'vs' */
  textout_centre(screen,data[FONT_002].dat,"VS",320,390,white);

  /* player 2 */
  /* this part works exactly the same way, it's a cut and paste with */
  /* the numbers changed. So I'm not adding comments to it. ;) */
  if (players<2)
  { charp1=rand()%4;
    textout_centre(screen,data[FONT_002].dat,&name[charp1][0],160+320,390,white);
  }
  else
  { charp1=0;
    while ((key[KEY_SPACE]==0)&&(key[KEY_ENTER]==0))
	{
	  poll_keyboard();
      rect(screen,x[charp1]-5+320,y[charp1]-5,x[charp1]+w[charp1]+5+320,290+5,254);
      textout_centre(screen,data[FONT_002].dat,&name[charp1][0],160+320,390,white);
      if (key[KEY_LEFT])
      { play_sample(data[POC].dat,80,128,1000,FALSE);
        while (key[KEY_LEFT])
			poll_keyboard();
        rect(screen,x[charp1]-5+320,y[charp1]-5,x[charp1]+w[charp1]+5+320,290+5,black);
        textout_centre(screen,data[FONT_002].dat,"          ",160+320,390,white);
        charp1--;
        if (charp1<0) charp1=3;
      }
      if (key[KEY_RIGHT])
      { play_sample(data[POC].dat,80,128,1000,FALSE);
        while (key[KEY_RIGHT])
			poll_keyboard();
        rect(screen,x[charp1]-5+320,y[charp1]-5,x[charp1]+w[charp1]+5+320,290+5,black);
        textout_centre(screen,data[FONT_002].dat,"          ",160+320,390,white);
        charp1++;
        if (charp1>3) charp1=0;
      }
    }
  }
  play_sample(temp[charp1+S_SAM_OFF].dat,80,128,1000,FALSE);
  rectfill(screen,x[charp1]-5+320,y[charp1]-5,x[charp1]+w[charp1]+5+320,290+5,black);
  j=charp1*2+1;
  draw_sprite(screen,temp[j+S_P2_OFF].dat,x[charp1]+320,290-((BITMAP*)(temp[j].dat))->h);

  strcpy(&playname[1][0],&name[charp1][0]);

  rest(1000);

  while ((key[KEY_SPACE])||(key[KEY_ENTER]))
	  poll_keyboard();

  remove_int(cycle254);         /* remove interrupt */
  iloaded=0;                    /* flag that it's off */

  fade_out(4);                  /* fade screen */
  for (i=10; i>0; i--)          /* fade music.. you've seen this enough */
  { retrace_count=0;
    set_volume(DIGIvol,((MIDIvol*10)*(i*10))/1000);
    while (retrace_count<3);
  }
  stop_midi();                  /* stop music */
  unload_datafile(temp);        /* free memory */
}

void help()
{ /* this routine just displays the help page - which is 2 pictures */
  /* in the main datafile. */

/* display logo */
blit(data[Z_LOGO].dat,work,0,0,0,0,640,((BITMAP*)(data[Z_LOGO].dat))->h);
/* display help */
blit(data[Z_HELP].dat,screen,0,0,0,117,640,363);
/* wait for ESC key */
while (key[KEY_ESC]==0)
	poll_keyboard();
}

void about()
{ /* this is like help, but it prints it's information instead of using */
  /* a big picture. ;) */

  /* set transparent text */
  text_mode(-1);
  /* clear screen, as before with the trick */
  clear_to_color(screen,getpixel(data[Z_LOGO].dat,0,0));
  /* draw the logo */
  blit(data[Z_LOGO].dat,screen,0,0,0,0,640,((BITMAP*)(data[Z_LOGO].dat))->h);
  /* print all the text */
  textout_centre(screen,data[FONT_002].dat,"(c)1998 Mike Brent (Tursi)",320,124,white);
  textout_centre(screen,data[FONT_001].dat,"Created as a Birthday Gift for my mate, Roy Crump (Foxxfire)",320,160,white);
  textout_centre(screen,data[FONT_001].dat,"(but not finished until much, much later... tsk on me.)",320,170,white);
  textout_centre(screen,data[FONT_001].dat,"Created using DJGPP by D.J. Delories (http://www.delorie.com/djgpp/)",320,190,white);
  textout_centre(screen,data[FONT_001].dat,"and the Allegro Game Library by Shawn Hargreaves",320,200,white);
  textout_centre(screen,data[FONT_001].dat,"(http://www.talula.demon.co.uk/allegro/)",320,210,white);
  textout_centre(screen,data[FONT_001].dat,"Background Graphics from various clipart collections",320,220,white);
  textout_centre(screen,data[FONT_001].dat,"MIDIs from around the net (I know the title is Bach ;) )",320,230,white);
  textout_centre(screen,data[FONT_001].dat,"Characters grabbed from Street Fighter 2 by Capcom using",320,240,white);
  textout_centre(screen,data[FONT_001].dat,"the Genecyst Genesis Emulator by Bloodlust Software",320,250,white);
  textout_centre(screen,data[FONT_001].dat,"Adobe Photoshop 3.0 and Thumbs Plus! 3.0 used for image editing",320,260,white);
  textout_centre(screen,data[FONT_001].dat,"and conversion.",320,270,white);
  textout_centre(screen,data[FONT_001].dat,"Battling foxes idea by Steve Brent - that's why it's so late ;)",320,290,white);
  textout_centre(screen,data[FONT_001].dat,"Contact me: http://www.neteng.bc.ca/~tursi",320,310,white);
  textout_centre(screen,data[FONT_001].dat,"Email: tursi@neteng.bc.ca",320,320,white);
  textout_centre(screen,data[FONT_001].dat,"And visit Foxxfire's webpage at",320,340,white);
  textout_centre(screen,data[FONT_001].dat,"http://www.cyberhighway.net/~royvk/",320,350,white);
  textout_centre(screen,data[FONT_001].dat,"Try 'yiff -greet' for more babble",320,370,white);
  textout_centre(screen,data[FONT_002].dat,"ENJOY! :)",320,410,white);
  /* wait for ESC */
  while (key[KEY_ESC]==0)
	  poll_keyboard();
}

void greet()
{ /* standard greetings page */
/* my programs print this out when you do a -greet command-line switch */
/* it couldn't be more basic. */
/* it never returns. */
printf("\nA happy birthday and very late snugs to my mate, Foxxfire, who, since I\n");
printf("started this game, has become my fiancee', and.. well, tommorrow, I guess,\n");
printf("we'll be exchanging rings and vows to start our engagement, leading hopefully\n");
printf("to life and love together for many years to come. It's 13 Jan 1998, and it's\n");
printf("snowing outside, pretty hard, too. I guess I could say 'finally', but I\n");
printf("don't like the snow. It's also 10 minutes after midnight, and I've crammed\n");
printf("for the last two days to get this game presentable. :) I hope Foxx likes it,\n");
printf("but I hope to avoid it for a while.. is no fun by myself. :) The source code\n");
printf("is just shy of 1200 lines, which isn't HUGE, but isn't bad, either. ;) With\n");
printf("all the graphics data, it's certainly the biggest game I've ever written.\n");
printf("Must say I love DJGPP as a *good* 32-bit DOS-based compiler, cause the\n");
printf("limits of the 16-bit ones after working on the Atari and Amiga were *really*\n");
printf("bugging me. :) Segments and offsets, my ass. I also love the Allegro library\n");
printf("cause it means someone ELSE did all the ugly hardware stuff for me. ;)\n");
printf("A 'hi!' to my brother, for suggesting the battling foxes who have made this\n");
printf("game (a) so big, and (b) soooo late. (was due on Aug 16th! ok, I DID put\n");
printf("it off a little, but was cause of all the graphics work!) Also, no more\n");
printf("MIDIs! Ick! I no sooner find an acceptable MIDI (that, in itself, a task),\n");
printf("than I learn it's miserable under Allegro's player. MODs and other digital\n");
printf("audio for the future. Hey, press a key for more...\n");
while (!keypressed());
while (keypressed())
  readkey();
printf("\nThanks. :)\n");
printf("So, yeah, hope you get some enjoyment out of this game. :) It's a little\n");
printf("beyond the silly idea I originally had. Oh, I hafta mention Lawrence, too,\n");
printf("for a couple of cool ideas that just came too late to implement. (But hey, I\n");
printf("might take those ideas and rebuild the engine to implement what YOU were\n");
printf("thinking, someday. Sigh... always 'someday'.) I'm also gonna risk a 'hi'\n");
printf("to Orzel, whom I ticked off a bit when the mod chip I installed in his PSX\n");
printf("didn't work after it was shipped. I'll get that fixed.. that's a priority.\n");
printf("Oh, don't tell Foxx, but I got him a really nice little silver crystal fox\n");
printf("as an engagement present. :) I would've got the bigger one, but his muzzle\n");
printf("was way too long... tsk. But this guy is cute. :) Well, I keep saying I'm\n");
printf("gonna do Super Space Acer, and I have Allegro 3.0 now, MikAlleg for MODs,\n");
printf("and a CD-Audio library for that version, so in theory I'm all set. Almost\n");
printf("scary, but it's time. Is sorta like Greg was saying when I mentioned SSA\n");
printf("a couple of weeks ago: 'Super Space Acer? That's achieved legendary status\n");
printf("by now.' Heh... guess so. I wonder if it'll come out anything like I\n");
printf("planned? Well, Keady and Foxxfire are doing some of the art for me, I\n");
printf("still have to find a musician unless Canis' life has settled down some.\n");
printf("But lotsa time... I'm hoping, though, that I might have something showable\n");
printf("for ConFURence '99 (January).. that gives me 1 year. If I bust my butt, I\n");
printf("should be able to do it. Oh, hey, hit a key again, will ya?\n");
while (!keypressed());
while (keypressed())
  readkey();
printf("\n(Sorry that you did -greet yet? ;) )\n");
printf("Well, tsk, now the source code is at 1223 lines. No matter, I'm\n");
printf("almost done. SSA is a 3D polygon shooter.. I was gonna do it in DirectX,\n");
printf("but I think I'll do it in DOS/Allegro after all... I have everything I\n");
printf("want there, now. I'll chance it.. I think DOS will be an okay platform\n");
printf("for a few more years. (Should see how NT likes this compiler, I guess...)\n");
printf("Oh yeah, I'm off to ConFURence tommorrow, that's where Foxx and I are doing\n");
printf("our ring exchange. Looking forward to that... Foxx is an artist, did I\n");
printf("mention that? (Or are you just blinded by all the text? Hey! I have a\n");
printf("*long* time to catch up on since my last greet page.) Well, tsk, I will\n");
printf("*never* get to bed at this rate. Thanks for reading... if you don't wanna\n");
printf("see the mushy stuff, stop reading now.\n");
printf("Love you Foxx, hope that what we have will last for the coming months\n");
printf("and years, that each of us treasures our time together, and that I\n");
printf("don't horribly fumble my vows tommorrow night. :) <kiss>\n");
printf("\n\nM.Brent (Tursi!) - 13 jan 98\n");
exit(0);
}
