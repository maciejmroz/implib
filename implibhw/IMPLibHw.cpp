/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

#include "stdafx.h"
#include "IMPLibHw.h"
#include <malloc.h>
#include <algorithm>
#include <png.h>

using namespace NIMPLib;

ExecutionEngineHw::ExecutionEngineHw() :
device(NULL),
vertex_decl(NULL),
swizzle_buffer(NULL),
swizzle_buffer_size(0),
first_r32f_layer(0)
{
	//v1
	vb[0]=128.0f;
	vb[1]=-0.5f;
	vb[2]=0.5f;
	vb[3]=1.0f;
	vb[4]=1.0f;
	vb[5]=0.0f;
	//v2
	vb[6]=128.0f;
	vb[7]=128.0f;
	vb[8]=0.5f;
	vb[9]=1.0f;
	vb[10]=1.0f;
	vb[11]=1.0f;
	//v3
	vb[12]=-0.5f;
	vb[13]=-0.5f;
	vb[14]=0.5f;
	vb[15]=1.0f;
	vb[16]=0.0f;
	vb[17]=0.0f;
	//v4
	vb[18]=-0.5f;
	vb[19]=128.0f;
	vb[20]=0.5f;
	vb[21]=1.0f;
	vb[22]=0.0f;
	vb[23]=1.0f;
}

ExecutionEngineHw::~ExecutionEngineHw()
{
	assert(swizzle_buffer==NULL);
}

void ExecutionEngineHw::prepare_vb(unsigned int rt_size_x,unsigned int rt_size_y)
{
	float f_size_x=(float)rt_size_x;
	float f_size_y=(float)rt_size_y;
	vb[0]=f_size_x-0.5f;
	vb[6]=f_size_x-0.5f;
	vb[7]=f_size_y-0.5f;
	vb[19]=f_size_y-0.5f;
}

void ExecutionEngineHw::set_render_target(unsigned char ucLayer)
{
	PDIRECT3DSURFACE9 rt=NULL;
	if(FAILED(layers[ucLayer].tx->GetSurfaceLevel(0,&rt)))
	{
		assert(false);
		return;
	}
	device->SetRenderTarget(0,rt);
	rt->Release();
	prepare_vb(layers[ucLayer].desc.dwSizeX,
		layers[ucLayer].desc.dwSizeY);
}

void ExecutionEngineHw::draw()
{
	if(FAILED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vb,24)))
	{
		OutputDebugString(L"D3D Error!\n");
	}
}

void ExecutionEngineHw::set_texture(unsigned char stage,unsigned char ucLayer,bool bFilter)
{
	set_texture(stage,layers[ucLayer].tx,bFilter);
}

