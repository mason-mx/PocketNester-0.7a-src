// DecmpImage.h: interface for the DecmpImage class.
//
// (c)Copyright 2003. Rick Lei
// $Id$

#if !defined(AFX_DECMPIMAGE_H__CB1E39FC_AB00_4A2A_B911_6187E4CEDE90__INCLUDED_)
#define AFX_DECMPIMAGE_H__CB1E39FC_AB00_4A2A_B911_6187E4CEDE90__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "imgdecmp.h"
#include "imgrendr.h"

class DecmpImage  
{
public:
	DecmpImage(BYTE * data, DWORD size, HDC hdc);
	// add more constructors if you like :)

	virtual ~DecmpImage();

	BYTE * GetBits()
	{
		return _bits;
	}

	int GetHeight() const
	{
		return _height;
	}

	int GetWidth() const
	{
		return _width;
	}

	HBITMAP GetBitmap()
	{
		return _bmp;
	}

private:
	static DWORD CALLBACK GetImageData(LPSTR buffer, DWORD bufferMax, LPARAM lParam);
	static void CALLBACK ImageProgress(IImageRender *pRender, BOOL bComplete, LPARAM lParam);

	BYTE * _bits;
	int _height;
	int _width;
	HBITMAP _bmp;

	BYTE * _inputData;
	DWORD _inputDataSize;
	DWORD _pos;
};

#endif // !defined(AFX_DECMPIMAGE_H__CB1E39FC_AB00_4A2A_B911_6187E4CEDE90__INCLUDED_)
