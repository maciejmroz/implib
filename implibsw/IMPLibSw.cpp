/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


#include "stdafx.h"
#include "IMPLibSw.h"
#include "IMPDispatcher.h"
#include <malloc.h>
#include <algorithm>
#include <png.h>

using namespace NIMPLib;

EError NIMPLibSwInternal::generic_error(EError e)
{
	return e;
}

ExecutionEngineSw::ExecutionEngineSw()
{
	//CPU detection makes use of Win2k/XP api so that assembly can be
	//minimized here
	if(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
	{
		//it also means we are at least on Windows XP - w2k kernel
		//does not detect SSE2, is only compatible with it
		m_eProcessorCaps=cpSSE2;
	}
	else if(IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
	{
		//so we may as well be running SSE2 CPU on W2K, check for that
		int sse2=0;
		__asm {
			mov eax, 1				//request for feature flags
			cpuid					//0Fh, 0A2h cpuid instruction
			test edx, 004000000h	//bit 26 in feature flags equal to 1
			jnz Found
			jmp End
		Found:
			mov sse2,1
		End:
		}
		if(sse2)
		{
			m_eProcessorCaps=cpSSE2;
		}
		else
		{
			m_eProcessorCaps=cpSSE;
		}
	}
	else
	{
		//theoretically code should check for that too, but it is
		//impossible impossible to find cpu that does not have MMX
		//extensions these days
		m_eProcessorCaps=cpGenericMMX;
	}
	//by default use generic path anyway, user may change that by request
	m_eCodePath=cpGenericMMX;
}

ExecutionEngineSw::~ExecutionEngineSw()
{
}

EError ExecutionEngineSw::init(CodePath code_path)
{
	if(code_path>m_eProcessorCaps)
	{
		m_eCodePath=m_eProcessorCaps;
	}
	else
	{
		m_eCodePath=code_path;
	}
	return eeOk;
}

bool ExecutionEngineSw::isLayerFormatSupported(ELayerFormat /*format*/)
{
	return false;
}

EError ExecutionEngineSw::setLayerFormat(ELayerFormat /*format*/)
{
	return eeNotImplemented;
}

bool ExecutionEngineSw::layersMustBePow2Sized()
{
	return true;			//may be less restrictive in future
}

struct free_sw_layer_funct
{
	void operator()(NIMPLibSwInternal::Layer &rf_layer)
	{
		_aligned_free(rf_layer.byte_data);
	}
};

EError ExecutionEngineSw::getLayerByteSize(LayerDesc &desc,DWORD &dwByteSize)
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

EError ExecutionEngineSw::allocateLayers(LayerDesc &desc,DWORD dwCount)
{
	if(dwCount==0)
	{
		return eeOk;
	}
	std::vector<NIMPLibSwInternal::Layer> temp_layer_container;
	NIMPLibSwInternal::Layer temp_layer;
	EError retcode=eeOk;
	bool rollback=false;
	DWORD dwByteSize=0;
	retcode=getLayerByteSize(desc,dwByteSize);
	if(retcode!=eeOk)
	{
		return retcode;
	}
	DWORD i=0;
	for(;i<dwCount;i++)
	{
		temp_layer.desc=desc;
		temp_layer.byte_data=(BYTE*)_aligned_malloc(dwByteSize,16);
		if(!temp_layer.byte_data)
		{
			rollback=true;
			retcode=eeOutOfResources;
			break;
		}
		temp_layer_container.push_back(temp_layer);
	}
	if(rollback)
	{
		for_each(temp_layer_container.begin(),temp_layer_container.end(),
			free_sw_layer_funct());
		temp_layer_container.clear();
		return retcode;
	}
	m_Layers.insert(m_Layers.end(),
		temp_layer_container.begin(),temp_layer_container.end());
	return retcode;
}

void ExecutionEngineSw::freeLayers()
{
	for_each(m_Layers.begin(),m_Layers.end(),free_sw_layer_funct());
	m_Layers.clear();
}

EError ExecutionEngineSw::getLayerDesc(UCHAR ucLayerID,LayerDesc &desc)
{
	if(ucLayerID>=m_Layers.size())
	{
		return eeInvalidID;
	}
	desc=m_Layers[ucLayerID].desc;
	return eeOk;
}

EError ExecutionEngineSw::loadLayerData(UCHAR ucLayerID,DWORD dwByteCount,const void *pData)
{
	if(ucLayerID>=m_Layers.size())
	{
		return eeInvalidID;
	}
	DWORD dwExpectedByteCount=0;
	EError err=getLayerByteSize(m_Layers[ucLayerID].desc,dwExpectedByteCount);
	if(err!=eeOk)
	{
		return err;
	}
	if(dwByteCount!=dwExpectedByteCount)
	{
		return eeInvalidParameter;
	}
	memcpy(m_Layers[ucLayerID].byte_data,pData,dwByteCount);
	return eeOk;
}

EError ExecutionEngineSw::getLayerData(UCHAR ucLayerID,DWORD dwByteCount,
									   void *pData)
{
	if(ucLayerID>=m_Layers.size())
	{
		return eeInvalidID;
	}
	DWORD dwExpectedByteCount=0;
	EError err=getLayerByteSize(m_Layers[ucLayerID].desc,dwExpectedByteCount);
	if(err!=eeOk)
	{
		return err;
	}
	if(dwByteCount!=dwExpectedByteCount)
	{
		return eeInvalidParameter;
	}
	memcpy(pData,m_Layers[ucLayerID].byte_data,dwByteCount);
	return eeOk;
}

EError ExecutionEngineSw::loadLayerFromPNGFile(UCHAR ucLayerID,
											   const TCHAR *pszFileName,
											   data_ready_callback_t dr)
{
	if(ucLayerID>=m_Layers.size())
	{
		return eeInvalidID;
	}
	if((m_Layers[ucLayerID].desc.eFormat!=elfInt32Bit) &&
		(m_Layers[ucLayerID].desc.eFormat!=elfInt64Bit))
	{
		return eeInvalidParameter;
	}
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;
	unsigned int sig_read=0;
	png_uint_32 width=0;
	png_uint_32 height=0;
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
	if((info_ptr->width!=m_Layers[ucLayerID].desc.dwSizeX) ||
		(info_ptr->width!=m_Layers[ucLayerID].desc.dwSizeY))
	{
		png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
		fclose(fp);
		return eeLayerSizeMismatch;
	}
	if(info_ptr->bit_depth==16)
	{
		if(m_Layers[ucLayerID].desc.eFormat!=elfInt64Bit)
		{
			png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
			fclose(fp);
			return eeBitDepthMismatch;
		}
	}
	else
	{
		if(m_Layers[ucLayerID].desc.eFormat!=elfInt32Bit)
		{
			png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
			fclose(fp);
			return eeBitDepthMismatch;
		}
	}
	png_bytep *row_pointers=png_get_rows(png_ptr, info_ptr);

	width=info_ptr->width;
	height=info_ptr->height;
	png_uint_32 row_id=0;
	png_uint_32 i=0;

	if(info_ptr->bit_depth==16)
	{
		for(;row_id<height;row_id++)
		{
			unsigned short *in_row=(unsigned short*)row_pointers[row_id];
			unsigned short *out_row=m_Layers[ucLayerID].ushort_data+4*width*row_id;
			for(i=0;i<width;i++)
			{
				*out_row++=*in_row++;
				*out_row++=*in_row++;
				*out_row++=*in_row++;
				if(info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA)
				{
					*out_row++=*in_row++;
				}
				else
				{
					*out_row++=0xffff;
				}
			}
		}
	}
	else
	{
		for(;row_id<height;row_id++)
		{
			png_bytep in_row=row_pointers[row_id];
			unsigned char *out_row=m_Layers[ucLayerID].byte_data+4*width*row_id;
			for(i=0;i<width;i++)
			{
				*out_row++=*in_row++;
				*out_row++=*in_row++;
				*out_row++=*in_row++;
				if(info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA)
				{
					*out_row++=*in_row++;
				}
				else
				{
					*out_row++=0xff;
				}
			}
		}
	}
	//clean up after the read, and free any memory allocated - REQUIRED 
	png_destroy_read_struct(&png_ptr,&info_ptr,png_infopp_NULL);
	fclose(fp);
	if(!dr.empty())
	{
		dr();
	}
	return eeOk;
}

EError ExecutionEngineSw::saveLayerToPNGFile(UCHAR ucLayerID,
											 const TCHAR *pszFileName,
											 data_ready_callback_t dr) {
	if(ucLayerID>=m_Layers.size())
	{
		return eeInvalidID;
	}
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
	if(m_Layers[ucLayerID].desc.eFormat==elfInt64Bit)
	{
		bit_depth=16;
	}
	png_set_IHDR(png_ptr,info_ptr,m_Layers[ucLayerID].desc.dwSizeX,
		m_Layers[ucLayerID].desc.dwSizeY,bit_depth,
		PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	DWORD dwRowCount=m_Layers[ucLayerID].desc.dwSizeY;
	BYTE **ppRows=new BYTE*[dwRowCount];
	DWORD i=0;
	DWORD dwLayerSize=0;
	EError retcode=getLayerByteSize(m_Layers[ucLayerID].desc,dwLayerSize);
	if(retcode!=eeOk)
	{
		delete [] ppRows;
		fclose(fp);
		png_destroy_write_struct(&png_ptr,&info_ptr);
		return retcode;
	}
	DWORD dwRowSize=dwLayerSize/m_Layers[ucLayerID].desc.dwSizeY;
	for(i=0;i<dwRowCount;i++)
	{
		ppRows[i]=m_Layers[ucLayerID].byte_data+i*dwRowSize;
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

#define CHECK_LAYER(layer) if(m_Layers.size()<=layer) return eeInvalidParameter
#define RANGE_CHECK_INT(val,min,max) if((val<min) || (val>max)) return eeInvalidParameter

EError ExecutionEngineSw::add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::add
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3]);
}

EError ExecutionEngineSw::add_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::add_color
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],color);
}

