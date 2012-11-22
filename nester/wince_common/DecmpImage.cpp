// DecmpImage.cpp: implementation of the DecmpImage class.
//
// (c)Copyright 2003. Rick Lei
// $Id$

#include "DecmpImage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DecmpImage::DecmpImage(BYTE * data, DWORD size, HDC hdc) :
	_bmp(NULL), _bits(NULL), _height(0), _width(0), _inputData(data), _inputDataSize(size), _pos(0)
{
	DecompressImageInfo	dii;
	BYTE buf[2048];
	
	dii.dwSize = sizeof(DecompressImageInfo);
	dii.pbBuffer = buf;
	dii.dwBufferMax = sizeof(buf);
	dii.dwBufferCurrent = 0;
	dii.phBM = NULL;
	dii.ppImageRender = NULL;
	dii.iBitDepth = 24;
	dii.lParam = (LPARAM)this;
	dii.hdc = hdc;
	dii.iScale = 100;
	dii.iMaxWidth = 1024;
	dii.iMaxHeight = 1024;
	dii.pfnGetData = DecmpImage::GetImageData;
	dii.pfnImageProgress = DecmpImage::ImageProgress;
	dii.crTransparentOverride = ( UINT ) -1;
	
	DecompressImageIndirect( &dii );
}

DecmpImage::~DecmpImage()
{
	if (_bmp != NULL)
		DeleteObject(_bmp);
}

DWORD CALLBACK DecmpImage::GetImageData(LPSTR buffer, DWORD bufferMax, LPARAM lParam)
{
	DecmpImage * image = (DecmpImage *)lParam;
	DWORD size = bufferMax;
	if (size > image->_inputDataSize - image->_pos)
		size = image->_inputDataSize - image->_pos;

	memcpy(buffer, image->_inputData + image->_pos, size);
	image->_pos += size;
	
	return size;
}

void CALLBACK DecmpImage::ImageProgress(IImageRender *pRender, BOOL bComplete, LPARAM lParam)
{
	DecmpImage * image = (DecmpImage *)lParam;
	
	if( bComplete )
	{
		pRender->GetBitmap(&image->_bmp, TRUE);
		pRender->GetOrigHeight(&image->_height);
		pRender->GetOrigWidth(&image->_width);
		pRender->GetBits(&image->_bits);
	}
}
