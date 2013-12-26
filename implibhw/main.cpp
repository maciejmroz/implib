/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 *
 */



// IMPLibSw - implementacja referencyjna na CPU
//

#include "stdafx.h"
#include <assert.h>
#include <mmsystem.h>

using namespace NIMPLib;

#include "../Common/IMPLibTests.cpp"

#include <boost/bind.hpp>

#ifdef TEST
#ifdef TEST64
	#define INFILE L"lena64_1024.png"
	#ifdef TEST1
		#define OUTFILE L"test1_64.png"
	#endif
	#ifdef TEST2
		#define OUTFILE L"test2_64.png"
	#endif
	#ifdef TEST3
		#define OUTFILE L"test3_64.png"
	#endif
	#ifdef TEST4
		#define OUTFILE L"test4_64.png"
	#endif
	#ifdef TEST5
		#define OUTFILE L"test5_64.png"
	#endif
	#ifdef TEST6
		#define OUTFILE L"test6_64.png"
	#endif
	#ifdef TEST7
		#define OUTFILE L"test7_64.png"
	#endif
	#ifdef TEST8
		#define OUTFILE L"test8_64.png"
	#endif
#else
	#define INFILE L"lena32_1024.png"
	#ifdef TEST1
		#define OUTFILE L"test1_32.png"
	#endif
	#ifdef TEST2
		#define OUTFILE L"test2_32.png"
	#endif
	#ifdef TEST3
		#define OUTFILE L"test3_32.png"
	#endif
	#ifdef TEST4
		#define OUTFILE L"test4_32.png"
	#endif
	#ifdef TEST5
		#define OUTFILE L"test5_32.png"
	#endif
	#ifdef TEST6
		#define OUTFILE L"test6_32.png"
	#endif
	#ifdef TEST7
		#define OUTFILE L"test7_32.png"
	#endif
	#ifdef TEST8
		#define OUTFILE L"test8_32.png"
	#endif
#endif

#endif

PDIRECT3D9			g_pD3D=NULL;
PDIRECT3DDEVICE9	g_pD3DDevice=NULL;

//this is "a bit" too much since we are not going to ever
//use back/z-buffer - yet these settings are likely to
//allow for successful device creation - while 1x1 backbuffer
//is unpredictable
static D3DPRESENT_PARAMETERS	s_PresentParameters=
{
	640,480,
	D3DFMT_X8R8G8B8,
	1,
	D3DMULTISAMPLE_NONE,
	0,
	D3DSWAPEFFECT_DISCARD,
	NULL,
	TRUE,
	TRUE,
	D3DFMT_D24S8,
	0,
	D3DPRESENT_RATE_DEFAULT,
	D3DPRESENT_INTERVAL_ONE
};

int _tmain(int, _TCHAR*[])
{
	timeBeginPeriod(1);

	if(!(g_pD3D=Direct3DCreate9(D3D_SDK_VERSION)))
	{
		_tprintf(_T("Failed to create Direct3D object"));
		return -1;
	}
	s_PresentParameters.hDeviceWindow=GetConsoleWindow();
	//TODO: remember to change REF back to HAL
	if(FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,
		GetConsoleWindow(),D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&s_PresentParameters,&g_pD3DDevice)))
	{
		_tprintf(_T("Failed to create Direct3D device"));
		return -1;
	}
	ExecutionEngine *ee=createHardwareExecutionEngine(g_pD3DDevice);
	assert(ee);

	LayerDesc ld;
#ifdef TEST64
	ld.eFormat=elfInt64Bit;
#else
	ld.eFormat=elfInt32Bit;
#endif
	ld.dwSizeX=1024;
	ld.dwSizeY=1024;

	ee->allocateLayers(ld,4);

	NIMPLibTests::timer tm;

#ifndef TEST
	ee->loadLayerFromPNGFile(1,L"lena64_1024.png",boost::bind(&NIMPLibTests::timer::start,&tm));
	NIMPLibTests::test_multi_distort_bicubic(ee);

	ee->saveLayerToPNGFile(1,L"temp.png",
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#else
#ifdef TEST1
	NIMPLibTests::test_checkers(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST1
#ifdef TEST2
	NIMPLibTests::test_gradient_1(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST2
#ifdef TEST3
	NIMPLibTests::test_gradient_2(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST3
#ifdef TEST4
	NIMPLibTests::basic_noise(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST4
#ifdef TEST5
	NIMPLibTests::basic_noise3(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST5
#ifdef TEST6
	ee->loadLayerFromPNGFile(1,INFILE,boost::bind(&NIMPLibTests::timer::start,&tm));
	NIMPLibTests::test_memory(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST6
#ifdef TEST7
	ee->loadLayerFromPNGFile(1,INFILE,boost::bind(&NIMPLibTests::timer::start,&tm));
	NIMPLibTests::test_multi_distort_linear(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST7
#ifdef TEST8
	ee->loadLayerFromPNGFile(1,INFILE,boost::bind(&NIMPLibTests::timer::start,&tm));
	NIMPLibTests::test_multi_distort_bicubic(ee);

	ee->saveLayerToPNGFile(0,OUTFILE,
		NIMPLib::data_ready_callback_t(boost::bind(&NIMPLibTests::timer::end,&tm)));
#endif	//TEST8
#endif	//TEST

	ee->freeLayers();

	g_pD3DDevice->Release();
	g_pD3DDevice=NULL;
	g_pD3D->Release();
	g_pD3D=NULL;

	_tprintf(_T("duration %u\n"),tm.duration);
	return 0;
}

