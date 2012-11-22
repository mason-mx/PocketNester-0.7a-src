//////////////////////////////////////////////////////////////////////////////
//
// GAPI for the x86 PocketPC emulator
// Copyright 2000, 2001, 2002 by Thierry Tremblay
//
// http://frogengine.net-connect.net
//
//////////////////////////////////////////////////////////////////////////////
//
// The default configuration is set to emulate a simple device. To change
// this, search for the "l_config" variable, right after the list of
// predefined configurations, and select the one you want to use.
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <aygshell.h>
#include "gx.h"



//////////////////////////////////////////////////////////////////////////////
//
// Configuration of emulator
//
//////////////////////////////////////////////////////////////////////////////

struct Config
{
   int      width, height;    // Dimensions
   long     pitchX, pitchY;   // Pitches (GAPI convention)
   long     bpp;              // Color depth in bits
   unsigned format;           // Pixel format
   BOOL     bDRAMBuffer;      // Is the display buffered?
};



enum Orientation
{
   ORIENTATION_LEFT  = 1,
   ORIENTATION_UP    = 0,
   ORIENTATION_RIGHT = -1
};



// Lines ending with "//?" means that the bDRAMBuffer value is not confirmed.
// The iPAQ 38xx is reported with "kfLandscape" as there is no "kfInvertedLandscape" flag yet...

#define AMIGO               { 240, 320, 640,  -2, 16, kfDirect | kfDirect565 | kfLandscape,      FALSE } //?
#define AERO_1500_CONTRAST  { 240, 320,  80,  -0,  2, kfDirect | kfLandscape | kfDirectInverted, FALSE } //?
#define AERO_1500_QUALITY   { 240, 320, 160,  -0,  4, kfDirect | kfLandscape | kfDirectInverted, FALSE } //?
#define AERO_2100           { 240, 320, 640,  -2, 16, kfDirect | kfDirect565 | kfLandscape,      FALSE } //?
#define CASIO_E125          { 240, 320,   2, 512, 16, kfDirect | kfDirect565,                    TRUE  }
#define CASIO_EM500         { 240, 320,   2, 512, 16, kfDirect | kfDirect565,                    TRUE  } //?
#define IPAQ_31xx           { 240, 320, 160,  -0,  4, kfDirect | kfLandscape | kfDirectInverted, FALSE } //?
#define IPAQ_36xx           { 240, 320, 640,  -2, 16, kfDirect | kfDirect565 | kfLandscape,      FALSE }
#define IPAQ_38xx           { 240, 320,-640,   2, 16, kfDirect | kfDirect565 | kfLandscape,      FALSE }
#define JORNADA_525         { 240, 320,   1, 240,  8, kfPalette,                                 FALSE } //?
#define JORNADA_54x         { 240, 320,   2, 480, 16, kfDirect | kfDirect565,                    FALSE } //?
#define JORNADA_54x_PALETTE { 240, 320,   1, 240,  8, kfPalette,                                 FALSE } //?
#define JORNADA_56x         { 240, 320,   2, 480, 16, kfDirect | kfDirect565,                    FALSE } //?

#define SIMPLE_565          { 240, 320,   2, 480, 16, kfDirect | kfDirect565,                    TRUE }
#define SIMPLE_555          { 240, 320,   2, 480, 16, kfDirect | kfDirect555,                    TRUE }
#define SIMPLE_444          { 240, 320,   2, 480, 16, kfDirect | kfDirect444,                    TRUE }
#define SIMPLE_0888         { 240, 320,   4, 960, 32, kfDirect | kfDirect888,                    TRUE }
#define SIMPLE_PALETTE      { 240, 320,   1, 240,  8, kfPalette,                                 TRUE }
#define SIMPLE_MONO_1       { 240, 320,   0,  30,  1, kfDirect,                                  TRUE }
#define SIMPLE_MONO_2       { 240, 320,   0,  60,  2, kfDirect,                                  TRUE }
#define SIMPLE_MONO_4       { 240, 320,   0, 120,  4, kfDirect,                                  TRUE }
#define SIMPLE_MONO_8       { 240, 320,   1, 240,  8, kfDirect,                                  TRUE }

