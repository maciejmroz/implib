/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


/*
 *	Implementation file for MMX optimized operations
 */

#include "stdafx.h"
#include "IMPLibSw.h"
#include "IMPSSE2Kernel.h"

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

EError NIMPLibSwInternal::addInt32SSE2(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	BYTE *pSrc1=layer2.byte_data;
	BYTE *pSrc2=layer3.byte_data;
	BYTE *pDest=layer1.byte_data;
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+4*EDX-4]
		mov ecx,DWORD PTR [esi+4*EDX-4]
		shr edx,5			//16 pixels/loop
		mov ecx,pDest
	LoopAddI32:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusb xmm0,XMMWORD PTR [esi]
		paddusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusb xmm0,XMMWORD PTR [esi]
		paddusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusb xmm0,XMMWORD PTR [esi]
		paddusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusb xmm0,XMMWORD PTR [esi]
		paddusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//loop tail
		dec edx
		jnz LoopAddI32
	}
	return eeOk;
}

EError NIMPLibSwInternal::addInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc1=layer2.ushort_data;
	unsigned short *pSrc2=layer3.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		mov ecx,DWORD PTR [esi+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
	LoopAddI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusw xmm0,XMMWORD PTR [esi]
		paddusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusw xmm0,XMMWORD PTR [esi]
		paddusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusw xmm0,XMMWORD PTR [esi]
		paddusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		paddusw xmm0,XMMWORD PTR [esi]
		paddusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//loop tail
		dec edx
		jnz LoopAddI64
	}
	return eeOk;
}

/*
 * add color
 */

EError NIMPLibSwInternal::addColorInt32SSE2(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) BYTE clamped_col[16];
	convertColorToInt32(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4);
	memcpy(&clamped_col[8],clamped_col,8);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	BYTE *pSrc=layer2.byte_data;
	BYTE *pDest=layer1.byte_data;
	//process 128 bytes (32 pixels!) in single iteration
	//(P4 prefetch loads 128 bytes)
	__asm {
		mov eax,pSrc
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+4*EDX-4]
		mov ecx,DWORD PTR [esi+4*EDX-4]
		shr edx,5			//32 pixels/loop
		mov ecx,pDest
		movdqa xmm7,clamped_col
	LoopAddcI32:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusb xmm0,xmm7
		paddusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusb xmm0,xmm7
		paddusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusb xmm0,xmm7
		paddusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusb xmm0,xmm7
		paddusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//loop tail
		dec edx
		jnz LoopAddcI32
	}
	return eeOk;
}

EError NIMPLibSwInternal::addColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) unsigned short clamped_col[8];
	convertColorToInt64(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4*sizeof(unsigned short));
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc=layer2.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	//process 128 bytes (16 pixels!) in single iteration
	//(P4 prefetch loads 128 bytes)
	__asm {
		mov eax,pSrc
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		mov ecx,DWORD PTR [esi+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
		movdqa xmm7,clamped_col
	LoopAddcI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusw xmm0,xmm7
		paddusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusw xmm0,xmm7
		paddusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusw xmm0,xmm7
		paddusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		paddusw xmm0,xmm7
		paddusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//loop tail
		dec edx
		jnz LoopAddcI64
	}
	return eeOk;
}

/*
 *	sub
 */

EError NIMPLibSwInternal::subInt32SSE2(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	BYTE *pSrc1=layer2.byte_data;
	BYTE *pSrc2=layer3.byte_data;
	BYTE *pDest=layer1.byte_data;
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+4*EDX-4]
		mov ecx,DWORD PTR [esi+4*EDX-4]
		shr edx,5			//32 pixels/loop
		mov ecx,pDest
	LoopSubI32:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusb xmm0,XMMWORD PTR [esi]
		psubusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusb xmm0,XMMWORD PTR [esi]
		psubusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusb xmm0,XMMWORD PTR [esi]
		psubusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusb xmm0,XMMWORD PTR [esi]
		psubusb xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//loop tail
		dec edx
		jnz LoopSubI32
	}
	return eeOk;
}

EError NIMPLibSwInternal::subInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc1=layer2.ushort_data;
	unsigned short *pSrc2=layer3.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		mov ecx,DWORD PTR [esi+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
	LoopSubI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusw xmm0,XMMWORD PTR [esi]
		psubusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusw xmm0,XMMWORD PTR [esi]
		psubusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusw xmm0,XMMWORD PTR [esi]
		psubusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		psubusw xmm0,XMMWORD PTR [esi]
		psubusw xmm2,XMMWORD PTR [esi+16]
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//loop tail
		dec edx
		jnz LoopSubI64
	}
	return eeOk;
}

