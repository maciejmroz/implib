/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


/*
 *	Implementation file for MMX optimized operations
 */

#include "stdafx.h"
#include "IMPLibSw.h"
#include "IMPMMXKernel.h"

using namespace NIMPLib;
using namespace NIMPLibSwInternal;

static bool equalL2(Layer &layer1,Layer &layer2) {
	return (layer1.desc==layer2.desc);
}

static bool equalL3(Layer &layer1,Layer &layer2,Layer &layer3) {
	return ((layer1.desc==layer2.desc) &&
			(layer2.desc==layer3.desc));
}

static void clampColor(float in[4],float out[4]) {
	int i=0;
	for(;i<4;i++) {
		if(in[i]<0.0f) {
			out[i]=0.0f;
		}
		else if(in[i]>1.0f) {
			out[i]=1.0f;
		}
		else {
			out[i]=in[i];
		}
	}
}

static void convertColorToInt32(float in[4],BYTE out[4]) {
	float temp[4];
	clampColor(in,temp);
	int i=0;
	for(;i<4;i++) {
		out[i]=(BYTE)(in[i]*255.0f);
	}
}

static void convertColorToInt64(float in[4],unsigned short out[4]) {
	float temp[4];
	clampColor(in,temp);
	int i=0;
	for(;i<4;i++) {
		out[i]=(unsigned short)(in[i]*65535.0f);
	}
}

/*
 *	add
 */

EError NIMPLibSwInternal::addInt32MMX(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nDPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY/2;
	BYTE *pSrc1=layer2.byte_data;
	BYTE *pSrc2=layer3.byte_data;
	BYTE *pDest=layer1.byte_data;
	//process two full pixels in every iteration
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov ecx,pDest
		mov edx,nDPixels
	LoopAddI32:
		//load sources
		movq mm0,QWORD PTR [EAX]
		add eax,8
		paddusb mm0,QWORD PTR [esi]
		add esi,8
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopAddI32
		emms
	}
	return eeOk;
}

EError NIMPLibSwInternal::addInt64MMX(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc1=layer2.ushort_data;
	unsigned short *pSrc2=layer3.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	//process one full pixel in every iteration
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov ecx,pDest
		mov edx,nPixels
	LoopAddI64:
		//load sources
		movq mm0,QWORD PTR [EAX]
		add eax,8
		paddusw mm0,QWORD PTR [esi]
		add esi,8
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopAddI64
		emms
	}
	return eeOk;
}

/*
 * add color
 */

EError NIMPLibSwInternal::addColorInt32MMX(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) BYTE clamped_col[8];
	convertColorToInt32(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4);
	unsigned int nDPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY/2;
	BYTE *pSrc=layer2.byte_data;
	BYTE *pDest=layer1.byte_data;
	__asm {
		mov eax,pSrc
		mov ecx,pDest
		mov edx,nDPixels
		movq mm7,clamped_col
	LoopAddcI32:
		movq mm0,QWORD PTR [EAX]
		add eax,8
		add esi,8
		paddusb mm0,mm7
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopAddcI32
		emms
	}
	return eeOk;
}

EError NIMPLibSwInternal::addColorInt64MMX(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) unsigned short clamped_col[4];
	convertColorToInt64(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc=layer2.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc
		mov ecx,pDest
		mov edx,nPixels
		movq mm7,clamped_col
	LoopAddcI64:
		movq mm0,QWORD PTR [EAX]
		add eax,8
		add esi,8
		paddusw mm0,mm7
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopAddcI64
		emms
	}
	return eeOk;
}

/*
 *	sub
 */

EError NIMPLibSwInternal::subInt32MMX(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nDPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY/2;
	BYTE *pSrc1=layer2.byte_data;
	BYTE *pSrc2=layer3.byte_data;
	BYTE *pDest=layer1.byte_data;
	//process two full pixels in every iteration
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov ecx,pDest
		mov edx,nDPixels
	LoopSubI32:
		//load sources
		movq mm0,QWORD PTR [EAX]
		add eax,8
		psubusb mm0,QWORD PTR [esi]
		add esi,8
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopSubI32
		emms
	}
	return eeOk;
}

EError NIMPLibSwInternal::subInt64MMX(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc1=layer2.ushort_data;
	unsigned short *pSrc2=layer3.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	//process one full pixel in every iteration
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov ecx,pDest
		mov edx,nPixels
	LoopSubI64:
		//load sources
		movq mm0,QWORD PTR [EAX]
		add eax,8
		psubusw mm0,QWORD PTR [esi]
		add esi,8
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopSubI64
		emms
	}
	return eeOk;
}

/*
 * sub color
 */

EError NIMPLibSwInternal::subColorInt32MMX(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) BYTE clamped_col[8];
	convertColorToInt32(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4);
	unsigned int nDPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY/2;
	BYTE *pSrc=layer2.byte_data;
	BYTE *pDest=layer1.byte_data;
	__asm {
		mov eax,pSrc
		mov ecx,pDest
		mov edx,nDPixels
		movq mm7,clamped_col
	LoopSubcI32:
		movq mm0,QWORD PTR [EAX]
		add eax,8
		add esi,8
		psubusb mm0,mm7
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopSubcI32
		emms
	}
	return eeOk;
}

EError NIMPLibSwInternal::subColorInt64MMX(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) unsigned short clamped_col[4];
	convertColorToInt64(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc=layer2.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc
		mov ecx,pDest
		mov edx,nPixels
		movq mm7,clamped_col
	LoopSubcI64:
		movq mm0,QWORD PTR [EAX]
		add eax,8
		add esi,8
		psubusw mm0,mm7
		movq QWORD PTR [ECX],mm0
		add ecx,8
		dec edx
		jnz LoopSubcI64
		emms
	}
	return eeOk;
}
