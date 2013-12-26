/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


/*
 *	A way of choosing between multiple implementations
 */

#include "IMPGenericCKernel.h"
#include "IMPMMXKernel.h"
#include "IMPSSE2Kernel.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace NIMPLib
{
	namespace NIMPLibSwInternal
	{
		typedef boost::function<EError (Layer&,Layer&)>				func_l2;
		typedef boost::function<EError (Layer&,Layer&,float)>		func_l2_f;
		typedef boost::function<EError (Layer&,Layer&,Layer&)>		func_l3;
		typedef boost::function<EError (Layer&,Layer&,float[4])>	func_l2_c;
		typedef boost::function<EError (Layer&,int,int,
			float[4],float[4])>										func_l_i2_c2;
		typedef boost::function<EError (Layer&,float,float,
			float,float[4],float[4])>								func_l_f3_c2;
		typedef boost::function<EError (Layer&,Layer&,float,
			float,int)>												func_l2_f2_i;
		typedef boost::function<EError (Layer&,float,float,int)>	func_l_f2_i;
		typedef boost::function<EError (Layer&,Layer&,Layer&,
			float,float)>											func_l3_f2;
		typedef boost::function<EError (Layer&,int,int,
			int,float[4],float[4])>									func_l_i3_c2;
		typedef boost::function<EError (Layer&,Layer&,float,
			float,float)>											func_l2_f3;

		extern EError generic_error(EError e);

		//dispatch maps:
		//every operation exposed in ExecutionEngine interface in fact has
		//num_formats*num_codepaths possible variants (which means 16 now)
		//dispatching cost is neglible (2D table lookup + boost::function call,
		//which should not cost more than simple call through function pointer
		//or virtual function call)
		const func_l3 add[format_count][code_path_count]={
			{&addInt32GenericC,&addInt32MMX,&addInt32MMX,&addInt32SSE2},
			{&addInt64GenericC,&addInt64MMX,&addInt64MMX,&addInt64SSE2},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&addFp128GenericC,&addFp128GenericC,&addFp128GenericC,&addFp128GenericC}};

		const func_l2_c add_color[format_count][code_path_count]={
			{&addColorInt32GenericC,&addColorInt32MMX,&addColorInt32MMX,&addColorInt32SSE2},
			{&addColorInt64GenericC,&addColorInt64MMX,&addColorInt64MMX,&addColorInt64SSE2},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&addColorFp128GenericC,&addColorFp128GenericC,&addColorFp128GenericC,&addColorFp128GenericC}};

		const func_l3 sub[format_count][code_path_count]={
			{&subInt32GenericC,&subInt32MMX,&subInt32MMX,&subInt32SSE2},
			{&subInt64GenericC,&subInt64MMX,&subInt64MMX,&subInt64SSE2},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&subFp128GenericC,&subFp128GenericC,&subFp128GenericC,&subFp128GenericC}};

		const func_l2_c sub_color[format_count][code_path_count]={
			{&subColorInt32GenericC,&subColorInt32MMX,&subColorInt32MMX,&subColorInt32SSE2},
			{&subColorInt64GenericC,&subColorInt64MMX,&subColorInt64MMX,&subColorInt64SSE2},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&subColorFp128GenericC,&subColorFp128GenericC,&subColorFp128GenericC,&subColorFp128GenericC}};

		const func_l3 mul[format_count][code_path_count]={
			{&mulInt32GenericC,&mulInt32GenericC,&mulInt32GenericC,&mulInt32GenericC},
			{&mulInt64GenericC,&mulInt64GenericC,&mulInt64GenericC,&mulInt64SSE2},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&mulFp128GenericC,&mulFp128GenericC,&mulFp128GenericC,&mulFp128GenericC}};

		const func_l2_c mul_color[format_count][code_path_count]={
			{&mulColorInt32GenericC,&mulColorInt32GenericC,&mulColorInt32GenericC,&mulColorInt32GenericC},
			{&mulColorInt64GenericC,&mulColorInt64GenericC,&mulColorInt64GenericC,&mulColorInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&mulColorFp128GenericC,&mulColorFp128GenericC,&mulColorFp128GenericC,&mulColorFp128GenericC}};

		const func_l3 signed_add[format_count][code_path_count]={
			{&signed_addInt32GenericC,&signed_addInt32GenericC,&signed_addInt32GenericC,&signed_addInt32GenericC},
			{&signed_addInt64GenericC,&signed_addInt64GenericC,&signed_addInt64GenericC,&signed_addInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&addFp128GenericC,&addFp128GenericC,&addFp128GenericC,&addFp128GenericC}};

		const func_l_i2_c2 checkers_aa[format_count][code_path_count]={
			{&checkers_aaInt32GenericC,&checkers_aaInt32GenericC,&checkers_aaInt32GenericC,&checkers_aaInt32GenericC},
			{&checkers_aaInt64GenericC,&checkers_aaInt64GenericC,&checkers_aaInt64GenericC,&checkers_aaInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&checkers_aaFp128GenericC,&checkers_aaFp128GenericC,&checkers_aaFp128GenericC,&checkers_aaFp128GenericC}};

		const func_l_f3_c2 rgradient_c[format_count][code_path_count]={
			{&rgradient_cInt32GenericC,&rgradient_cInt32GenericC,&rgradient_cInt32GenericC,&rgradient_cInt32GenericC},
			{&rgradient_cInt64GenericC,&rgradient_cInt64GenericC,&rgradient_cInt64GenericC,&rgradient_cInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&rgradient_cFp128GenericC,&rgradient_cFp128GenericC,&rgradient_cFp128GenericC,&rgradient_cFp128GenericC}};

		const func_l_f3_c2 rgradient_c_cubic[format_count][code_path_count]={
			{&rgradient_c_cubicInt32GenericC,&rgradient_c_cubicInt32GenericC,&rgradient_c_cubicInt32GenericC,&rgradient_c_cubicInt32GenericC},
			{&rgradient_c_cubicInt64GenericC,&rgradient_c_cubicInt64GenericC,&rgradient_c_cubicInt64GenericC,&rgradient_c_cubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&rgradient_c_cubicFp128GenericC,&rgradient_c_cubicFp128GenericC,&rgradient_c_cubicFp128GenericC,&rgradient_c_cubicFp128GenericC}};

		const func_l_f3_c2 rgradient_c_sin[format_count][code_path_count]={
			{&rgradient_c_sinInt32GenericC,&rgradient_c_sinInt32GenericC,&rgradient_c_sinInt32GenericC,&rgradient_c_sinInt32GenericC},
			{&rgradient_c_sinInt64GenericC,&rgradient_c_sinInt64GenericC,&rgradient_c_sinInt64GenericC,&rgradient_c_sinInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&rgradient_c_sinFp128GenericC,&rgradient_c_sinFp128GenericC,&rgradient_c_sinFp128GenericC,&rgradient_c_sinFp128GenericC}};

		const func_l_f3_c2 rgradient_e[format_count][code_path_count]={
			{&rgradient_eInt32GenericC,&rgradient_eInt32GenericC,&rgradient_eInt32GenericC,&rgradient_eInt32GenericC},
			{&rgradient_eInt64GenericC,&rgradient_eInt64GenericC,&rgradient_eInt64GenericC,&rgradient_eInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&rgradient_eFp128GenericC,&rgradient_eFp128GenericC,&rgradient_eFp128GenericC,&rgradient_eFp128GenericC}};

		const func_l_f3_c2 rgradient_e_sin[format_count][code_path_count]={
			{&rgradient_e_sinInt32GenericC,&rgradient_e_sinInt32GenericC,&rgradient_e_sinInt32GenericC,&rgradient_e_sinInt32GenericC},
			{&rgradient_e_sinInt64GenericC,&rgradient_e_sinInt64GenericC,&rgradient_e_sinInt64GenericC,&rgradient_e_sinInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&rgradient_e_sinFp128GenericC,&rgradient_e_sinFp128GenericC,&rgradient_e_sinFp128GenericC,&rgradient_e_sinFp128GenericC}};

		const func_l_i3_c2 noise[format_count][code_path_count]={
			{&noiseInt32GenericC,&noiseInt32GenericC,&noiseInt32GenericC,&noiseInt32GenericC},
			{&noiseInt64GenericC,&noiseInt64GenericC,&noiseInt64GenericC,&noiseInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&noiseFp128GenericC,&noiseFp128GenericC,&noiseFp128GenericC,&noiseFp128GenericC}};

		const func_l_i3_c2 turbulence[format_count][code_path_count]={
			{&turbulenceInt32GenericC,&turbulenceInt32GenericC,&turbulenceInt32GenericC,&turbulenceInt32GenericC},
			{&turbulenceInt64GenericC,&turbulenceInt64GenericC,&turbulenceInt64GenericC,&turbulenceInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&turbulenceFp128GenericC,&turbulenceFp128GenericC,&turbulenceFp128GenericC,&turbulenceFp128GenericC}};

		const func_l2_f2_i wave_x[format_count][code_path_count]={
			{&wave_xInt32GenericC,&wave_xInt32GenericC,&wave_xInt32GenericC,&wave_xInt32GenericC},
			{&wave_xInt64GenericC,&wave_xInt64GenericC,&wave_xInt64GenericC,&wave_xInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_xFp128GenericC,&wave_xFp128GenericC,&wave_xFp128GenericC,&wave_xFp128GenericC}};

		const func_l2_f2_i wave_x_bicubic[format_count][code_path_count]={
			{&wave_x_bicubicInt32GenericC,&wave_x_bicubicInt32GenericC,&wave_x_bicubicInt32GenericC,&wave_x_bicubicInt32GenericC},
			{&wave_x_bicubicInt64GenericC,&wave_x_bicubicInt64GenericC,&wave_x_bicubicInt64GenericC,&wave_x_bicubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_x_bicubicFp128GenericC,&wave_x_bicubicFp128GenericC,&wave_x_bicubicFp128GenericC,&wave_x_bicubicFp128GenericC}};

		const func_l2_f2_i lwave_x[format_count][code_path_count]={
			{&lwave_xInt32GenericC,&lwave_xInt32GenericC,&lwave_xInt32GenericC,&lwave_xInt32GenericC},
			{&lwave_xInt64GenericC,&lwave_xInt64GenericC,&lwave_xInt64GenericC,&lwave_xInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_xFp128GenericC,&lwave_xFp128GenericC,&lwave_xFp128GenericC,&lwave_xFp128GenericC}};

		const func_l2_f2_i lwave_x_bicubic[format_count][code_path_count]={
			{&lwave_x_bicubicInt32GenericC,&lwave_x_bicubicInt32GenericC,&lwave_x_bicubicInt32GenericC,&lwave_x_bicubicInt32GenericC},
			{&lwave_x_bicubicInt64GenericC,&lwave_x_bicubicInt64GenericC,&lwave_x_bicubicInt64GenericC,&lwave_x_bicubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_x_bicubicFp128GenericC,&lwave_x_bicubicFp128GenericC,&lwave_x_bicubicFp128GenericC,&lwave_x_bicubicFp128GenericC}};

		const func_l2_f2_i wave_y[format_count][code_path_count]={
			{&wave_yInt32GenericC,&wave_yInt32GenericC,&wave_yInt32GenericC,&wave_yInt32GenericC},
			{&wave_yInt64GenericC,&wave_yInt64GenericC,&wave_yInt64GenericC,&wave_yInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_yFp128GenericC,&wave_yFp128GenericC,&wave_yFp128GenericC,&wave_yFp128GenericC}};

		const func_l2_f2_i wave_y_bicubic[format_count][code_path_count]={
			{&wave_y_bicubicInt32GenericC,&wave_y_bicubicInt32GenericC,&wave_y_bicubicInt32GenericC,&wave_y_bicubicInt32GenericC},
			{&wave_y_bicubicInt64GenericC,&wave_y_bicubicInt64GenericC,&wave_y_bicubicInt64GenericC,&wave_y_bicubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_y_bicubicFp128GenericC,&wave_y_bicubicFp128GenericC,&wave_y_bicubicFp128GenericC,&wave_y_bicubicFp128GenericC}};

		const func_l2_f2_i lwave_y[format_count][code_path_count]={
			{&lwave_yInt32GenericC,&lwave_yInt32GenericC,&lwave_yInt32GenericC,&lwave_yInt32GenericC},
			{&lwave_yInt64GenericC,&lwave_yInt64GenericC,&lwave_yInt64GenericC,&lwave_yInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_yFp128GenericC,&lwave_yFp128GenericC,&lwave_yFp128GenericC,&lwave_yFp128GenericC}};

		const func_l2_f2_i lwave_y_bicubic[format_count][code_path_count]={
			{&lwave_y_bicubicInt32GenericC,&lwave_y_bicubicInt32GenericC,&lwave_y_bicubicInt32GenericC,&lwave_y_bicubicInt32GenericC},
			{&lwave_y_bicubicInt64GenericC,&lwave_y_bicubicInt64GenericC,&lwave_y_bicubicInt64GenericC,&lwave_y_bicubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_y_bicubicFp128GenericC,&lwave_y_bicubicFp128GenericC,&lwave_y_bicubicFp128GenericC,&lwave_y_bicubicFp128GenericC}};

		const func_l3_f2 vf_distort[format_count][code_path_count]={
			{&vf_distortInt32GenericC,&vf_distortInt32GenericC,&vf_distortInt32GenericC,&vf_distortInt32GenericC},
			{&vf_distortInt64GenericC,&vf_distortInt64GenericC,&vf_distortInt64GenericC,&vf_distortInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&vf_distortFp128GenericC,&vf_distortFp128GenericC,&vf_distortFp128GenericC,&vf_distortFp128GenericC}};

		const func_l3_f2 vf_distort_bicubic[format_count][code_path_count]={
			{&vf_distort_bicubicInt32GenericC,&vf_distort_bicubicInt32GenericC,&vf_distort_bicubicInt32GenericC,&vf_distort_bicubicInt32GenericC},
			{&vf_distort_bicubicInt64GenericC,&vf_distort_bicubicInt64GenericC,&vf_distort_bicubicInt64GenericC,&vf_distort_bicubicInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&vf_distort_bicubicFp128GenericC,&vf_distort_bicubicFp128GenericC,&vf_distort_bicubicFp128GenericC,&vf_distort_bicubicFp128GenericC}};

		const func_l_f2_i wave_x_vf[format_count][code_path_count]={
			{&wave_x_vfInt32GenericC,&wave_x_vfInt32GenericC,&wave_x_vfInt32GenericC,&wave_x_vfInt32GenericC},
			{&wave_x_vfInt64GenericC,&wave_x_vfInt64GenericC,&wave_x_vfInt64GenericC,&wave_x_vfInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_x_vfFp128GenericC,&wave_x_vfFp128GenericC,&wave_x_vfFp128GenericC,&wave_x_vfFp128GenericC}};

		const func_l_f2_i lwave_x_vf[format_count][code_path_count]={
			{&lwave_x_vfInt32GenericC,&lwave_x_vfInt32GenericC,&lwave_x_vfInt32GenericC,&lwave_x_vfInt32GenericC},
			{&lwave_x_vfInt64GenericC,&lwave_x_vfInt64GenericC,&lwave_x_vfInt64GenericC,&lwave_x_vfInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_x_vfFp128GenericC,&lwave_x_vfFp128GenericC,&lwave_x_vfFp128GenericC,&lwave_x_vfFp128GenericC}};

		const func_l_f2_i wave_y_vf[format_count][code_path_count]={
			{&wave_y_vfInt32GenericC,&wave_y_vfInt32GenericC,&wave_y_vfInt32GenericC,&wave_y_vfInt32GenericC},
			{&wave_y_vfInt64GenericC,&wave_y_vfInt64GenericC,&wave_y_vfInt64GenericC,&wave_y_vfInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&wave_y_vfFp128GenericC,&wave_y_vfFp128GenericC,&wave_y_vfFp128GenericC,&wave_y_vfFp128GenericC}};

		const func_l_f2_i lwave_y_vf[format_count][code_path_count]={
			{&lwave_y_vfInt32GenericC,&lwave_y_vfInt32GenericC,&lwave_y_vfInt32GenericC,&lwave_y_vfInt32GenericC},
			{&lwave_y_vfInt64GenericC,&lwave_y_vfInt64GenericC,&lwave_y_vfInt64GenericC,&lwave_y_vfInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&lwave_y_vfFp128GenericC,&lwave_y_vfFp128GenericC,&lwave_y_vfFp128GenericC,&lwave_y_vfFp128GenericC}};

		const func_l2_f3 hsv_adjust[format_count][code_path_count]={
			{&hsv_adjustInt32GenericC,&hsv_adjustInt32GenericC,&hsv_adjustInt32GenericC,&hsv_adjustInt32GenericC},
			{&hsv_adjustInt64GenericC,&hsv_adjustInt64GenericC,&hsv_adjustInt64GenericC,&hsv_adjustInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&hsv_adjustFp128GenericC,&hsv_adjustFp128GenericC,&hsv_adjustFp128GenericC,&hsv_adjustFp128GenericC}};

		const func_l2 grayscale[format_count][code_path_count]={
			{&grayscaleInt32GenericC,&grayscaleInt32GenericC,&grayscaleInt32GenericC,&grayscaleInt32GenericC},
			{&grayscaleInt64GenericC,&grayscaleInt64GenericC,&grayscaleInt64GenericC,&grayscaleInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&grayscaleFp128GenericC,&grayscaleFp128GenericC,&grayscaleFp128GenericC,&grayscaleFp128GenericC}};

		const func_l2_f sobel_edge_detect[format_count][code_path_count]={
			{&sobel_edge_detectInt32GenericC,&sobel_edge_detectInt32GenericC,&sobel_edge_detectInt32GenericC,&sobel_edge_detectInt32GenericC},
			{&sobel_edge_detectInt64GenericC,&sobel_edge_detectInt64GenericC,&sobel_edge_detectInt64GenericC,&sobel_edge_detectInt64GenericC},
			{boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat),
				boost::bind(&generic_error,eeUnsupportedFormat)},
			{&sobel_edge_detectFp128GenericC,&sobel_edge_detectFp128GenericC,&sobel_edge_detectFp128GenericC,&sobel_edge_detectFp128GenericC}};

	}
}