EError ExecutionEngineSw::sub(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::sub
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3]);
}

EError ExecutionEngineSw::sub_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::sub_color
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],color);
}

EError ExecutionEngineSw::mul(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::mul
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3]);
}

EError ExecutionEngineSw::mul_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4])
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::mul_color
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],color);
}

EError ExecutionEngineSw::signed_add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::signed_add
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3]);
}

EError ExecutionEngineSw::checkers_aa(UCHAR ucLayer1,int divisions_x,
									  int divisions_y,float color_odd[4],
									  float color_even[4])
{
	CHECK_LAYER(ucLayer1);
	RANGE_CHECK_INT(divisions_x,1,256);
	RANGE_CHECK_INT(divisions_y,1,256);
	return NIMPLibSwInternal::checkers_aa
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],divisions_x,divisions_y,color_odd,color_even);
}

EError ExecutionEngineSw::rgradient_c(UCHAR ucLayer1,float center_x,
									  float center_y,float radius,
									  float color_inner[4],float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::rgradient_c
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],center_x,center_y,radius,color_inner,color_outer);
}

EError ExecutionEngineSw::rgradient_c_cubic(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::rgradient_c_cubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],center_x,center_y,radius,color_inner,color_outer);
}

EError ExecutionEngineSw::rgradient_c_sin(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::rgradient_c_sin
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],center_x,center_y,radius,color_inner,color_outer);
}