#define LANDSCAPE_565       { 240, 320, 480,  -2, 16, kfDirect | kfDirect565,                    TRUE }
#define LANDSCAPE_555       { 240, 320, 480,  -2, 16, kfDirect | kfDirect555,                    TRUE }
#define LANDSCAPE_444       { 240, 320, 480,  -2, 16, kfDirect | kfDirect444,                    TRUE }
#define LANDSCAPE_0888      { 240, 320, 960,  -4, 32, kfDirect | kfDirect888,                    TRUE }
#define LANDSCAPE_PALETTE   { 240, 320, 240,  -1,  8, kfPalette,                                 TRUE }
#define LANDSCAPE_MONO_1    { 240, 320,  30,  -0,  1, kfDirect,                                  TRUE }
#define LANDSCAPE_MONO_2    { 240, 320,  60,  -0,  2, kfDirect,                                  TRUE }
#define LANDSCAPE_MONO_4    { 240, 320, 120,  -0,  4, kfDirect,                                  TRUE }
#define LANDSCAPE_MONO_8    { 240, 320, 240,  -1,  8, kfDirect,                                  TRUE }

#define CUSTOM              { 240, 320, ... }



//////////////////////////////////////////////////////////////////////////////
//
// Local variables
//
//////////////////////////////////////////////////////////////////////////////

namespace
{
   // Configuration, select what you want from above list
   Config      l_config         = SIMPLE_565;

   // Internal variables
   HWND        l_hwnd           = 0;     // Handle of display window
   bool        l_bSuspended     = false; // GAPI is suspended?
   HBITMAP     l_hBitmap        = 0;     // Bitmap emulating the display
   BYTE*       l_pBitmapBits    = 0;     // The bitmap's bits
   BYTE*       l_pVideoBits     = 0;     // Pointer to video memory
   BYTE*       l_pBufferBits    = 0;     // Bits used to emulate landscaped devices
   BYTE*       l_pGAPIPointer   = 0;     // Pointer returned by GAPI
   int         l_viewportTop    = 0;     // Viewport top
   int         l_viewportHeight = 0;     // Viewport height
   Orientation l_orientation    = ORIENTATION_UP;


   // Tables used by monochrome blit functions
   const BYTE l_table2bpp[4] =
   {
      0x00, 0x55, 0xAA, 0xFF
   };

   const BYTE l_table4bpp[16] =
   {
      0x00, 0x11, 0x22, 0x33,
      0x44, 0x55, 0x66, 0x77,
      0x88, 0x99, 0xAA, 0xBB,
      0xCC, 0xDD, 0xEE, 0xFF
   };
}



//////////////////////////////////////////////////////////////////////////////
//
// CreateColorDIB() - Create a color display
//
//////////////////////////////////////////////////////////////////////////////

static void CreateColorDIB()
{
   bool bWillUseBuffer = l_orientation != ORIENTATION_UP;

   // If we are not going to use an intermediate buffer,
   // we have to take padding into account at this point.
   int width  = l_config.width;
   int height = l_config.height;

   if ( !bWillUseBuffer )
   {
      width = abs( l_config.pitchY ) * 8 / l_config.bpp;
   }

   // Create the DIB
   BYTE buffer[sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD)];

   // Handy pointers
   BITMAPINFO*       pBMI    = (BITMAPINFO*) buffer;
   BITMAPINFOHEADER* pHeader = &pBMI->bmiHeader;
   DWORD*            pColors = (DWORD*)&pBMI->bmiColors;   

   // DIB Header
   pHeader->biSize            = sizeof(BITMAPINFOHEADER);
   pHeader->biWidth           = width;
   pHeader->biHeight          = -height;
   pHeader->biPlanes          = 1;
   pHeader->biBitCount        = (WORD)l_config.bpp;
   pHeader->biCompression     = BI_BITFIELDS;
   pHeader->biSizeImage       = (width * height * l_config.bpp) / 8;
   pHeader->biXPelsPerMeter   = 0;
   pHeader->biYPelsPerMeter   = 0;
   pHeader->biClrUsed         = 0;
   pHeader->biClrImportant    = 0;

   // For left oriented displays, we must flip the bitmap upside down
   if (l_orientation == ORIENTATION_LEFT)
   {
      pHeader->biHeight *= -1;
   }

   // Color masks
   if (l_config.format & kfDirect555)
   {
      pColors[0] = 0x1F << 10;
      pColors[1] = 0x1F << 5;
      pColors[2] = 0x1F;
   }
   else if (l_config.format & kfDirect565)
   {
      pColors[0] = 0x1F << 11;
      pColors[1] = 0x3F << 5;
      pColors[2] = 0x1F;
   }
   else if (l_config.format & kfDirect888)
   {
      pColors[0] = 0x00FF0000;
      pColors[1] = 0x0000FF00;
      pColors[2] = 0x000000FF;
   }
   else if (l_config.format & kfDirect444)
   {
      pColors[0] = 0x0F00;
      pColors[1] = 0x00F0;
      pColors[2] = 0x000F;
   }

   // Create the DIB
   l_hBitmap    = CreateDIBSection( 0, pBMI, DIB_RGB_COLORS, (void**)&l_pBitmapBits, 0, 0 );
   l_pVideoBits = l_pBitmapBits;   
}