/*
 * sub color
 */

EError NIMPLibSwInternal::subColorInt32SSE2(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) BYTE clamped_col[16];
	convertColorToInt32(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4);
	memcpy(&clamped_col[8],clamped_col,8);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	BYTE *pSrc=layer2.byte_data;
	BYTE *pDest=layer1.byte_data;
	//process 128 bytes (32 pixels!) in single iteration
	//(P4 prefetch loads 128 bytes)
	__asm {
		mov eax,pSrc
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+4*EDX-4]
		mov ecx,DWORD PTR [esi+4*EDX-4]
		shr edx,5			//32 pixels/loop
		mov ecx,pDest
		movdqa xmm7,clamped_col
	LoopSubcI32:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusb xmm0,xmm7
		psubusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusb xmm0,xmm7
		psubusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusb xmm0,xmm7
		psubusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusb xmm0,xmm7
		psubusb xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//loop tail
		dec edx
		jnz LoopSubcI32
	}
	return eeOk;
}

EError NIMPLibSwInternal::subColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) unsigned short clamped_col[8];
	convertColorToInt64(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4*sizeof(unsigned short));
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc=layer2.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	//process 128 bytes (16 pixels!) in single iteration
	//(P4 prefetch loads 128 bytes)
	__asm {
		mov eax,pSrc
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		mov ecx,DWORD PTR [esi+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
		movdqa xmm7,clamped_col
	LoopSubcI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusw xmm0,xmm7
		psubusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusw xmm0,xmm7
		psubusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusw xmm0,xmm7
		psubusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		psubusw xmm0,xmm7
		psubusw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//loop tail
		dec edx
		jnz LoopSubcI64
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3) {
	if(!equalL3(layer1,layer2,layer3)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc1=layer2.ushort_data;
	unsigned short *pSrc2=layer3.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc1
		mov esi,pSrc2
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [esi]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		mov ecx,DWORD PTR [esi+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
	LoopMulI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		prefetchnta [esi]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [esi]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		movdqa xmm3,XMMWORD PTR [esi+16]
		pmulhuw xmm0,xmm1
		pmulhuw xmm2,xmm3
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [esi]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		movdqa xmm3,XMMWORD PTR [esi+16]
		pmulhuw xmm0,xmm1
		pmulhuw xmm2,xmm3
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [esi]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		movdqa xmm3,XMMWORD PTR [esi+16]
		pmulhuw xmm0,xmm1
		pmulhuw xmm2,xmm3
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [esi]
		movdqa xmm2,XMMWORD PTR [EAX+16]
		movdqa xmm3,XMMWORD PTR [esi+16]
		pmulhuw xmm0,xmm1
		pmulhuw xmm2,xmm3
		add eax,32
		add esi,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm2
		add ecx,32
		//loop tail
		dec edx
		jnz LoopMulI64
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2)) {
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit) {
		return eeUnsupportedFormat;
	}
	__declspec(align(16)) unsigned short clamped_col[8];
	convertColorToInt64(color,clamped_col);
	memcpy(&clamped_col[4],clamped_col,4*sizeof(unsigned short));
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned short *pSrc=layer2.ushort_data;
	unsigned short *pDest=layer1.ushort_data;
	__asm {
		mov eax,pSrc
		mov edx,nPixels
		//touch sources
		mov ecx,DWORD PTR [EAX]
		mov ecx,DWORD PTR [EAX+8*EDX-4]
		shr edx,4			//16 pixels/loop
		mov ecx,pDest
		movdqa xmm7,clamped_col
	LoopMulcI64:
		//prefetch sources (128 bytes on P4)
		prefetchnta [eax]
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		pmulhuw xmm0,xmm7
		pmulhuw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		pmulhuw xmm0,xmm7
		pmulhuw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		pmulhuw xmm0,xmm7
		pmulhuw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//process 32 bytes
		movdqa xmm0,XMMWORD PTR [EAX]
		movdqa xmm1,XMMWORD PTR [EAX+16]
		pmulhuw xmm0,xmm7
		pmulhuw xmm1,xmm7
		add eax,32
		movdqa XMMWORD PTR [ECX],xmm0
		movdqa XMMWORD PTR [ECX+16],xmm1
		add ecx,32
		//loop tail
		dec edx
		jnz LoopMulcI64
	}
	return eeOk;
}
