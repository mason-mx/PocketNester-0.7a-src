GAPI Emulator v0.95 - ReadMe file - January 31, 2002
====================================================

Author  : Thierry Tremblay
E-mail  : thierry_tremblay@bigfoot.com
Web site: frogengine.net-connect.net



LICENCE
=======

All the files included in the PocketFrog archive (zip) are copyrighted by me,
Thierry Tremblay. Some parts may have been contributed by other authors and
will be indicated as such.

Common sense applies here: if I didn't want peoples to use the GAPI Emulator
and/or cut&paste code snippets into their own library/game, I wouldn't be
putting it on the web for free. If I didn't want peoples to contribute to
this project, I wouln't put it on the web either.


There is only one rule:

   You are not allowed to distribute modified versions of the GAPI Emulator
   archive. If you want fixes and/or code added to the library, you should
   contact me. If this is of general interest and fit within the goals of
   GAPI Emulator, I will add it.

   I will consider any demands to include the GAPI Emulator with your own
   archive and/or book.




LINKAGE ISSUES
==============

I regularly receive mails from people having problems with the API's calling
convention (__cdecl vs __stdcall), so here is an explanation of what is going
on:

When you create a x86 emulator project with the Pocket PC 2000 SDK, the default
calling convention of your project becomes "__stdcall". You can see this by
looking at the project's settings: the "/Gz" compiler switch is used. But when
you use the Pocket PC 2002 SDK, the default callin convention is now set to
"__cdecl". Note that all Pocket PC targets (ARM, MIPS, SH3) have no issue with
this because they only support one calling convention and the different
compilers don't care about the "__cdecl" and "__stdcall" declarations.

With version 0.94, I was forcing the calling convention to "__stdcall", but
this created more problems as peoples using the Pocket PC 2002 SDK would start
linking the GX.LIB that comes with the SDK instead of the one compiled with the
emulator, resulting in instant lookup of their application.

Starting with version 0.95, there is different version depending on whether you
are using the Pocket PC 2000 or Pocket PC 2002 SDK. The version compiled for
the Pocket PC 2002 SDK is compatible with the GX.LIB that comes with the SDK.
Because of this, you shouln't have any more issues with the calling conventions.

There is no more need for the "GAPI.h" that was previously included with the
GAPI emulator since there is now a separate version for each SDK.




CREDITS
=======

I'd like to thanks everyone who provided me with feedback on how the emulator
is working for them. Some even pointed out problems, bugs, provided fixes, and
got involved somehow. I also want to thanks peoples who simply drop a mail to
tell me what the configuration of their device's display is... This information
is invaluable to all GAPI developpers.

Special thanks to:

   Tristan Savatier,
      Who made sure that the GAPI emulator was properly emulating devices.

   Jim Barry
      Found a few remaining bugs, but most importantly, was the one who made me
      understand why so many peoples had "unresolved externals" problems... It
      was the calling convention being different in the PocketPC 2002 SDK.

   Stephane (aka Boris)
      He motivated me to write this emulator so that he could debug his Space
      Invaders emulator.



WHAT'S NEW IN VERSION 0.95
==========================

   + Two targets are now defined: Pocket PC 2000 and Pocket PC 2002 SDKs
   ! Task bar is now hidden in GXOpenDisplay() to emulate real devices.
   * Default configuration is now "SIMPLE_565" instead of IPAQ_36xx.




WHAT'S NEW IN VERSION 0.94
==========================

   * Buffer offset was off by one for configs where pitchY < 0.




WHAT'S NEW IN VERSION 0.93
==========================

   * Color palette was not set properly.




WHAT'S NEW IN VERSION 0.92
==========================

   ! Some configuration entries were missing a "," before the last column
   ! Changed the name "Jordana" to "Jornada"
   + Explanation of linkage issues when using Pocket PC 2000/2002 and GX.h
   * Changed calling convention from "__cdecl" to "__stdcall"




WHAT'S NEW IN VERSION 0.91
==========================

   ! Video pointer was not calculated properly for monochrome displays.
   ! Fixed a bug in GXSuspend() that would crash the program if no display was
     currently initialized.
   + Support for 320x240 display on desktop PC.




WHAT'S NEW IN VERSION 0.9
=========================

   + Cleanup / restructuration of the code (now easier to use / understand).
   + New header file: "GAPI.h to specify the calling convention of GX functions.
   + Support for "padded" surfaces where the pitch != width * 2.
   + Configurations to emulate different devices easily.
   + Palettized display support (palette now forced to 332).




WHAT'S NEXT
===========

The emulator now looks like a complete product... I am quite satisfied with it.
I'm able to simulate the main devices (Casio E-125, iPAQ 36xx, iPAQ 38xx). It
is not possible for me to test all configurations, and there is probably some
bugs still lurking in there.

I don't have the intention to add new things to GAPI Emulator, as I want to
concentrate on PocketFrog. When this version has been out in the field for a
while and that I am satisfied that it works properly, it will make a new
release, version 1.0.




---