//////////////////////////////////////////////////////////////////////////////
//
// SetupColorPalette() - Used to set up the palette on palettized displays
//
//////////////////////////////////////////////////////////////////////////////

static void SetupColorPalette( DWORD* pPalette )
{
   // The question here is what palette are we supposed to use?

   // For now, we create a 332 color palette
   for (int i = 0; i < 256; ++i)
   {
      // Find each color's bit pattern
      int rbits = (i >> 5) & 0x07;
      int gbits = (i >> 2) & 0x07;
      int bbits = (i >> 0) & 0x03;

      // Find intensity by replicating the bit patterns over a byte
      int r = (rbits << 5) | (rbits << 2) | (rbits >> 1);
      int g = (gbits << 5) | (gbits << 2) | (gbits >> 1);
      int b = (bbits << 6) | (bbits << 4) | (bbits << 2) | bbits;

      pPalette[i] = RGB( b, g, r );
   }
}



//////////////////////////////////////////////////////////////////////////////
//
// SetupMonochromePalette() - Used to set up the palette on monochrome displays
//
//////////////////////////////////////////////////////////////////////////////

static void SetupMonochromePalette( int nbColor, DWORD* pPalette, bool bInverted )
{
   if (bInverted)
   {
      for (int i = 0; i < nbColor; ++i)
      {
         int c = (255 * i) / (nbColor - 1);
         pPalette[nbColor - i - 1] = RGB( c, c, c );
      }
   }
   else
   {
      for (int i = 0; i < 256; ++i)
      {
         int c = (255 * i) / (nbColor - 1);
         pPalette[i] = RGB( c, c, c );
      }
   }
}



//////////////////////////////////////////////////////////////////////////////
//
// CreateMonochromeDIB() - Create a monochrome (or palettized) display
//
//////////////////////////////////////////////////////////////////////////////

static void CreateMonochromeDIB()
{
   bool bWillUseBuffer = (l_orientation != ORIENTATION_UP) || (l_config.bpp == 2);

   // If we are not going to use an intermediate buffer,
   // we have to take padding into account at this point.
   int width  = l_config.width;
   int height = l_config.height;

   if ( !bWillUseBuffer )
   {
      width = abs( l_config.pitchY ) * 8 / l_config.bpp;
   }

   // Create the DIB
   BYTE buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];

   // Handy pointers
   BITMAPINFO*       pBMI    = (BITMAPINFO*) buffer;
   BITMAPINFOHEADER* pHeader = &pBMI->bmiHeader;
   DWORD*            pColors = (DWORD*)&pBMI->bmiColors;   

   // DIB Header
   pHeader->biSize            = sizeof(BITMAPINFOHEADER);
   pHeader->biWidth           = width;
   pHeader->biHeight          = -height;
   pHeader->biPlanes          = 1;
   pHeader->biBitCount        = (WORD)l_config.bpp;
   pHeader->biCompression     = BI_RGB;
   pHeader->biSizeImage       = (width * height * l_config.bpp) / 8;
   pHeader->biXPelsPerMeter   = 0;
   pHeader->biYPelsPerMeter   = 0;
   pHeader->biClrUsed         = 0;
   pHeader->biClrImportant    = 0;

   // For left oriented displays, we must flip the bitmap upside down
   if (l_orientation == ORIENTATION_LEFT)
   {
      pHeader->biHeight *= -1;
   }

   // If the mode is not supported natively by desktop Windows, we will use
   // an intermediate buffer. Because of this, we define the screen as a simple
   // 8-bit palettized display for speed efficiency.
   if ( bWillUseBuffer )
   {
      pHeader->biBitCount  = 8;
      pHeader->biSizeImage = width * height;
   }
   
   // Set up the palette
   if (l_config.format & kfPalette)
   {
      SetupColorPalette( pColors );
   }
   else
   {
      int nbColor = 1 << pHeader->biBitCount;
      bool bInverted = (l_config.format & kfDirectInverted) != 0;
      SetupMonochromePalette( nbColor, pColors, bInverted );
   }

   // Create the DIB
   l_hBitmap    = CreateDIBSection( 0, pBMI, DIB_RGB_COLORS, (void**)&l_pBitmapBits, 0, 0 );
   l_pVideoBits = l_pBitmapBits;

   // Create intermediate buffer if needed. We check for the landscape flag
   // because we don't want to allocate two intermediate buffers.
   if ( l_config.bpp==2 && l_orientation == ORIENTATION_UP )
   {
      // 2 bpp monochrome (not supported by desktop Windows)
      int width  = l_config.width;
      int height = abs( l_config.pitchX ) * 8 / l_config.bpp;
      int size   = (width * height * l_config.bpp) / 8;
      
      l_pBufferBits = new BYTE[ size ];
      memset( l_pBufferBits, 0, size );

      l_pVideoBits = l_pBufferBits;
   }
}



