/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


#ifndef _IMP_LIB_SW_H_
#define _IMP_LIB_SW_H_

#if _MSC_VER>=1400
	#define restrict_ptr __restrict
#else
	#define restrict_ptr
#endif

namespace NIMPLib {

	namespace NIMPLibSwInternal {
		struct Layer {
			LayerDesc desc;
			union {
				unsigned char*		restrict_ptr byte_data;
				unsigned short*		restrict_ptr ushort_data;
				float*				restrict_ptr float_data;
			};
		};
	};

	class ExecutionEngineSw : public ExecutionEngine {
		CodePath									m_eProcessorCaps;
		CodePath									m_eCodePath;
		std::vector<NIMPLibSwInternal::Layer>		m_Layers;
		EError getLayerByteSize(LayerDesc &desc,DWORD &dwByteSize);
	public:
		ExecutionEngineSw();
		~ExecutionEngineSw();
		bool isLayerFormatSupported(ELayerFormat format);
		EError setLayerFormat(ELayerFormat format);
		bool layersMustBePow2Sized();
		EError allocateLayers(LayerDesc &desc,DWORD dwCount);
		void freeLayers();
		EError getLayerDesc(UCHAR ucLayerID,LayerDesc &desc);
		EError loadLayerData(UCHAR ucLayerID,DWORD dwByteCount,const void *pData);
		EError getLayerData(UCHAR ucLayerID,DWORD dwByteCount,void *pData);
		EError loadLayerFromPNGFile(UCHAR ucLayerID,const TCHAR *pszFileName,
			data_ready_callback_t dr);
		EError saveLayerToPNGFile(UCHAR ucLayerID,const TCHAR *pszFileName,
			data_ready_callback_t dr);
		EError init(CodePath code_path);
		//operations
		EError add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3);
		EError add_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4]);
		EError sub(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3);
		EError sub_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4]);
		EError mul(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3);
		EError mul_color(UCHAR ucLayer1,UCHAR ucLayer2,float color[4]);
		EError signed_add(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3);

		EError checkers_aa(UCHAR ucLayer1,int divisions_x,int divisions_y,
			float color_odd[4],float color_even[4]);
		EError rgradient_c(UCHAR ucLayer1,float center_x,float center_y,
			float radius,float color_inner[4],float color_outer[4]);
		EError rgradient_c_cubic(UCHAR ucLayer1,float center_x,float center_y,
			float radius,float color_inner[4],float color_outer[4]);
		EError rgradient_c_sin(UCHAR ucLayer1,float center_x,float center_y,
			float radius,float color_inner[4],float color_outer[4]);
		EError rgradient_e(UCHAR ucLayer1,float center_x,float center_y,
			float radius,float color_inner[4],float color_outer[4]);
		EError rgradient_e_sin(UCHAR ucLayer1,float center_x,float center_y,
			float radius,float color_inner[4],float color_outer[4]);
		EError noise(UCHAR ucLayer1,int noise_scale,int seed,int octaves,
			float color1[4],float color2[4]);
		EError turbulence(UCHAR ucLayer1,int noise_scale,int seed,
			int octaves,float color1[4],float color2[4]);

		EError wave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError wave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError lwave_x(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError lwave_x_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError wave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError wave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError lwave_y(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError lwave_y_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,float amplitude,
			float phase,int nwaves);
		EError vf_distort(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3,
			float scale_x,float scale_y);
		EError vf_distort_bicubic(UCHAR ucLayer1,UCHAR ucLayer2,UCHAR ucLayer3,
			float scale_x,float scale_y);

		EError wave_x_vf(UCHAR ucLayer1,float amplitude,float phase,int nwaves);
		EError lwave_x_vf(UCHAR ucLayer1,float amplitude,float phase,int nwaves);
		EError wave_y_vf(UCHAR ucLayer1,float amplitude,float phase,int nwaves);
		EError lwave_y_vf(UCHAR ucLayer1,float amplitude,float phase,int nwaves);

		EError hsv_adjust(UCHAR ucLayer1,UCHAR ucLayer2,float h_rotation_deg,
			float s_offset,float v_offset);
		EError grayscale(UCHAR ucLayer1,UCHAR ucLayer2);

		EError sobel_edge_detect(UCHAR ucLayer1,UCHAR ucLayer2,float sample_spacing);
	};
};

#endif