void ExecutionEngineHw::set_texture(unsigned char stage,PDIRECT3DTEXTURE9 tx,bool bFilter)
{
	device->SetTexture(stage,tx);
	if(bFilter)
	{
		device->SetSamplerState(stage,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
		device->SetSamplerState(stage,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
	}
	else
	{
		device->SetSamplerState(stage,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
		device->SetSamplerState(stage,D3DSAMP_MINFILTER,D3DTEXF_POINT);
	}
}


static D3DVERTEXELEMENT9 sBasicVF[]={
	{0,0,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITIONT,0},
	{0,16,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},
	D3DDECL_END()
};

EError ExecutionEngineHw::init(PDIRECT3DDEVICE9 dev)
{
	assert(dev);
	device=dev;
	//initialize
	device->CreateVertexDeclaration(sBasicVF,&vertex_decl);
	s_checkers_aa.init(_T("checkers_aa.hlsl"),dev,"ps_2_0");
	s_add.init(_T("add.hlsl"),dev,"ps_2_0");
	s_sub.init(_T("sub.hlsl"),dev,"ps_2_0");
	s_mul.init(_T("mul.hlsl"),dev,"ps_2_0");
	s_signed_add.init(_T("signed_add.hlsl"),dev,"ps_2_0");
	s_add_color.init(_T("add_color.hlsl"),dev,"ps_2_0");
	s_sub_color.init(_T("sub_color.hlsl"),dev,"ps_2_0");
	s_mul_color.init(_T("mul_color.hlsl"),dev,"ps_2_0");
	s_rgradient_c.init(_T("rgradient_c.hlsl"),dev,"ps_2_0");
	s_rgradient_c_cubic.init(_T("rgradient_c_cubic.hlsl"),dev,"ps_2_0");
	s_rgradient_c_sin.init(_T("rgradient_c_sin.hlsl"),dev,"ps_2_0");
	s_rgradient_e.init(_T("rgradient_e.hlsl"),dev,"ps_2_0");
	s_rgradient_e_sin.init(_T("rgradient_e_sin.hlsl"),dev,"ps_2_0");
	s_wave_x.init(_T("wave_x.hlsl"),dev,"ps_2_0");
	s_wave_y.init(_T("wave_y.hlsl"),dev,"ps_2_0");
	s_lwave_x.init(_T("lwave_x.hlsl"),dev,"ps_2_0");
	s_lwave_y.init(_T("lwave_y.hlsl"),dev,"ps_2_0");
	s_wave_x_bicubic.init(_T("wave_x_bicubic.hlsl"),dev,"ps_2_b");
	s_wave_y_bicubic.init(_T("wave_y_bicubic.hlsl"),dev,"ps_2_b");
	s_lwave_x_bicubic.init(_T("lwave_x_bicubic.hlsl"),dev,"ps_2_b");
	s_lwave_y_bicubic.init(_T("lwave_y_bicubic.hlsl"),dev,"ps_2_b");
	s_wave_x_vf.init(_T("wave_x_vf.hlsl"),dev,"ps_2_0");
	s_wave_y_vf.init(_T("wave_y_vf.hlsl"),dev,"ps_2_0");
	s_lwave_x_vf.init(_T("lwave_x_vf.hlsl"),dev,"ps_2_0");
	s_lwave_y_vf.init(_T("lwave_y_vf.hlsl"),dev,"ps_2_0");
	s_vf_distort.init(_T("vf_distort.hlsl"),dev,"ps_2_0");
	s_vf_distort_bicubic.init(_T("vf_distort_bicubic.hlsl"),dev,"ps_2_b");
	s_noise_1_octave.init(_T("noise_octave.hlsl"),dev,"ps_2_b");
	s_noise_2_octaves.init(_T("noise_2_octaves.hlsl"),dev,"ps_2_b");
	s_noise_3_octaves.init(_T("noise_3_octaves.hlsl"),dev,"ps_2_b");
	s_noise_4_octaves.init(_T("noise_4_octaves_vectorized.hlsl"),dev,"ps_2_b");
	s_noise_final_remap.init(_T("noise_final_remap.hlsl"),dev,"ps_2_0");
	s_turbulence_1_octave.init(_T("turbulence_octave.hlsl"),dev,"ps_2_b");
	s_turbulence_2_octaves.init(_T("turbulence_2_octaves.hlsl"),dev,"ps_2_b");
	s_turbulence_3_octaves.init(_T("turbulence_3_octaves.hlsl"),dev,"ps_2_b");
	s_turbulence_4_octaves.init(_T("turbulence_4_octaves.hlsl"),dev,"ps_2_b");
	s_hsv_adjust.init(_T("hsv_adjust.hlsl"),dev,"ps_2_b");
	s_grayscale.init(_T("grayscale.hlsl"),dev,"ps_2_0");
	s_sobel_edge_detect.init(_T("sobel_edge_detect.hlsl"),dev,"ps_2_0");
	//prepare for rendering
	device->BeginScene();
	device->SetVertexDeclaration(vertex_decl);
	device->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE);
	device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
	device->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
	device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
	device->SetDepthStencilSurface(NULL);
	
	return eeOk;
}

bool ExecutionEngineHw::isLayerFormatSupported(ELayerFormat /*format*/)
{
	return false;
}

EError ExecutionEngineHw::setLayerFormat(ELayerFormat /*format*/)
{
	return eeNotImplemented;
}

bool ExecutionEngineHw::layersMustBePow2Sized()
{
	return true;			//may be less restrictive in future
}

struct free_hw_layer_funct
{
	void operator()(NIMPLibHwInternal::Layer &rf_layer)
	{
		if(rf_layer.tx)
		{
			rf_layer.tx->Release();
		}
	}
};

struct free_hw_layer_proxy_funct
{
	void operator()(io_proxy_map_t::value_type &rf_pair)
	{
		NIMPLibHwInternal::LayerProxy &lpx=rf_pair.second;
		if(lpx.proxy_surface)
		{
			lpx.proxy_surface->Release();
		}
	}
};

EError ExecutionEngineHw::getLayerByteSize(LayerDesc &desc,DWORD &dwByteSize)
{
	DWORD dwElemSize=0;
	switch(desc.eFormat)
	{
		case elfInt32Bit:
			dwElemSize=4;
			break;
		case elfInt64Bit:
			dwElemSize=8;
			break;
		case elfFloat128Bit:
			dwElemSize=16;
			break;
		default:
			//unsupported format
			return eeUnsupportedFormat;
			break;
	}
	dwByteSize=dwElemSize*desc.dwSizeX*desc.dwSizeY;
	return eeOk;
}

EError ExecutionEngineHw::allocateLayers(LayerDesc &desc,DWORD dwCount)
{
	if(dwCount==0)
	{
		return eeOk;
	}
	D3DFORMAT fmt=D3DFMT_A8R8G8B8;
	switch(desc.eFormat)
	{
		case elfInt64Bit:
			fmt=D3DFMT_A16B16G16R16;
			break;
		case elfFloat64Bit:
			fmt=D3DFMT_A16B16G16R16F;
			break;
		case elfFloat128Bit:
			fmt=D3DFMT_A32B32G32R32F;
			break;
		case elfFloat32Bit:
			fmt=D3DFMT_R32F;
			break;
	}
	//try to create io proxy first
	if((desc.eFormat!=elfFloat32Bit) && (io_proxy_map.find(desc)==io_proxy_map.end()))
	{
		NIMPLibHwInternal::LayerProxy lp;
		if(FAILED(device->CreateOffscreenPlainSurface(desc.dwSizeX,desc.dwSizeY,
			fmt,D3DPOOL_SYSTEMMEM,&lp.proxy_surface,NULL)))
		{
			return eeOutOfResources;
		}
		io_proxy_map[desc]=lp;
	}
	//check if there's enough memory swizzle buffer
	DWORD dwByteSize=0;
	getLayerByteSize(desc,dwByteSize);
	if(dwByteSize>swizzle_buffer_size)
	{
		delete swizzle_buffer;
		swizzle_buffer=(void*)new unsigned char[dwByteSize];
		swizzle_buffer_size=dwByteSize;
	}
	//create real surface
	std::vector<NIMPLibHwInternal::Layer> temp_layer_container;
	NIMPLibHwInternal::Layer temp_layer;
	EError retcode=eeOk;
	bool rollback=false;
	DWORD i=0;
	for(;i<dwCount;i++)
	{
		temp_layer.desc=desc;
		if(FAILED(device->CreateTexture(desc.dwSizeX,desc.dwSizeY,
			1,D3DUSAGE_RENDERTARGET,
			fmt,
			D3DPOOL_DEFAULT,&temp_layer.tx,NULL)))
		{
			rollback=true;
			retcode=eeOutOfResources;
			break;
		}
		else
		{
			temp_layer_container.push_back(temp_layer);
		}
	}
	if(rollback)
	{
		for_each(temp_layer_container.begin(),temp_layer_container.end(),
			free_hw_layer_funct());
		temp_layer_container.clear();
		return retcode;
	}
	layers.insert(layers.end(),
		temp_layer_container.begin(),temp_layer_container.end());
	return retcode;
}

void ExecutionEngineHw::freeLayers()
{
	for_each(layers.begin(),layers.end(),free_hw_layer_funct());
	layers.clear();
	for_each(io_proxy_map.begin(),io_proxy_map.end(),free_hw_layer_proxy_funct());
	io_proxy_map.clear();
	delete swizzle_buffer;
	swizzle_buffer=NULL;
	swizzle_buffer_size=NULL;
	vertex_decl->Release();
	vertex_decl=NULL;
	s_checkers_aa.release();
	s_add.release();
	s_sub.release();
	s_mul.release();
	s_signed_add.release();
	s_add_color.release();
	s_sub_color.release();
	s_mul_color.release();
	s_rgradient_c.release();
	s_rgradient_c_cubic.release();
	s_rgradient_c_sin.release();
	s_rgradient_e.release();
	s_rgradient_e_sin.release();
	s_wave_x.release();
	s_wave_y.release();
	s_lwave_x.release();
	s_lwave_y.release();
	s_wave_x_bicubic.release();
	s_wave_y_bicubic.release();
	s_lwave_x_bicubic.release();
	s_lwave_y_bicubic.release();
	s_wave_x_vf.release();
	s_wave_y_vf.release();
	s_lwave_x_vf.release();
	s_lwave_y_vf.release();
	s_vf_distort.release();
	s_vf_distort_bicubic.release();
	s_noise_1_octave.release();
	s_noise_2_octaves.release();
	s_noise_3_octaves.release();
	s_noise_4_octaves.release();
	s_noise_final_remap.release();
	s_turbulence_1_octave.release();
	s_turbulence_2_octaves.release();
	s_turbulence_3_octaves.release();
	s_turbulence_4_octaves.release();
	s_hsv_adjust.release();
	s_grayscale.release();
	s_sobel_edge_detect.release();
}

EError ExecutionEngineHw::getLayerDesc(UCHAR ucLayerID,LayerDesc &desc)
{
	if(ucLayerID>=layers.size())
	{
		return eeInvalidID;
	}
	desc=layers[ucLayerID].desc;
	return eeOk;
}

EError ExecutionEngineHw::loadLayerData(UCHAR ucLayerID,DWORD /*dwByteCount*/,const void* /*pData*/)
{
	if(ucLayerID>=layers.size())
	{
		return eeInvalidID;
	}
	//TODO
	return eeOk;
}

EError ExecutionEngineHw::getLayerData(UCHAR ucLayerID,DWORD /*dwByteCount*/,
									   void * /*pData*/)
{
	if(ucLayerID>=layers.size())
	{
		return eeInvalidID;
	}
	//TODO
	return eeOk;
}

EError ExecutionEngineHw::loadLayerFromPNGFile(UCHAR ucLayerID,
											   const TCHAR *pszFileName,
											   data_ready_callback_t dr)
{
	if(ucLayerID>=layers.size())
	{
		return eeInvalidID;
	}
	if((layers[ucLayerID].desc.eFormat!=elfInt32Bit) &&
		(layers[ucLayerID].desc.eFormat!=elfInt64Bit))
	{
		return eeInvalidParameter;
	}
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	unsigned int sig_read=0;
	//png_uint_32 width=0;
	//png_uint_32 height=0;
	FILE *fp=NULL;
	if((fp=_tfopen(pszFileName,_T("rb")))==NULL)
	{
		return eeInvalidParameter;
	}
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if(png_ptr==NULL)
	{
		fclose(fp);
		return eeUnknownError;
	}
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL)
	{
		fclose(fp);
		png_destroy_read_struct(&png_ptr,png_infopp_NULL,png_infopp_NULL);
		return eeUnknownError;
	}
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
		fclose(fp);
		return eeUnknownError;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_png(png_ptr,info_ptr,PNG_TRANSFORM_SWAP_ENDIAN,png_voidp_NULL);
	//At this point you have read the entire image
	//copy the data to layer, but first do some checks on data - it has to be the same size
	//and bit depth as destination layer
	if((info_ptr->width!=layers[ucLayerID].desc.dwSizeX) ||
		(info_ptr->width!=layers[ucLayerID].desc.dwSizeY))
	{
		png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
		fclose(fp);
		return eeLayerSizeMismatch;
	}
	if(info_ptr->bit_depth==16)
	{
		if(layers[ucLayerID].desc.eFormat!=elfInt64Bit)
		{
			png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
			fclose(fp);
			return eeBitDepthMismatch;
		}
	}
	else
	{
		if(layers[ucLayerID].desc.eFormat!=elfInt32Bit)
		{
			png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
			fclose(fp);
			return eeBitDepthMismatch;
		}
	}
	png_bytep *row_pointers=png_get_rows(png_ptr, info_ptr);

	//find layer io proxy
	LayerDesc ldesc=layers[ucLayerID].desc;
	io_proxy_map_t::iterator iter=io_proxy_map.find(ldesc);
	assert(iter!=io_proxy_map.end());
	NIMPLibHwInternal::LayerProxy lpx=(*iter).second;
	//perform swizzling to BGRA
	D3DLOCKED_RECT lcr;
	lpx.proxy_surface->LockRect(&lcr,NULL,0);
	unsigned int i=0;
	unsigned int j=0;
	if(info_ptr->bit_depth==8)
	{
		for(i=0;i<ldesc.dwSizeY;i++)
		{
			unsigned char *src_base=(unsigned char*)row_pointers[i];
			unsigned char *pDest=((unsigned char*)lcr.pBits)+lcr.Pitch*i;
			for(j=0;j<ldesc.dwSizeX;j++)
			{
				if(info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA)
				{
					*pDest++=*(src_base+4*j+2);
					*pDest++=*(src_base+4*j+1);
					*pDest++=*(src_base+4*j);
					*pDest++=*(src_base+4*j+3);
				}
				else
				{
					*pDest++=*(src_base+3*j+2);
					*pDest++=*(src_base+3*j+1);
					*pDest++=*(src_base+3*j);
					*pDest++=0xff;
				}
			}
			src_base+=lcr.Pitch;
		}
	}
	else
	{
		for(i=0;i<ldesc.dwSizeY;i++)
		{
			unsigned short *src_base=(unsigned short*)row_pointers[i];
			unsigned short *pDest=((unsigned short*)lcr.pBits)+lcr.Pitch/2*i;
			for(j=0;j<ldesc.dwSizeX;j++)
			{
				if(info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA)
				{
					*pDest++=*(src_base+4*j);
					*pDest++=*(src_base+4*j+1);
					*pDest++=*(src_base+4*j+2);
					*pDest++=*(src_base+4*j+3);
				}
				else
				{
					*pDest++=*(src_base+3*j);
					*pDest++=*(src_base+3*j+1);
					*pDest++=*(src_base+3*j+2);
					*pDest++=0xffff;
				}
			}
		}
	}
	lpx.proxy_surface->UnlockRect();
	//clean up after the read, and free any memory allocated - REQUIRED 
	png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
	fclose(fp);
	if(!dr.empty())
	{
		dr();
	}
	PDIRECT3DSURFACE9 rt=NULL;
	if(FAILED(layers[ucLayerID].tx->GetSurfaceLevel(0,&rt)))
	{
		return eeUnknownError;
	}
	if(FAILED(device->UpdateSurface(lpx.proxy_surface,NULL,rt,NULL)))
	{
		rt->Release();
		return eeUnknownError;
	}
	rt->Release();
	return eeOk;
}

EError ExecutionEngineHw::saveLayerToPNGFile(UCHAR ucLayerID,
											 const TCHAR *pszFileName,
											 data_ready_callback_t dr) {
	if(ucLayerID>=layers.size())
	{
		return eeInvalidID;
	}
	assert(swizzle_buffer);
	//find layer io proxy
	LayerDesc ldesc=layers[ucLayerID].desc;
	io_proxy_map_t::iterator iter=io_proxy_map.find(ldesc);
	assert(iter!=io_proxy_map.end());
	NIMPLibHwInternal::LayerProxy lpx=(*iter).second;
	//load layer to sysmem surface
	PDIRECT3DSURFACE9 rt=NULL;
	if(FAILED(layers[ucLayerID].tx->GetSurfaceLevel(0,&rt)))
	{
		return eeUnknownError;
	}
	if(FAILED(device->GetRenderTargetData(rt,lpx.proxy_surface)))
	{
		rt->Release();
		return eeUnknownError;
	}
	rt->Release();
	//perform swizzling to RGBA
	D3DLOCKED_RECT lcr;
	lpx.proxy_surface->LockRect(&lcr,NULL,0);
	unsigned int i=0;
	unsigned int j=0;
	//TODO: assert on float format
	//swizzles with mmx might be a bit faster!
	if(layers[ucLayerID].desc.eFormat==elfInt32Bit)
	{
		unsigned char *pDest=(unsigned char*)swizzle_buffer;
		unsigned char *src_base=(unsigned char*)lcr.pBits;
		for(i=0;i<ldesc.dwSizeY;i++)
		{
			for(j=0;j<ldesc.dwSizeX;j++)
			{
				*pDest++=*(src_base+4*j+2);
				*pDest++=*(src_base+4*j+1);
				*pDest++=*(src_base+4*j);
				*pDest++=*(src_base+4*j+3);
			}
			src_base+=lcr.Pitch;
		}
	}
	else
	{
		unsigned short *pDest=(unsigned short*)swizzle_buffer;
		unsigned short *src_base=(unsigned short*)lcr.pBits;
		for(i=0;i<ldesc.dwSizeY;i++)
		{
			for(j=0;j<ldesc.dwSizeX;j++)
			{
				*pDest++=*(src_base+4*j);
				*pDest++=*(src_base+4*j+1);
				*pDest++=*(src_base+4*j+2);
				*pDest++=*(src_base+4*j+3);
			}
			src_base+=lcr.Pitch/2;
		}
	}
	lpx.proxy_surface->UnlockRect();
	//save from swizzle buffer
	FILE *fp=NULL;
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	//png_colorp palette=NULL;

	fp=_tfopen(pszFileName,_T("wb"));
	if(fp==NULL)
	{
		return eeUnknownError;
	}
	if(!dr.empty())
	{
		dr();
	}
	png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL,NULL,NULL);
	if(png_ptr==NULL)
	{
		fclose(fp);
		return eeUnknownError;
	}
	info_ptr=png_create_info_struct(png_ptr);
	if(info_ptr==NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr,png_infopp_NULL);
		return eeUnknownError;
	}
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr,&info_ptr);
		return eeUnknownError;
	}
	png_init_io(png_ptr,fp);
	int bit_depth=8;
	if(layers[ucLayerID].desc.eFormat==elfInt64Bit)
	{
		bit_depth=16;
	}
	png_set_IHDR(png_ptr,info_ptr,layers[ucLayerID].desc.dwSizeX,
		layers[ucLayerID].desc.dwSizeY,bit_depth,
		PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	DWORD dwRowCount=layers[ucLayerID].desc.dwSizeY;
	BYTE **ppRows=new BYTE*[dwRowCount];
	DWORD dwLayerSize=0;
	EError retcode=getLayerByteSize(layers[ucLayerID].desc,dwLayerSize);
	if(retcode!=eeOk)
	{
		delete [] ppRows;
		fclose(fp);
		png_destroy_write_struct(&png_ptr,&info_ptr);
		return retcode;
	}
	DWORD dwRowSize=dwLayerSize/layers[ucLayerID].desc.dwSizeY;
	for(i=0;i<dwRowCount;i++)
	{
		ppRows[i]=((unsigned char*)swizzle_buffer)+i*dwRowSize;
	}
	png_set_rows(png_ptr,info_ptr,ppRows);
	if(bit_depth==16)
	{
		png_write_png(png_ptr,info_ptr,PNG_TRANSFORM_SWAP_ENDIAN,png_voidp_NULL);
	}
	else
	{
		png_write_png(png_ptr,info_ptr,0,png_voidp_NULL);
	}
	delete [] ppRows;
	png_destroy_write_struct(&png_ptr,&info_ptr);
	fclose(fp);
	return eeOk;
}

