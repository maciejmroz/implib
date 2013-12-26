/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */



/*
 *	Internal part of software execution engine - just a set of
 *  functions being reference implementation of library operators
 */

//function naming convention:
//OPERATOR NAME+FORMAT+"GenericC" where:
//format is one of Int32, Int64, Fp128

#ifndef _IMP_LIB_SW_GENERIC_C_KERNEL_
#define _IMP_LIB_SW_GENERIC_C_KERNEL_

namespace NIMPLib {
	namespace NIMPLibSwInternal {
		//add
		extern EError addInt32GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError addInt64GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError addFp128GenericC(Layer &layer1,Layer &layer2,Layer &layer3);

		//add color
		extern EError addColorInt32GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError addColorInt64GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError addColorFp128GenericC(Layer &layer1,Layer &layer2,
			float color[4]);

		//sub
		extern EError subInt32GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError subInt64GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError subFp128GenericC(Layer &layer1,Layer &layer2,Layer &layer3);

		//sub color
		extern EError subColorInt32GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError subColorInt64GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError subColorFp128GenericC(Layer &layer1,Layer &layer2,
			float color[4]);

		//mul
		extern EError mulInt32GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError mulInt64GenericC(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError mulFp128GenericC(Layer &layer1,Layer &layer2,Layer &layer3);

		//mul color
		extern EError mulColorInt32GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError mulColorInt64GenericC(Layer &layer1,Layer &layer2,
			float color[4]);
		extern EError mulColorFp128GenericC(Layer &layer1,Layer &layer2,
			float color[4]);

		//signed_add
		extern EError signed_addInt32GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3);
		extern EError signed_addInt64GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3);

		extern EError checkers_aaInt32GenericC(Layer &layer1,int divisions_x,
			int divisions_y,float color_odd[4],float color_even[4]);
		extern EError checkers_aaInt64GenericC(Layer &layer1,int divisions_x,
			int divisions_y,float color_odd[4],float color_even[4]);
		extern EError checkers_aaFp128GenericC(Layer &layer1,int divisions_x,
			int divisions_y,float color_odd[4],float color_even[4]);

		extern EError rgradient_cInt32GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_cInt64GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_cFp128GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);

		extern EError rgradient_c_cubicInt32GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_c_cubicInt64GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_c_cubicFp128GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);

		extern EError rgradient_c_sinInt32GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_c_sinInt64GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_c_sinFp128GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);

		extern EError rgradient_eInt32GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_eInt64GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_eFp128GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);

		extern EError rgradient_e_sinInt32GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_e_sinInt64GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);
		extern EError rgradient_e_sinFp128GenericC(Layer &layer1,float center_x,
			float center_y,float radius,float color_inner[4],float color_outer[4]);

		extern EError noiseInt32GenericC(Layer &layer1,int noise_scale,int seed,
			int octaves,float color1[4],float color2[4]);
		extern EError noiseInt64GenericC(Layer &layer1,int noise_scale,int seed,
			int octaves,float color1[4],float color2[4]);
		extern EError noiseFp128GenericC(Layer &layer1,int noise_scale,int seed,
			int octaves,float color1[4],float color2[4]);

		extern EError turbulenceInt32GenericC(Layer &layer1,int noise_scale,
			int seed,int octaves,float color1[4],float color2[4]);
		extern EError turbulenceInt64GenericC(Layer &layer1,int noise_scale,
			int seed,int octaves,float color1[4],float color2[4]);
		extern EError turbulenceFp128GenericC(Layer &layer1,int noise_scale,
			int seed,int octaves,float color1[4],float color2[4]);

		extern EError wave_xInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_xInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_xFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError wave_x_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_x_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_x_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError lwave_xInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_xInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_xFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError lwave_x_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_x_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_x_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError wave_yInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_yInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_yFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError wave_y_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_y_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError wave_y_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError lwave_yInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_yInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_yFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		extern EError lwave_y_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_y_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);
		extern EError lwave_y_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
			float amplitude,float phase,int nwaves);

		//distort l2 by vf stored in l3
		extern EError vf_distortInt32GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);
		extern EError vf_distortInt64GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);
		extern EError vf_distortFp128GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);

		extern EError vf_distort_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);
		extern EError vf_distort_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);
		extern EError vf_distort_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
			Layer &layer3,float scale_x,float scale_y);

		extern EError wave_x_vfInt32GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError wave_x_vfInt64GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError wave_x_vfFp128GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);

		extern EError lwave_x_vfInt32GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError lwave_x_vfInt64GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError lwave_x_vfFp128GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);

		extern EError wave_y_vfInt32GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError wave_y_vfInt64GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError wave_y_vfFp128GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);

		extern EError lwave_y_vfInt32GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError lwave_y_vfInt64GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);
		extern EError lwave_y_vfFp128GenericC(Layer &layer1,float amplitude,
			float phase,int nwaves);

		extern EError hsv_adjustInt32GenericC(Layer &layer1,Layer &layer2,
			float h_rotation_deg,float s_offset,float v_offset);
		extern EError hsv_adjustInt64GenericC(Layer &layer1,Layer &layer2,
			float h_rotation_deg,float s_offset,float v_offset);
		extern EError hsv_adjustFp128GenericC(Layer &layer1,Layer &layer2,
			float h_rotation_deg,float s_offset,float v_offset);

		extern EError grayscaleInt32GenericC(Layer &layer1,Layer &layer2);
		extern EError grayscaleInt64GenericC(Layer &layer1,Layer &layer2);
		extern EError grayscaleFp128GenericC(Layer &layer1,Layer &layer2);

		extern EError sobel_edge_detectInt32GenericC(Layer &layer1,Layer &layer2,float sample_spacing);
		extern EError sobel_edge_detectInt64GenericC(Layer &layer1,Layer &layer2,float sample_spacing);
		extern EError sobel_edge_detectFp128GenericC(Layer &layer1,Layer &layer2,float sample_spacing);

	};
};

#endif
