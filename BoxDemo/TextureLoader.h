#pragma once

#include <wincodec.h>

#include <d3d12.h>

class TextureLoader
{
public:
	TextureLoader(const LPCWSTR& Path);

	UINT GetWidth() const { return TextureWidth; }
	UINT GetHeight() const { return TextureHeight; }
	DXGI_FORMAT GetFormat() const { return Format; }
	BYTE* Pointer() const { return ImageData; }
	SIZE_T Size() const { return ImageSize; }
	SIZE_T GetRowPitch() const { return BytesPerRow; }

private:
	int ImageSize;
	int BytesPerRow;
	LPCWSTR Filename;
	UINT TextureWidth;
	UINT TextureHeight;
	DXGI_FORMAT Format;
	BYTE* ImageData;

	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);
	int LoadImageDataFromFile();
};