#define CHECK_LAYER(layer) if(layers.size()<=layer) return eeInvalidParameter
#define RANGE_CHECK_INT(val,min,max) if((val<min) || (val>max)) return eeInvalidParameter

EError ExecutionEngineHw::add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	set_texture(0,ucLayer2);
	set_texture(1,ucLayer3);

	set_render_target(ucLayer1);
	s_add.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::add_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);

	memcpy(s_add_color.c1,color,4*sizeof(float));
	s_add_color.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::sub(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	set_texture(0,ucLayer2);
	set_texture(1,ucLayer3);

	set_render_target(ucLayer1);
	s_sub.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::sub_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);

	memcpy(s_sub_color.c1,color,4*sizeof(float));
	s_sub_color.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::mul(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	set_texture(0,ucLayer2);
	set_texture(1,ucLayer3);

	set_render_target(ucLayer1);
	s_mul.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::mul_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);

	memcpy(s_mul_color.c1,color,4*sizeof(float));
	s_mul_color.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::signed_add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	set_texture(0,ucLayer2);
	set_texture(1,ucLayer3);

	set_render_target(ucLayer1);
	s_signed_add.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::checkers_aa(UCHAR ucLayer1,int divisions_x,
									  int divisions_y,float color_odd[4],
									  float color_even[4])
{
	CHECK_LAYER(ucLayer1);
	RANGE_CHECK_INT(divisions_x,1,256);
	RANGE_CHECK_INT(divisions_y,1,256);

	//base i2_c2 params
	s_checkers_aa.i1=(float)divisions_x;
	s_checkers_aa.i2=(float)divisions_y;
	memcpy(s_checkers_aa.c1,color_odd,4*sizeof(float));
	memcpy(s_checkers_aa.c2,color_even,4*sizeof(float));
	//checkers_aa params
	s_checkers_aa.pixel_size[0]=(float)divisions_x/layers[ucLayer1].desc.dwSizeX;
	s_checkers_aa.pixel_size[1]=(float)divisions_y/layers[ucLayer1].desc.dwSizeY;
	s_checkers_aa.pixel_size[2]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_checkers_aa.pixel_size[3]=0.5f/layers[ucLayer1].desc.dwSizeY;
	s_checkers_aa.pixel_area_inv=1.0f/
		(s_checkers_aa.pixel_size[0]*
		s_checkers_aa.pixel_size[1]);

	set_render_target(ucLayer1);
	s_checkers_aa.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::rgradient_c(UCHAR ucLayer1,float center_x,
									  float center_y,float radius,
									  float color_inner[4],float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	s_rgradient_c.f1=center_x;
	s_rgradient_c.f2=center_y;
	s_rgradient_c.f3=1.0f/radius;
	memcpy(s_rgradient_c.c1,color_inner,4*sizeof(float));
	memcpy(s_rgradient_c.c2,color_outer,4*sizeof(float));

	set_render_target(ucLayer1);
	s_rgradient_c.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::rgradient_c_cubic(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	s_rgradient_c_cubic.f1=center_x;
	s_rgradient_c_cubic.f2=center_y;
	s_rgradient_c_cubic.f3=1.0f/radius;
	memcpy(s_rgradient_c_cubic.c1,color_inner,4*sizeof(float));
	memcpy(s_rgradient_c_cubic.c2,color_outer,4*sizeof(float));

	set_render_target(ucLayer1);
	s_rgradient_c_cubic.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::rgradient_c_sin(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	s_rgradient_c_sin.f1=center_x;
	s_rgradient_c_sin.f2=center_y;
	s_rgradient_c_sin.f3=1.0f/radius;
	memcpy(s_rgradient_c_sin.c1,color_inner,4*sizeof(float));
	memcpy(s_rgradient_c_sin.c2,color_outer,4*sizeof(float));

	set_render_target(ucLayer1);
	s_rgradient_c_sin.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::rgradient_e(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	s_rgradient_e.f1=center_x;
	s_rgradient_e.f2=center_y;
	s_rgradient_e.f3=1.0f/radius;
	memcpy(s_rgradient_e.c1,color_inner,4*sizeof(float));
	memcpy(s_rgradient_e.c2,color_outer,4*sizeof(float));

	set_render_target(ucLayer1);
	s_rgradient_e.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::rgradient_e_sin(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	s_rgradient_e_sin.f1=center_x;
	s_rgradient_e_sin.f2=center_y;
	s_rgradient_e_sin.f3=1.0f/radius;
	memcpy(s_rgradient_e_sin.c1,color_inner,4*sizeof(float));
	memcpy(s_rgradient_e_sin.c2,color_outer,4*sizeof(float));

	set_render_target(ucLayer1);
	s_rgradient_e_sin.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::noise(UCHAR ucLayer1,int noise_scale,int seed,
								int octaves,float color1[4],float color2[4])
{
	CHECK_LAYER(ucLayer1);

	if(octaves<1)
	{
		octaves=1;
	}
	if(octaves>8)
	{
		octaves=8;
	}
	if(first_r32f_layer==0)
	{
		UCHAR last_layer=(UCHAR)layers.size();
		LayerDesc ld;
		ld.dwSizeX=layers[ucLayer1].desc.dwSizeX;
		ld.dwSizeY=layers[ucLayer1].desc.dwSizeY;
		ld.eFormat=elfFloat32Bit;
		EError err=allocateLayers(ld,3);
		if(err!=eeOk)
		{
			return err;
		}
		first_r32f_layer=last_layer;
	}

	srand(seed);
	s_noise_1_octave.init_noise_tables(device);

	UCHAR current_fp_rt_offset=0;
	UCHAR prev_fp_rt_offset=0;
	float octave_scale=1.0f;
	float octave_weight=1.0f;
	while(octaves>0)
	{
		if(octaves>=4)
		{
			s_noise_4_octaves.noise_scale=(float)noise_scale;
			s_noise_4_octaves.octave_scale=octave_scale;
			s_noise_4_octaves.octave_weight=octave_weight;
			s_noise_4_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_noise_4_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_noise_1_octave.noise_perm_tbl);
			set_texture(1,s_noise_1_octave.noise_grad_tbl);
			s_noise_4_octaves.setup(device);
			octave_scale*=16.0f;
			octave_weight*=0.0625f;
			octaves-=4;
		}
		else if(octaves>=3)
		{
			s_noise_3_octaves.noise_scale=(float)noise_scale;
			s_noise_3_octaves.octave_scale=octave_scale;
			s_noise_3_octaves.octave_weight=octave_weight;
			s_noise_3_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_noise_3_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_noise_1_octave.noise_perm_tbl);
			set_texture(1,s_noise_1_octave.noise_grad_tbl);
			s_noise_3_octaves.setup(device);
			octave_scale*=8.0f;
			octave_weight*=0.125f;
			octaves-=3;
		}
		else if(octaves>=2)
		{
			s_noise_2_octaves.noise_scale=(float)noise_scale;
			s_noise_2_octaves.octave_scale=octave_scale;
			s_noise_2_octaves.octave_weight=octave_weight;
			s_noise_2_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_noise_2_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_noise_1_octave.noise_perm_tbl);
			set_texture(1,s_noise_1_octave.noise_grad_tbl);
			s_noise_2_octaves.setup(device);
			octave_scale*=4.0f;
			octave_weight*=0.25f;
			octaves-=2;
		}
		else
		{
			s_noise_1_octave.noise_scale=(float)noise_scale;
			s_noise_1_octave.octave_scale=octave_scale;
			s_noise_1_octave.octave_weight=octave_weight;
			s_noise_1_octave.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_noise_1_octave.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_noise_1_octave.noise_perm_tbl);
			set_texture(1,s_noise_1_octave.noise_grad_tbl);
			s_noise_1_octave.setup(device);
			octave_scale*=2.0f;
			octave_weight*=0.5f;
			octaves--;
		}
		set_render_target(first_r32f_layer+current_fp_rt_offset);
		draw();
		if(current_fp_rt_offset!=prev_fp_rt_offset)
		{
			//accumulate if needed
			if(prev_fp_rt_offset==0)
			{
				add(first_r32f_layer+2,first_r32f_layer+prev_fp_rt_offset,first_r32f_layer+current_fp_rt_offset);
				prev_fp_rt_offset=2;
			}
			else
			{
				add(first_r32f_layer,first_r32f_layer+prev_fp_rt_offset,first_r32f_layer+current_fp_rt_offset);
				prev_fp_rt_offset=0;
			}
		}
		else
		{
			if(octaves>0)
			{
				current_fp_rt_offset=1;
			}
		}
	}
	s_noise_final_remap.f1=0.94f;
	s_noise_final_remap.f2=0.526f;
	memcpy(s_noise_final_remap.c1,color1,4*sizeof(float));
	memcpy(s_noise_final_remap.c2,color2,4*sizeof(float));

	set_texture(0,first_r32f_layer+prev_fp_rt_offset);
	set_render_target(ucLayer1);
	s_noise_final_remap.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::turbulence(UCHAR ucLayer1,int noise_scale,int seed,
									 int octaves,float color1[4],float color2[4])
{
	CHECK_LAYER(ucLayer1);

	if(octaves<1)
	{
		octaves=1;
	}
	if(octaves>8)
	{
		octaves=8;
	}
	if(first_r32f_layer==0)
	{
		UCHAR last_layer=(UCHAR)layers.size();
		LayerDesc ld;
		ld.dwSizeX=layers[ucLayer1].desc.dwSizeX;
		ld.dwSizeY=layers[ucLayer1].desc.dwSizeY;
		ld.eFormat=elfFloat32Bit;
		EError err=allocateLayers(ld,3);
		if(err!=eeOk)
		{
			return err;
		}
		first_r32f_layer=last_layer;
	}

	srand(seed);
	s_turbulence_1_octave.init_noise_tables(device);

	UCHAR current_fp_rt_offset=0;
	UCHAR prev_fp_rt_offset=0;
	float octave_scale=1.0f;
	float octave_weight=1.0f;
	while(octaves>0)
	{
		if(octaves>=4)
		{
			s_turbulence_4_octaves.noise_scale=(float)noise_scale;
			s_turbulence_4_octaves.octave_scale=octave_scale;
			s_turbulence_4_octaves.octave_weight=octave_weight;
			s_turbulence_4_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_turbulence_4_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_turbulence_1_octave.noise_perm_tbl);
			set_texture(1,s_turbulence_1_octave.noise_grad_tbl);
			s_turbulence_4_octaves.setup(device);
			octave_scale*=16.0f;
			octave_weight*=0.0625f;
			octaves-=4;
		}
		else if(octaves>=3)
		{
			s_turbulence_3_octaves.noise_scale=(float)noise_scale;
			s_turbulence_3_octaves.octave_scale=octave_scale;
			s_turbulence_3_octaves.octave_weight=octave_weight;
			s_turbulence_3_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_turbulence_3_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_turbulence_1_octave.noise_perm_tbl);
			set_texture(1,s_turbulence_1_octave.noise_grad_tbl);
			s_turbulence_3_octaves.setup(device);
			octave_scale*=8.0f;
			octave_weight*=0.125f;
			octaves-=3;
		}
		else if(octaves>=2)
		{
			s_turbulence_2_octaves.noise_scale=(float)noise_scale;
			s_turbulence_2_octaves.octave_scale=octave_scale;
			s_turbulence_2_octaves.octave_weight=octave_weight;
			s_turbulence_2_octaves.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_turbulence_2_octaves.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_turbulence_1_octave.noise_perm_tbl);
			set_texture(1,s_turbulence_1_octave.noise_grad_tbl);
			s_turbulence_2_octaves.setup(device);
			octave_scale*=4.0f;
			octave_weight*=0.25f;
			octaves-=2;
		}
		else
		{
			s_turbulence_1_octave.noise_scale=(float)noise_scale;
			s_turbulence_1_octave.octave_scale=octave_scale;
			s_turbulence_1_octave.octave_weight=octave_weight;
			s_turbulence_1_octave.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
			s_turbulence_1_octave.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;
			set_texture(0,s_turbulence_1_octave.noise_perm_tbl);
			set_texture(1,s_turbulence_1_octave.noise_grad_tbl);
			s_turbulence_1_octave.setup(device);
			octave_scale*=2.0f;
			octave_weight*=0.5f;
			octaves--;
		}
		set_render_target(first_r32f_layer+current_fp_rt_offset);
		draw();
		if(current_fp_rt_offset!=prev_fp_rt_offset)
		{
			//accumulate if needed
			if(prev_fp_rt_offset==0)
			{
				add(first_r32f_layer+2,first_r32f_layer+prev_fp_rt_offset,first_r32f_layer+current_fp_rt_offset);
				prev_fp_rt_offset=2;
			}
			else
			{
				add(first_r32f_layer,first_r32f_layer+prev_fp_rt_offset,first_r32f_layer+current_fp_rt_offset);
				prev_fp_rt_offset=0;
			}
		}
		else
		{
			if(octaves>0)
			{
				current_fp_rt_offset=1;
			}
		}
	}
	s_noise_final_remap.f1=0.0f;
	s_noise_final_remap.f2=1.0f;
	memcpy(s_noise_final_remap.c1,color1,4*sizeof(float));
	memcpy(s_noise_final_remap.c2,color2,4*sizeof(float));

	set_texture(0,first_r32f_layer+prev_fp_rt_offset);
	set_render_target(ucLayer1);
	s_noise_final_remap.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::wave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_wave_x.f1=amplitude;
	s_wave_x.f2=phase;
	s_wave_x.i1=(float)nwaves;
	s_wave_x.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_x.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2,true);
	set_render_target(ucLayer1);
	s_wave_x.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::wave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_wave_x_bicubic.f1=amplitude;
	s_wave_x_bicubic.f2=phase;
	s_wave_x_bicubic.i1=(float)nwaves;
	s_wave_x_bicubic.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_x_bicubic.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	s_wave_x_bicubic.tx_sizes[0]=(float)layers[ucLayer1].desc.dwSizeX;
	s_wave_x_bicubic.tx_sizes[1]=(float)layers[ucLayer1].desc.dwSizeY;
	s_wave_x_bicubic.tx_sizes[2]=1.0f/layers[ucLayer1].desc.dwSizeX;
	s_wave_x_bicubic.tx_sizes[3]=1.0f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);
	s_wave_x_bicubic.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_lwave_x.f1=amplitude;
	s_lwave_x.f2=phase;
	s_lwave_x.i1=(float)nwaves;
	s_lwave_x.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_x.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2,true);
	set_render_target(ucLayer1);
	s_lwave_x.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_lwave_x_bicubic.f1=amplitude;
	s_lwave_x_bicubic.f2=phase;
	s_lwave_x_bicubic.i1=(float)nwaves;
	s_lwave_x_bicubic.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_x_bicubic.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	s_lwave_x_bicubic.tx_sizes[0]=(float)layers[ucLayer1].desc.dwSizeX;
	s_lwave_x_bicubic.tx_sizes[1]=(float)layers[ucLayer1].desc.dwSizeY;
	s_lwave_x_bicubic.tx_sizes[2]=1.0f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_x_bicubic.tx_sizes[3]=1.0f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);
	s_lwave_x_bicubic.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::wave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_wave_y.f1=amplitude;
	s_wave_y.f2=phase;
	s_wave_y.i1=(float)nwaves;
	s_wave_y.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_y.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2,true);
	set_render_target(ucLayer1);
	s_wave_y.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::wave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_wave_y_bicubic.f1=amplitude;
	s_wave_y_bicubic.f2=phase;
	s_wave_y_bicubic.i1=(float)nwaves;
	s_wave_y_bicubic.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_y_bicubic.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	s_wave_y_bicubic.tx_sizes[0]=(float)layers[ucLayer1].desc.dwSizeX;
	s_wave_y_bicubic.tx_sizes[1]=(float)layers[ucLayer1].desc.dwSizeY;
	s_wave_y_bicubic.tx_sizes[2]=1.0f/layers[ucLayer1].desc.dwSizeX;
	s_wave_y_bicubic.tx_sizes[3]=1.0f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);
	s_wave_y_bicubic.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_lwave_y.f1=amplitude;
	s_lwave_y.f2=phase;
	s_lwave_y.i1=(float)nwaves;
	s_lwave_y.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_y.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2,true);
	set_render_target(ucLayer1);
	s_lwave_y.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	s_lwave_y_bicubic.f1=amplitude;
	s_lwave_y_bicubic.f2=phase;
	s_lwave_y_bicubic.i1=(float)nwaves;
	s_lwave_y_bicubic.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_y_bicubic.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	s_lwave_y_bicubic.tx_sizes[0]=(float)layers[ucLayer1].desc.dwSizeX;
	s_lwave_y_bicubic.tx_sizes[1]=(float)layers[ucLayer1].desc.dwSizeY;
	s_lwave_y_bicubic.tx_sizes[2]=1.0f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_y_bicubic.tx_sizes[3]=1.0f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2);
	set_render_target(ucLayer1);
	s_lwave_y_bicubic.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::vf_distort(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3,
									 float scale_x,float scale_y)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	s_vf_distort.f1=scale_x;
	s_vf_distort.f2=scale_y;

	set_texture(0,ucLayer2,true);
	set_texture(1,ucLayer3);
	set_render_target(ucLayer1);
	s_vf_distort.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::vf_distort_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
											 UCHAR ucLayer3,float scale_x,
											 float scale_y)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);

	s_vf_distort_bicubic.f1=scale_x;
	s_vf_distort_bicubic.f2=scale_y;

	s_vf_distort_bicubic.tx_sizes[0]=(float)layers[ucLayer1].desc.dwSizeX;
	s_vf_distort_bicubic.tx_sizes[1]=(float)layers[ucLayer1].desc.dwSizeY;
	s_vf_distort_bicubic.tx_sizes[2]=1.0f/layers[ucLayer1].desc.dwSizeX;
	s_vf_distort_bicubic.tx_sizes[3]=1.0f/layers[ucLayer1].desc.dwSizeY;

	set_texture(0,ucLayer2);
	set_texture(1,ucLayer3);
	set_render_target(ucLayer1);
	s_vf_distort_bicubic.setup(device);
	draw();
	return eeOk;
}