//////////////////////////////////////////////////////////////////////////////
//
// All the wonderfull blit functions
//
//////////////////////////////////////////////////////////////////////////////

static void BlitLandscape1()
{
   int width      = l_config.width;
   int height     = l_config.height;
   int cellWidth  = l_config.width / 8;
   int cellHeight = l_config.height / 8;

   for (int h = 0; h < cellHeight; ++h)
   {
      BYTE* pSrc[8];
      pSrc[0] = l_pBufferBits + h;
      pSrc[1] = pSrc[0] + cellHeight;
      pSrc[2] = pSrc[1] + cellHeight;
      pSrc[3] = pSrc[2] + cellHeight;
      pSrc[4] = pSrc[3] + cellHeight;
      pSrc[5] = pSrc[4] + cellHeight;
      pSrc[6] = pSrc[5] + cellHeight;
      pSrc[7] = pSrc[6] + cellHeight;

      BYTE* pDest[8];
      pDest[0] = l_pBitmapBits + h * width * 8;
      pDest[1] = pDest[0] + width;
      pDest[2] = pDest[1] + width;
      pDest[3] = pDest[2] + width;
      pDest[4] = pDest[3] + width;
      pDest[5] = pDest[4] + width;
      pDest[6] = pDest[5] + width;
      pDest[7] = pDest[6] + width;

      for (int w = 0; w < cellWidth; ++w)
      {
         int mask = 0x80;

         for (int i = 0; i < 8; ++i)
         {
            *pDest[i]++ = (pSrc[0][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[1][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[2][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[3][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[4][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[5][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[6][w*height] & mask) ? 0xFF : 0x00;
            *pDest[i]++ = (pSrc[7][w*height] & mask) ? 0xFF : 0x00;

            mask >>= 1;
         }
      }
   }
}



static void BlitLandscape2()
{
   int width      = l_config.width;
   int height     = l_config.height;
   int cellWidth  = l_config.width / 4;
   int cellHeight = l_config.height / 4;

   for (int h = 0; h < cellHeight; ++h)
   {
      BYTE* pSrc0 = l_pBufferBits + h;
      BYTE* pSrc1 = pSrc0 + cellHeight;
      BYTE* pSrc2 = pSrc1 + cellHeight;
      BYTE* pSrc3 = pSrc2 + cellHeight;

      BYTE* pDest0 = l_pBitmapBits + h * width * 4;
      BYTE* pDest1 = pDest0 + width;
      BYTE* pDest2 = pDest1 + width;
      BYTE* pDest3 = pDest2 + width;

      for (int w = 0; w < cellWidth; ++w)
      {
         BYTE b0 = pSrc0[ w * height ];
         BYTE b1 = pSrc1[ w * height ];
         BYTE b2 = pSrc2[ w * height ];
         BYTE b3 = pSrc3[ w * height ];

         *pDest0++ = l_table2bpp[b0 >> 6];
         *pDest0++ = l_table2bpp[b1 >> 6];
         *pDest0++ = l_table2bpp[b2 >> 6];
         *pDest0++ = l_table2bpp[b3 >> 6];
         *pDest1++ = l_table2bpp[(b0 >> 4) & 0x03];
         *pDest1++ = l_table2bpp[(b1 >> 4) & 0x03];
         *pDest1++ = l_table2bpp[(b2 >> 4) & 0x03];
         *pDest1++ = l_table2bpp[(b3 >> 4) & 0x03];
         *pDest2++ = l_table2bpp[(b0 >> 2) & 0x03];
         *pDest2++ = l_table2bpp[(b1 >> 2) & 0x03];
         *pDest2++ = l_table2bpp[(b2 >> 2) & 0x03];
         *pDest2++ = l_table2bpp[(b3 >> 2) & 0x03];
         *pDest3++ = l_table2bpp[b0 & 0x03];
         *pDest3++ = l_table2bpp[b1 & 0x03];
         *pDest3++ = l_table2bpp[b2 & 0x03];
         *pDest3++ = l_table2bpp[b3 & 0x03];
      }
   }
}



static void BlitLandscape4()
{
   int endVideo   = l_config.width * l_config.height / 2;
   int cellHeight = l_config.height / 2;

   for (int h = 0; h < cellHeight; ++h)
   {
      BYTE* pSrc0 = l_pBufferBits + h;
      BYTE* pSrc1 = pSrc0 + cellHeight;

      BYTE* pDest0 = l_pBitmapBits + h * l_config.width * 2;
      BYTE* pDest1 = pDest0 + l_config.width;

      for (int offset = 0; offset < endVideo; offset += l_config.height)
      {
         BYTE b0 = pSrc0[ offset ];
         BYTE b1 = pSrc1[ offset ];

         *pDest0++ = l_table4bpp[b0 >> 4];
         *pDest0++ = l_table4bpp[b1 >> 4];
         *pDest1++ = l_table4bpp[b0 & 0x0F];
         *pDest1++ = l_table4bpp[b1 & 0x0F];
      }
   }
}



static void BlitLandscape8()
{
   int width  = l_config.width;
   int height = l_config.height;
   int end    = width * height;

   for (int h = 0; h < height; ++h)
   {
      BYTE* pSrc  = l_pBufferBits + h;
      BYTE* pDest = l_pBitmapBits + h * width;

      for (int w = 0; w < end; w += height)
      {
         *pDest++ = pSrc[w];
      }
   }
}



static void BlitLandscape16()
{
   int width  = l_config.width;
   int height = l_config.height;
   int end    = width * height;

   for (int h = 0; h < height; ++h)
   {
      WORD* pSrc  = (WORD*)l_pBufferBits + h;
      WORD* pDest = (WORD*)l_pBitmapBits + h * width;

      for (int w = 0; w < end; w += height)
      {
         *pDest++ = pSrc[w];
      }
   }
}



static void BlitLandscape32()
{
   int width  = l_config.width;
   int height = l_config.height;
   int end    = width * height;

   for (int h = 0; h < height; ++h)
   {
      UINT32* pSrc  = (UINT32*)l_pBufferBits + h;
      UINT32* pDest = (UINT32*)l_pBitmapBits + h * width;

      for (int w = 0; w < end; w += height)
      {
         *pDest++ = pSrc[w];
      }
   }
}


static void BlitMonochrome2()
{
   BYTE* pSrc    = l_pVideoBits;
   BYTE* pDest   = l_pBitmapBits;
   BYTE* pEndSrc = pSrc + l_config.width * l_config.height / 4;

   for ( ; pSrc < pEndSrc; ++pSrc )
   {
      BYTE b = *pSrc;

      *pDest++ = l_table2bpp[b >> 6];
      *pDest++ = l_table2bpp[(b >> 4) & 0x03];
      *pDest++ = l_table2bpp[(b >> 2) & 0x03];
      *pDest++ = l_table2bpp[b & 0x03];
   }
}



//////////////////////////////////////////////////////////////////////////////
//
// GAPI Functions
//
//////////////////////////////////////////////////////////////////////////////

int GXOpenDisplay( HWND hwnd, DWORD dwFlags )
{
   // The given window handle is validated. Most (if not all)
   // GAPI implementation don't care about this, but we do.
   // If your application crash, make sure you are providing
   // a valid window handle here.
   if (!::IsWindow(hwnd))
      return FALSE;

   // Check if we are already initialized
   if (::IsWindow(l_hwnd))
      return FALSE;

   // According to feedback I've got, the GX_FULLSCREEN flag
   // simply reset the viewport to the full screen...
   if (dwFlags & GX_FULLSCREEN)
      GXSetViewport( 0, l_config.height, 0, 0 );

   // Find out screen orientation
   l_orientation = ORIENTATION_UP;

   if (l_config.format & kfLandscape)
   {
      if (l_config.pitchX > 0)
         l_orientation = ORIENTATION_LEFT;
      else
         l_orientation = ORIENTATION_RIGHT;
   }

   // Initialize the bitmap that will simulate the display
   if ( l_config.format & (kfDirect555 | kfDirect565 | kfDirect888 | kfDirect444) )
      CreateColorDIB();
   else
      CreateMonochromeDIB();

   // Allocate an intermediate buffer for landscape displays
   if (l_orientation != ORIENTATION_UP)
   {
      // Size takes pitch into account
      int width  = l_config.width;
      int height = abs( l_config.pitchX ) * 8 / l_config.bpp;
      int size   = (width * height * l_config.bpp) / 8;
      
      l_pBufferBits =  new BYTE[ size ];
      memset( l_pBufferBits, 0, size );

      l_pVideoBits = l_pBufferBits;
   }

   // Adjust the video bits pointer for rotated screens
   if ( (l_orientation != ORIENTATION_UP) )
   {
      if (l_orientation == ORIENTATION_LEFT)
      {
         l_pVideoBits = l_pVideoBits + (l_config.height * l_config.bpp) / 8 - (l_config.bpp + 7) / 8;
      }
      else
      {
         l_pVideoBits = l_pVideoBits - (l_config.width - 1) * l_config.pitchX;
      }
   }

   // Get ready to rock
   l_bSuspended = false;
   l_hwnd       = hwnd;

   // Hide the taskbar (most devices seems to do this)
   ::SHFullScreen( hwnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON );

   return TRUE;
}



int GXCloseDisplay()
{
   // Check if we are actually initialized
   if (!l_hwnd)
      return FALSE;

   // Free temporary buffer if it was used
   if (l_pBufferBits)
   {
      delete [] l_pBufferBits;
      l_pBufferBits = 0;
   }

   // Free the bitmap
   ::DeleteObject( l_hBitmap );
   l_pBitmapBits = 0;
   l_pVideoBits  = 0;
   
   // Reset values
   l_hwnd = 0;

   return TRUE;
}



void* GXBeginDraw()
{
   // If we are suspended, return 0
   if (l_bSuspended)
      return 0;

   return l_pVideoBits;
}



int GXEndDraw()
{
   // Is the display initialized?
   if (!::IsWindow(l_hwnd))
      return FALSE;
 
   // If there is a temporary buffer, copy it to the bitmap
   if (l_pBufferBits)
   {
      if ( l_orientation != ORIENTATION_UP )
      {
         switch (l_config.bpp)
         {
         case 1:  BlitLandscape1(); break;
         case 2:  BlitLandscape2(); break;
         case 4:  BlitLandscape4(); break;
         case 8:  BlitLandscape8(); break;
         case 16: BlitLandscape16(); break;
         case 32: BlitLandscape32(); break;
        }
      }
      else
      {
         // This is the 2bpp monochrome mode
         BlitMonochrome2();
      }
   }
   
   // Blit the bitmap to the display
   HDC hDestDC = ::GetDC( l_hwnd );
   HDC hSrcDC  = ::CreateCompatibleDC( hDestDC );

   ::SelectObject( hSrcDC, l_hBitmap );
   
#ifndef _WIN32_WCE
   // Warning: rotation is only available on Windows NT
   bool bSuccess = false;

   RECT r;
   ::GetClientRect( l_hwnd, &r );
   if (r.right - r.left == l_config.height &&
       r.bottom - r.top == l_config.width)
   {
      // Rotate the bitmap
      XFORM xform;
      xform.eM11 = 0;
      xform.eM12 = 1;
      xform.eM21 = -1;
      xform.eM22 = 0;
      xform.eDx = (float)l_config.height;
      xform.eDy = 0;

      if (l_orientation == ORIENTATION_RIGHT)
      {
         xform.eM21 = 1;
         xform.eDx  = 0;
      }

      if (::SetGraphicsMode( hDestDC, GM_ADVANCED ) && 
          ::SetWorldTransform( hDestDC, &xform ) &&
          ::BitBlt( hDestDC, 0, 0, l_config.width, l_config.height, hSrcDC, 0, 0, SRCCOPY ))
      {
         bSuccess = true;
      }
   }

   // If rotation didn't work (not on Windows NT for example), do normal blitting...
   if (!bSuccess)
#endif
   // Special case for right orientation: we must mirror the display
   if ( l_orientation == ORIENTATION_RIGHT )
   {
      // We use StretchBlt() to mirror the display
      ::StretchBlt( hDestDC, 0, 0, l_config.width, l_config.height, 
                    hSrcDC, l_config.width - 1, 0, -l_config.width, l_config.height, SRCCOPY );
   }
   else
   {
      // A simple BitBlt() will do
      ::BitBlt( hDestDC, 0, l_viewportTop, l_config.width, l_viewportHeight,
                hSrcDC, 0, l_viewportTop, SRCCOPY );
   }

   ::DeleteDC( hSrcDC );
   ::ReleaseDC( l_hwnd, hDestDC );

   return TRUE;
}



GXDisplayProperties GXGetDisplayProperties()
{
   GXDisplayProperties properties;
   
   properties.cxWidth  = l_config.width;
   properties.cyHeight = l_config.height;
   properties.cbxPitch = l_config.pitchX;
   properties.cbyPitch = l_config.pitchY;
   properties.cBPP     = l_config.bpp;
   properties.ffFormat = l_config.format;

   return properties;
}



int GXSetViewport( DWORD top, DWORD height, DWORD dwReserved1, DWORD dwReserved2 )
{
   // Validate parameters
   if (top >= (unsigned)l_config.height)
      return FALSE;

   if (height == 0 || height > (unsigned)l_config.height)
      return FALSE;

   // Clip viewport to screen
   if (top + height > (unsigned)l_config.height)
      height = l_config.height - top;

   // Set viewport
   l_viewportTop    = top;
   l_viewportHeight = height;
   
   return TRUE;
}



BOOL GXIsDisplayDRAMBuffer()
{
   return l_config.bDRAMBuffer;
}



GXKeyList GXGetDefaultKeys( int iOptions )
{
   GXKeyList keys;
   
   memset( &keys, 0, sizeof(keys) );

   switch( iOptions )
   {
   case GX_LANDSCAPEKEYS:
      keys.vkUp    = VK_LEFT;
      keys.vkDown  = VK_RIGHT;
      keys.vkLeft  = VK_DOWN;
      keys.vkRight = VK_UP;
      break;
   
   case GX_NORMALKEYS:
   default:
      keys.vkUp    = VK_UP;
      keys.vkDown  = VK_DOWN;
      keys.vkLeft  = VK_LEFT;
      keys.vkRight = VK_RIGHT;
   }
   
   keys.vkA     = '1';
   keys.vkB     = '2';
   keys.vkC     = '3';
   keys.vkStart = '\r';

   // Key positions. These values are from the Casio E-125.
   keys.ptUp.x    = 25;    keys.ptUp.y    = 383;
   keys.ptDown.x  = 25;    keys.ptDown.y  = 442;
   keys.ptLeft.x  = -4;    keys.ptLeft.y  = 417;
   keys.ptRight.x = 58;    keys.ptRight.y = 417;
   keys.ptA.x     = 182;   keys.ptA.y     = 442;
   keys.ptB.x     = 240;   keys.ptB.y     = 417;
   keys.ptC.x     = 199;   keys.ptC.y     = 396;
   keys.ptStart.x = -50;   keys.ptStart.y = 199;

   return keys;
}



int GXSuspend()
{
   if (!l_bSuspended)
   {
      // Clear the display, take pitch into account
      int size;

      if (l_orientation != ORIENTATION_UP)
         size = abs( l_config.pitchX ) * l_config.width;
      else
         size = abs( l_config.pitchY ) * l_config.height;
      
      if (l_pBufferBits)
         memset( l_pBufferBits, 0, size );
      else if (l_pBitmapBits)
         memset( l_pBitmapBits, 0, size );

      l_bSuspended = true;
   }

   return TRUE;
}



int GXResume()
{
   l_bSuspended = false;
   return TRUE;
}


int GXOpenInput()
{
   return TRUE;
}



int GXCloseInput()
{
   return TRUE;
}