EError ExecutionEngineSw::rgradient_e(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::rgradient_e
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],center_x,center_y,radius,color_inner,color_outer);
}

EError ExecutionEngineSw::rgradient_e_sin(UCHAR ucLayer1,float center_x,
											float center_y,float radius,
											float color_inner[4],
											float color_outer[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::rgradient_e_sin
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],center_x,center_y,radius,color_inner,color_outer);
}

EError ExecutionEngineSw::noise(UCHAR ucLayer1,int noise_scale,int seed,
								int octaves,float color1[4],float color2[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::noise
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],noise_scale,seed,octaves,color1,color2);
}

EError ExecutionEngineSw::turbulence(UCHAR ucLayer1,int noise_scale,int seed,
									 int octaves,float color1[4],float color2[4])
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::turbulence
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],noise_scale,seed,octaves,color1,color2);
}

EError ExecutionEngineSw::wave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::wave_x
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::wave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::wave_x_bicubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::lwave_x
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::lwave_x_bicubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::wave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::wave_y
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::wave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::wave_y_bicubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::lwave_y
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
										 float amplitude,float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::lwave_y_bicubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::vf_distort(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3,
									 float scale_x,float scale_y)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::vf_distort
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3],
			scale_x,scale_y);
}

EError ExecutionEngineSw::vf_distort_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,
											 UCHAR ucLayer3,float scale_x,
											 float scale_y)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	CHECK_LAYER(ucLayer3);
	return NIMPLibSwInternal::vf_distort_bicubic
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],m_Layers[ucLayer3],
			scale_x,scale_y);
}

EError ExecutionEngineSw::wave_x_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::wave_x_vf
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_x_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::lwave_x_vf
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::wave_y_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::wave_y_vf
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::lwave_y_vf(UCHAR ucLayer1,float amplitude,
								 float phase,int nwaves)
{
	CHECK_LAYER(ucLayer1);
	return NIMPLibSwInternal::lwave_y_vf
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],amplitude,phase,nwaves);
}

EError ExecutionEngineSw::hsv_adjust(UCHAR ucLayer1,UCHAR ucLayer2,float h_rotation_deg,
									 float s_offset,float v_offset)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::hsv_adjust
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2],h_rotation_deg,s_offset,v_offset);
}

EError ExecutionEngineSw::grayscale(UCHAR ucLayer1,UCHAR ucLayer2)
{
	CHECK_LAYER(ucLayer1);
	CHECK_LAYER(ucLayer2);
	return NIMPLibSwInternal::grayscale
		[m_Layers[ucLayer1].desc.eFormat]
		[m_eCodePath]
		(m_Layers[ucLayer1],m_Layers[ucLayer2]);
}

EError ExecutionEngineSw::sobel_edge_detect(UCHAR /*ucLayer1*/,UCHAR /*ucLayer2*/,float /*sample_spacing*/)
{
	return eeNotImplemented;
}