EError ExecutionEngineHw::wave_x_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);

	s_wave_x_vf.f1=amplitude;
	s_wave_x_vf.f2=phase;
	s_wave_x_vf.i1=(float)nwaves;
	s_wave_x_vf.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_x_vf.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_render_target(ucLayer1);
	s_wave_x_vf.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_x_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);

	s_lwave_x_vf.f1=amplitude;
	s_lwave_x_vf.f2=phase;
	s_lwave_x_vf.i1=(float)nwaves;
	s_lwave_x_vf.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_x_vf.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_render_target(ucLayer1);
	s_lwave_x_vf.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::wave_y_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);

	s_wave_y_vf.f1=amplitude;
	s_wave_y_vf.f2=phase;
	s_wave_y_vf.i1=(float)nwaves;
	s_wave_y_vf.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_wave_y_vf.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_render_target(ucLayer1);
	s_wave_y_vf.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::lwave_y_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);

	s_lwave_y_vf.f1=amplitude;
	s_lwave_y_vf.f2=phase;
	s_lwave_y_vf.i1=(float)nwaves;
	s_lwave_y_vf.shading_space_offset[0]=0.5f/layers[ucLayer1].desc.dwSizeX;
	s_lwave_y_vf.shading_space_offset[1]=0.5f/layers[ucLayer1].desc.dwSizeY;

	set_render_target(ucLayer1);
	s_lwave_y_vf.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::hsv_adjust(UCHAR ucLayer1,UCHAR ucLayer2,float h_rotation_deg,
									 float s_offset,float v_offset)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	h_rotation_deg=fmodf(h_rotation_deg,360.0f);
	h_rotation_deg+=360.0f;
	h_rotation_deg=fmodf(h_rotation_deg,360.0f);
	h_rotation_deg/=60.0f;

	s_hsv_adjust.f1=h_rotation_deg;
	s_hsv_adjust.f2=s_offset;
	s_hsv_adjust.f3=v_offset;

	set_texture(0,ucLayer2);

	set_render_target(ucLayer1);
	s_hsv_adjust.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::grayscale(UCHAR ucLayer1,UCHAR ucLayer2)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	set_texture(0,ucLayer2);

	set_render_target(ucLayer1);
	s_grayscale.setup(device);
	draw();

	return eeOk;
}

EError ExecutionEngineHw::sobel_edge_detect(UCHAR ucLayer1,UCHAR ucLayer2,float sample_spacing)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);

	set_texture(0,ucLayer2);

	set_render_target(ucLayer1);
	s_sobel_edge_detect.f1=sample_spacing;
	s_sobel_edge_detect.setup(device);
	draw();

	return eeOk;
}

