/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


/*
 *	Implementation file for generic CPU based computation kernel
 */

#include "stdafx.h"
#include "IMPLibSw.h"
#include "IMPGenericCKernel.h"
#include <math.h>
#include <assert.h>
#include <boost/utility.hpp>

using namespace NIMPLib;
using namespace NIMPLibSwInternal;

static bool equalL2(Layer &layer1,Layer &layer2)
{
	return (layer1.desc==layer2.desc);
}

static bool equalL3(Layer &layer1,Layer &layer2,Layer &layer3)
{
	return ((layer1.desc==layer2.desc) &&
			(layer2.desc==layer3.desc));
}

static inline unsigned char convertColorToInt32Component(float in)
{
	if(in<0.0f)
	{
		return 0;
	}
	else if(in>1.0f)
	{
		return 255;
	}
	return (unsigned char)(in*255.0f);
}

static inline void convertColorToInt32(float in[4],unsigned char out[4])
{
	int i=0;
	for(;i<4;i++)
	{
		out[i]=convertColorToInt32Component(in[i]);
	}
}

static inline unsigned short convertColorToInt64Component(float in)
{
	if(in<0.0f)
	{
		return 0;
	}
	else if(in>1.0f)
	{
		return 65535;
	}
	return (unsigned short)(in*65535.0f);
}

static inline void convertColorToInt64(float in[4],unsigned short out[4])
{
	int i=0;
	for(;i<4;i++)
	{
		out[i]=convertColorToInt64Component(in[i]);
	}
}

static inline float frc(float v)
{
	return (v-floorf(v));
}

template<typename T>
struct output_float
{
	inline T operator()(float in)
	{
		return in;
	}
};

template<>
struct output_float<unsigned char>
{
	inline unsigned char operator()(float in)
	{
		if(in<0.0f)
		{
			return 0;
		}
		else if(in>1.0f)
		{
			return 255;
		}
		return (unsigned char)(in*255.0f);
	}
};

template<>
struct output_float<unsigned short>
{
	inline unsigned short operator()(float in)
	{
		if(in<0.0f)
		{
			return 0;
		}
		else if(in>1.0f)
		{
			return 65535;
		}
		return (unsigned short)(in*65535.0f);
	}
};

template<typename T>
struct output_float_signed
{
	inline T operator()(float in)
	{
		return in;
	}
};

template<>
struct output_float_signed<unsigned char>
{
	//signed output maps to 0..254 for unsigned char in order
	//to get accurate representation of 0 (as 127)
	inline unsigned char operator()(float in)
	{
		float v=127.0f*(in+1.0f);
		if(v<0.0f)
		{
			return 0;
		}
		else if(v>254.0f)
		{
			return 254;
		}
		return (unsigned char)(v);
	}
};

template<>
struct output_float_signed<unsigned short>
{
	inline unsigned short operator()(float in)
	{
		float v=32767.0f*(in+1.0f);
		if(v<0.0f)
		{
			return 0;
		}
		else if(v>65534.0f)
		{
			return 65534;
		}
		return (unsigned short)(v);
	}
};

template<typename T>
struct input_float
{
	inline float operator()(T in)
	{
		return in;
	}
};

static const float inv_255=1.0f/255.0f;

template<>
struct input_float<unsigned char>
{
	inline float operator()(unsigned char in)
	{
		return in*inv_255;
	}
};

static const float inv_65535=1.0f/65535.0f;

template<>
struct input_float<unsigned short>
{
	inline float operator()(unsigned short in)
	{
		return in*inv_65535;
	}
};

template<typename T>
struct input_float_signed
{
	inline float operator()(T in)
	{
		return in;
	}
};

static const float inv_127=1.0f/127.0f;

template<>
struct input_float_signed<unsigned char>
{
	inline float operator()(unsigned char in)
	{
		return in*inv_127-1.0f;
	}
};

static const float inv_32767=1.0f/32767.0f;

template<>
struct input_float_signed<unsigned short>
{
	inline float operator()(unsigned short in)
	{
		return in*inv_32767-1.0f;
	}
};

struct weight_mapper_linear
{
	inline float operator()(float t)
	{
		return t;
	}
};

struct weight_mapper_linear_clamp_up
{
	inline float operator()(float t)
	{
		if(t>1.0f)
		{
			return 1.0f;
		}
		return t;
	}
};

struct weight_mapper_cubic
{
	inline float operator()(float t)
	{
		return t*t*(3.0f-2.0f*t);
	}
};

struct weight_mapper_cubic_clamp_up
{
	inline float operator()(float t)
	{
		if(t>1.0f)
		{
			return 1.0f;
		}
		return t*t*(3.0f-2.0f*t);
	}
};

const float pi=3.14159f;

struct weight_mapper_sin
{
public:
	inline float operator()(float t)
	{
		return 0.5f*(1.0f+sinf(pi*(t-0.5f)));
	}
};

struct weight_mapper_sin_clamp_up
{
public:
	inline float operator()(float t)
	{
		if(t>1.0f)
		{
			return 1.0f;
		}
		return 0.5f*(1.0f+sinf(pi*(t-0.5f)));
	}
};

//all texture sampling operations match Direct3D texel addessing rules
//and use 'repeat' border mode
template<typename T>
struct texture_sampler_nearest
{
private:
	const DWORD	m_dwSizeX;
	const DWORD	m_dwSizeY;
	const float	m_fSizeX;
	const float	m_fSizeY;
	const T* in_tx_base;
public:
	texture_sampler_nearest(const Layer& in) :
	m_dwSizeX(in.desc.dwSizeX),
	m_dwSizeY(in.desc.dwSizeY),
	m_fSizeX((float)m_dwSizeX),
	m_fSizeY((float)m_dwSizeY),
	in_tx_base((T*)in.byte_data)
	{
	}
	inline void operator()(T* restrict_ptr out,float x,float y)
	{
		unsigned short tx=(unsigned short)((m_fSizeX*frc(x))-0.5f);
		unsigned short ty=(unsigned short)((m_fSizeY*frc(y))-0.5f);
		assert(tx<m_dwSizeX);
		assert(ty<m_dwSizeY);
		const T* restrict_ptr in_tx=in_tx_base+((ty*m_dwSizeX+tx)<<2);
		*out++=*in_tx++;
		*out++=*in_tx++;
		*out++=*in_tx++;
		*out++=*in_tx++;
	}
};

template<typename T>
struct texture_sampler_bilinear : private boost::noncopyable
{
private:
	const DWORD		m_dwSizeX;
	const DWORD		m_dwSizeY;
	const float		m_fSizeX;
	const float		m_fSizeY;
	const T*		in_tx_base;
	input_float<T>	input_mapper;
	output_float<T>	output_mapper;
public:
	texture_sampler_bilinear(const Layer& in) :
	m_dwSizeX(in.desc.dwSizeX),
	m_dwSizeY(in.desc.dwSizeY),
	m_fSizeX((float)m_dwSizeX),
	m_fSizeY((float)m_dwSizeY),
	in_tx_base((T*)in.byte_data)
	{
	}
	inline void operator()(T* restrict_ptr out,float x,float y)
	{
		float fx=(m_fSizeX*frc(x))-0.5f;
		float fy=(m_fSizeY*frc(y))-0.5f;

		unsigned short tx=(unsigned short)(fx);
		unsigned short ty=(unsigned short)(fy);
		assert(tx<m_dwSizeX);
		assert(ty<m_dwSizeY);

		float weight_x=frc(fx);
		float weight_y=frc(fy);

		const T* restrict_ptr in_tx=in_tx_base;
		const T* restrict_ptr tl=in_tx+((ty*m_dwSizeX+tx)<<2);
		const T* restrict_ptr tr=in_tx+((ty*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);
		const T* restrict_ptr bl=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+tx)<<2);
		const T* restrict_ptr br=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);

		float weight_x_n=1.0f-weight_x;
		float weight_y_n=1.0f-weight_y;

		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
	}
};

template<typename T>
struct texture_sampler_bilinear_pow2 : private boost::noncopyable
{
private:
	const DWORD		m_dwSizeX;
	const DWORD		m_dwSizeY;
	const float		m_fSizeX;
	const float		m_fSizeY;
	const T*		in_tx_base;
	const unsigned int	x_size_shift;
	const unsigned int	x_size_mask;
	const unsigned int	y_size_shift;
	const unsigned int	y_size_mask;
	input_float<T>	input_mapper;
	output_float<T>	output_mapper;
	unsigned int calc_shift(unsigned int v)
	{
		assert((v & (v-1))==0);
		unsigned int ret=0;
		while(v!=1)
		{
			v>>=1;
			ret++;
		}
		return ret;
	}
	unsigned int calc_mask(unsigned int v)
	{
		assert((v & (v-1))==0);
		return (v-1);
	}
public:
	texture_sampler_bilinear_pow2(const Layer& in) :
	m_dwSizeX(in.desc.dwSizeX),
	m_dwSizeY(in.desc.dwSizeY),
	x_size_shift(calc_shift(m_dwSizeX)),
	x_size_mask(calc_mask(m_dwSizeX)),
	y_size_shift(calc_shift(m_dwSizeY)),
	y_size_mask(calc_mask(m_dwSizeY)),
	m_fSizeX((float)m_dwSizeX),
	m_fSizeY((float)m_dwSizeY),
	in_tx_base((T*)in.byte_data)
	{
	}
	inline void operator()(T* restrict_ptr out,float x,float y)
	{
		float fx=(m_fSizeX*frc(x))-0.5f;
		float fy=(m_fSizeY*frc(y))-0.5f;

		unsigned int tx=(unsigned int)(fx);
		unsigned int ty=(unsigned int)(fy);
		assert(tx<m_dwSizeX);
		assert(ty<m_dwSizeY);

		float weight_x=frc(fx);
		float weight_y=frc(fy);

		const T* restrict_ptr in_tx=in_tx_base;
		const T* restrict_ptr tl=in_tx+(((ty<<x_size_shift)+tx)<<2);
		const T* restrict_ptr tr=in_tx+(((ty<<x_size_shift)+((tx+1)&x_size_mask))<<2);
		const T* restrict_ptr bl=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+tx)<<2);
		const T* restrict_ptr br=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+((tx+1)&x_size_mask))<<2);

		float weight_x_n=1.0f-weight_x;
		float weight_y_n=1.0f-weight_y;

		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
		*out++=output_mapper(
			(input_mapper(*tl++)*weight_x_n+
				input_mapper(*tr++)*weight_x)*weight_y_n+
			(input_mapper(*bl++)*weight_x_n+
				input_mapper(*br++)*weight_x)*weight_y);
	}
};

struct catmull_rom
{
	inline float operator()(float s,float v1,float v2,float v3,float v4)
	{
		//return 0.5f*((2*v2)+
		//	(-v1+v2)*s+
		//	(2*v1-5*v2+4*v3-v4)*s*s+
		//	(-v1+3*v2-3*v3+v4)*s*s*s);
		return ((((-v1+3*v2-3*v3+v4)*s+
			(2*v1-5*v2+4*v3-v4))*s+
			(-v1+v3))*s)*0.5f+v2;
	}
};

template<typename T>
struct texture_sampler_bicubic : private boost::noncopyable
{
private:
	const DWORD		m_dwSizeX;
	const DWORD		m_dwSizeY;
	const float		m_fSizeX;
	const float		m_fSizeY;
	const T*		in_tx_base;
	input_float<T>	input_mapper;
	output_float<T>	output_mapper;
	catmull_rom		quad_val_interpolator;
public:
	texture_sampler_bicubic(const Layer& in) :
	m_dwSizeX(in.desc.dwSizeX),
	m_dwSizeY(in.desc.dwSizeY),
	m_fSizeX((float)m_dwSizeX),
	m_fSizeY((float)m_dwSizeY),
	in_tx_base((T*)in.byte_data)
	{
	}
	inline void operator()(T* restrict_ptr out,float x,float y)
	{
		float fx=(m_fSizeX*(frc(x)+1.0f))-0.5f;
		float fy=(m_fSizeY*(frc(y)+1.0f))-0.5f;

		unsigned short tx=(unsigned short)(fx);
		unsigned short ty=(unsigned short)(fy);
		assert(tx<m_dwSizeX);
		assert(ty<m_dwSizeY);

		//float weight_x=(frc(fx)+1.0f)*0.33333f;
		//float weight_y=(frc(fy)+1.0f)*0.33333f;
		float weight_x=frc(fx);
		float weight_y=frc(fy);

		const T* restrict_ptr in_tx=in_tx_base;

		const T* restrict_ptr t11=in_tx+((((ty-1)%m_dwSizeY)*m_dwSizeX+(tx-1)%m_dwSizeX)<<2);
		const T* restrict_ptr t12=in_tx+((((ty-1)%m_dwSizeY)*m_dwSizeX+(tx)%m_dwSizeX)<<2);
		const T* restrict_ptr t13=in_tx+((((ty-1)%m_dwSizeY)*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);
		const T* restrict_ptr t14=in_tx+((((ty-1)%m_dwSizeY)*m_dwSizeX+(tx+2)%m_dwSizeX)<<2);

		const T* restrict_ptr t21=in_tx+((((ty)%m_dwSizeY)*m_dwSizeX+(tx-1)%m_dwSizeX)<<2);
		const T* restrict_ptr t22=in_tx+((((ty)%m_dwSizeY)*m_dwSizeX+(tx)%m_dwSizeX)<<2);
		const T* restrict_ptr t23=in_tx+((((ty)%m_dwSizeY)*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);
		const T* restrict_ptr t24=in_tx+((((ty)%m_dwSizeY)*m_dwSizeX+(tx+2)%m_dwSizeX)<<2);

		const T* restrict_ptr t31=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+(tx-1)%m_dwSizeX)<<2);
		const T* restrict_ptr t32=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+(tx)%m_dwSizeX)<<2);
		const T* restrict_ptr t33=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);
		const T* restrict_ptr t34=in_tx+((((ty+1)%m_dwSizeY)*m_dwSizeX+(tx+2)%m_dwSizeX)<<2);

		const T* restrict_ptr t41=in_tx+((((ty+2)%m_dwSizeY)*m_dwSizeX+(tx-1)%m_dwSizeX)<<2);
		const T* restrict_ptr t42=in_tx+((((ty+2)%m_dwSizeY)*m_dwSizeX+(tx)%m_dwSizeX)<<2);
		const T* restrict_ptr t43=in_tx+((((ty+2)%m_dwSizeY)*m_dwSizeX+(tx+1)%m_dwSizeX)<<2);
		const T* restrict_ptr t44=in_tx+((((ty+2)%m_dwSizeY)*m_dwSizeX+(tx+2)%m_dwSizeX)<<2);

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));
	}
};

template<typename T>
struct texture_sampler_bicubic_pow2 : private boost::noncopyable
{
private:
	const DWORD		m_dwSizeX;
	const DWORD		m_dwSizeY;
	const float		m_fSizeX;
	const float		m_fSizeY;
	const T*		in_tx_base;
	const unsigned int	x_size_shift;
	const unsigned int	x_size_mask;
	const unsigned int	y_size_shift;
	const unsigned int	y_size_mask;
	input_float<T>	input_mapper;
	output_float<T>	output_mapper;
	catmull_rom		quad_val_interpolator;
	unsigned int calc_shift(unsigned int v)
	{
		assert((v & (v-1))==0);
		unsigned int ret=0;
		while(v!=1)
		{
			v>>=1;
			ret++;
		}
		return ret;
	}
	unsigned int calc_mask(unsigned int v)
	{
		assert((v & (v-1))==0);
		return (v-1);
	}
public:
	texture_sampler_bicubic_pow2(const Layer& in) :
	m_dwSizeX(in.desc.dwSizeX),
	m_dwSizeY(in.desc.dwSizeY),
	x_size_shift(calc_shift(m_dwSizeX)),
	x_size_mask(calc_mask(m_dwSizeX)),
	y_size_shift(calc_shift(m_dwSizeY)),
	y_size_mask(calc_mask(m_dwSizeY)),
	m_fSizeX((float)m_dwSizeX),
	m_fSizeY((float)m_dwSizeY),
	in_tx_base((T*)in.byte_data)
	{
	}
	inline void operator()(T* restrict_ptr out,float x,float y)
	{
		float fx=(m_fSizeX*(frc(x)+1.0f))-0.5f;
		float fy=(m_fSizeY*(frc(y)+1.0f))-0.5f;

		unsigned short tx=(unsigned short)(fx);
		unsigned short ty=(unsigned short)(fy);

		//float weight_x=(frc(fx)+1.0f)*0.33333f;
		//float weight_y=(frc(fy)+1.0f)*0.33333f;
		float weight_x=frc(fx);
		float weight_y=frc(fy);

		const T* restrict_ptr in_tx=in_tx_base;

		const T* restrict_ptr t11=in_tx+(((((ty-1)&y_size_mask)<<x_size_shift)+((tx-1)&x_size_mask))<<2);
		const T* restrict_ptr t12=in_tx+(((((ty-1)&y_size_mask)<<x_size_shift)+((tx)&x_size_mask))<<2);
		const T* restrict_ptr t13=in_tx+(((((ty-1)&y_size_mask)<<x_size_shift)+((tx+1)&x_size_mask))<<2);
		const T* restrict_ptr t14=in_tx+(((((ty-1)&y_size_mask)<<x_size_shift)+((tx+2)&x_size_mask))<<2);

		const T* restrict_ptr t21=in_tx+(((((ty)&y_size_mask)<<x_size_shift)+((tx-1)&x_size_mask))<<2);
		const T* restrict_ptr t22=in_tx+(((((ty)&y_size_mask)<<x_size_shift)+((tx)&x_size_mask))<<2);
		const T* restrict_ptr t23=in_tx+(((((ty)&y_size_mask)<<x_size_shift)+((tx+1)&x_size_mask))<<2);
		const T* restrict_ptr t24=in_tx+(((((ty)&y_size_mask)<<x_size_shift)+((tx+2)&x_size_mask))<<2);

		const T* restrict_ptr t31=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+((tx-1)&x_size_mask))<<2);
		const T* restrict_ptr t32=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+((tx)&x_size_mask))<<2);
		const T* restrict_ptr t33=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+((tx+1)&x_size_mask))<<2);
		const T* restrict_ptr t34=in_tx+(((((ty+1)&y_size_mask)<<x_size_shift)+((tx+2)&x_size_mask))<<2);

		const T* restrict_ptr t41=in_tx+(((((ty+2)&y_size_mask)<<x_size_shift)+((tx-1)&x_size_mask))<<2);
		const T* restrict_ptr t42=in_tx+(((((ty+2)&y_size_mask)<<x_size_shift)+((tx)&x_size_mask))<<2);
		const T* restrict_ptr t43=in_tx+(((((ty+2)&y_size_mask)<<x_size_shift)+((tx+1)&x_size_mask))<<2);
		const T* restrict_ptr t44=in_tx+(((((ty+2)&y_size_mask)<<x_size_shift)+((tx+2)&x_size_mask))<<2);

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));

		*out++=output_mapper(quad_val_interpolator(weight_y,
			quad_val_interpolator(weight_x,
				input_mapper(*t11++),input_mapper(*t12++),
				input_mapper(*t13++),input_mapper(*t14++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t21++),input_mapper(*t22++),
				input_mapper(*t23++),input_mapper(*t24++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t31++),input_mapper(*t32++),
				input_mapper(*t33++),input_mapper(*t34++)),
			quad_val_interpolator(weight_x,
				input_mapper(*t41++),input_mapper(*t42++),
				input_mapper(*t43++),input_mapper(*t44++))));
	}
};

static bool is_layer_pow2(Layer& l)
{
	return (((l.desc.dwSizeX&(l.desc.dwSizeX-1))==0) &&
			((l.desc.dwSizeY&(l.desc.dwSizeY-1))==0));
}
/*
 *	add
 */

EError NIMPLibSwInternal::addInt32GenericC(Layer &layer1,Layer &layer2,Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int uiTmp=0;
	unsigned int i=0;
	unsigned char* restrict_ptr pSrc1=layer2.byte_data;
	unsigned char* restrict_ptr pSrc2=layer3.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nElems;i++)
	{
		uiTmp=(*pSrc1++)+(*pSrc2++);
		if(uiTmp>255)
		{	//saturate
			uiTmp=255;
		}
		*pDest++=(BYTE)uiTmp;
	}
	return eeOk;
}

EError NIMPLibSwInternal::addInt64GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int uiTmp=0;
	unsigned int i=0;
	unsigned short* restrict_ptr pSrc1=layer2.ushort_data;
	unsigned short* restrict_ptr pSrc2=layer3.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nElems;i++)
	{
		uiTmp=(*pSrc1++)+(*pSrc2++);
		if(uiTmp>65535)
		{	//saturate
			uiTmp=65535;
		}
		*pDest++=(unsigned short)uiTmp;
	}
	return eeOk;
}

EError NIMPLibSwInternal::addFp128GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int i=0;
	float* restrict_ptr pSrc1=layer2.float_data;
	float* restrict_ptr pSrc2=layer3.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nElems;i++)
	{
		*pDest++=(*pSrc1++)+(*pSrc2++);
	}
	return eeOk;
}

/*
 * add color
 */

EError NIMPLibSwInternal::addColorInt32GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	BYTE clamped_col[4];
	convertColorToInt32(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int uiTmp=0;
	unsigned int i=0;
	unsigned int j=0;
	unsigned char* restrict_ptr pSrc=layer2.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			uiTmp=(*pSrc++)+clamped_col[j];
			if(uiTmp>255)
			{	//saturate
				uiTmp=255;
			}
			*pDest++=(BYTE)uiTmp;
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::addColorInt64GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned short clamped_col[4];
	convertColorToInt64(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int uiTmp=0;
	unsigned int i=0;
	unsigned int j=0;
	unsigned short* restrict_ptr pSrc=layer2.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			uiTmp=(*pSrc++)+clamped_col[j];
			if(uiTmp>65535)
			{	//saturate
				uiTmp=65535;
			}
			*pDest++=(unsigned short)uiTmp;
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::addColorFp128GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int i=0;
	unsigned int j=0;
	float* restrict_ptr pSrc=layer2.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			*pDest++=(*pSrc++)+color[j];
		}
	}
	return eeOk;
}

/*
 *	sub
 */

EError NIMPLibSwInternal::subInt32GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	int iTmp=0;
	unsigned int i=0;
	unsigned char* restrict_ptr pSrc1=layer2.byte_data;
	unsigned char* restrict_ptr pSrc2=layer3.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nElems;i++)
	{
		iTmp=(*pSrc1++)-(*pSrc2++);
		if(iTmp<0)
		{	//saturate
			iTmp=0;
		}
		*pDest++=(BYTE)iTmp;
	}
	return eeOk;
}

EError NIMPLibSwInternal::subInt64GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	int iTmp=0;
	unsigned int i=0;
	unsigned short* restrict_ptr pSrc1=layer2.ushort_data;
	unsigned short* restrict_ptr pSrc2=layer3.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nElems;i++)
	{
		iTmp=(*pSrc1++)-(*pSrc2++);
		if(iTmp<0)
		{	//saturate
			iTmp=0;
		}
		*pDest++=(unsigned short)iTmp;
	}
	return eeOk;
}

EError NIMPLibSwInternal::subFp128GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int i=0;
	float* restrict_ptr pSrc1=layer2.float_data;
	float* restrict_ptr pSrc2=layer3.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nElems;i++)
	{
		*pDest++=(*pSrc1++)-(*pSrc2++);
	}
	return eeOk;
}

/*
 * sub color
 */

EError NIMPLibSwInternal::subColorInt32GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	BYTE clamped_col[4];
	convertColorToInt32(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	int iTmp=0;
	unsigned int i=0;
	unsigned int j=0;
	unsigned char* restrict_ptr pSrc=layer2.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			iTmp=(*pSrc++)-clamped_col[j];
			if(iTmp<0)
			{	//saturate
				iTmp=0;
			}
			*pDest++=(BYTE)iTmp;
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::subColorInt64GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned short clamped_col[4];
	convertColorToInt64(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	int iTmp=0;
	unsigned int i=0;
	unsigned int j=0;
	unsigned short* restrict_ptr pSrc=layer2.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			iTmp=(*pSrc++)-clamped_col[j];
			if(iTmp<0)
			{	//saturate
				iTmp=0;
			}
			*pDest++=(unsigned short)iTmp;
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::subColorFp128GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int i=0;
	unsigned int j=0;
	float* restrict_ptr pSrc=layer2.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			*pDest++=(*pSrc++)-color[j];
		}
	}
	return eeOk;
}

/*
 *	mul
 */

EError NIMPLibSwInternal::mulInt32GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int i=0;
	unsigned char* restrict_ptr pSrc1=layer2.byte_data;
	unsigned char* restrict_ptr pSrc2=layer3.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nElems;i++)
	{
		*pDest++=(BYTE)(((*pSrc1++)*(*pSrc2++))>>8);
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulInt64GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int i=0;
	unsigned short* restrict_ptr pSrc1=layer2.ushort_data;
	unsigned short* restrict_ptr pSrc2=layer3.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nElems;i++)
	{
		*pDest++=(unsigned short)(((*pSrc1++)*(*pSrc2++))>>16);
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulFp128GenericC(Layer &layer1,Layer &layer2,
										   Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	unsigned int i=0;
	float* restrict_ptr pSrc1=layer2.float_data;
	float* restrict_ptr pSrc2=layer3.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nElems;i++)
	{
		*pDest++=(*pSrc1++)*(*pSrc2++);
	}
	return eeOk;
}

/*
 * mul color
 */

EError NIMPLibSwInternal::mulColorInt32GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	BYTE clamped_col[4];
	convertColorToInt32(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int i=0;
	unsigned int j=0;
	unsigned char* restrict_ptr pSrc=layer2.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			*pDest++=(BYTE)(((*pSrc++)*(clamped_col[j]))>>8);
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulColorInt64GenericC(Layer &layer1,Layer &layer2,
												float color[4])
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned short clamped_col[4];
	convertColorToInt64(color,clamped_col);
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int i=0;
	unsigned int j=0;
	unsigned short* restrict_ptr pSrc=layer2.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			*pDest++=(unsigned short)(((*pSrc++)*(clamped_col[j]))>>16);
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::mulColorFp128GenericC(Layer &layer1,Layer &layer2,float color[4]) {
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nPixels=layer1.desc.dwSizeX*layer1.desc.dwSizeY;
	unsigned int i=0;
	unsigned int j=0;
	float* restrict_ptr pSrc=layer2.float_data;
	float* restrict_ptr pDest=layer1.float_data;
	for(i=0;i<nPixels;i++)
	{
		for(j=0;j<4;j++)
		{
			*pDest++=(*pSrc++)*color[j];
		}
	}
	return eeOk;
}

EError NIMPLibSwInternal::signed_addInt32GenericC(Layer &layer1,Layer &layer2,
												  Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	int iTmp=0;
	unsigned int i=0;
	unsigned char* restrict_ptr pSrc1=layer2.byte_data;
	unsigned char* restrict_ptr pSrc2=layer3.byte_data;
	unsigned char* restrict_ptr pDest=layer1.byte_data;
	for(i=0;i<nElems;i++)
	{
		iTmp=(*pSrc1++)+(*pSrc2++)-127;
		if(iTmp<0)
		{
			iTmp=0;
		}
		if(iTmp>255)
		{
			iTmp=255;
		}
		*pDest++=(unsigned char)iTmp;
	}
	return eeOk;
}

EError NIMPLibSwInternal::signed_addInt64GenericC(Layer &layer1,Layer &layer2,
												  Layer &layer3)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	unsigned int nElems=layer1.desc.dwSizeX*layer1.desc.dwSizeY*4;
	int iTmp=0;
	unsigned int i=0;
	unsigned short* restrict_ptr pSrc1=layer2.ushort_data;
	unsigned short* restrict_ptr pSrc2=layer3.ushort_data;
	unsigned short* restrict_ptr pDest=layer1.ushort_data;
	for(i=0;i<nElems;i++)
	{
		iTmp=(*pSrc1++)+(*pSrc2++)-32767;
		if(iTmp<0)
		{
			iTmp=0;
		}
		if(iTmp>65535)
		{
			iTmp=65535;
		}
		*pDest++=(unsigned short)iTmp;
	}
	return eeOk;
}

template<typename T,typename WeightMapper>
void checkers_aa_t(Layer &layer1,int divisions_x,int divisions_y,
				   float color_odd[4],float color_even[4])
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float pixel_size_x=(float)divisions_x/(float)size_x;
	float pixel_size_y=(float)divisions_y/(float)size_y;
	float pixel_area_inv=1.0f/(pixel_size_x*pixel_size_y);
	output_float<T>		out_mapper;
	WeightMapper		weight_mapper;
	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			float fcoord_x=j*pixel_size_x;
			float fcoord_y=i*pixel_size_y;
			int odd_even_choose=((int)(fcoord_x)+(int)(fcoord_y))%2;
			float xdiv=0.0f;
			if(floorf(fcoord_x+pixel_size_x)>floorf(fcoord_x))
			{
				xdiv=frc(fcoord_x+pixel_size_x);
			}
			float ydiv=0.0f;
			if(floorf(fcoord_y+pixel_size_y)>floorf(fcoord_y))
			{
				ydiv=frc(fcoord_y+pixel_size_y);
			}
			float my_weight=((pixel_size_x-xdiv)*(pixel_size_y-ydiv)+xdiv*ydiv)*
							pixel_area_inv;
			if(odd_even_choose)
			{
				my_weight=1.0f-my_weight;
			}
			my_weight=weight_mapper(my_weight);
			float my_weight_n=1.0f-my_weight;
			*pDest++=out_mapper(
				my_weight*color_odd[0]+(my_weight_n)*color_even[0]);
			*pDest++=out_mapper(
				my_weight*color_odd[1]+(my_weight_n)*color_even[1]);
			*pDest++=out_mapper(
				my_weight*color_odd[2]+(my_weight_n)*color_even[2]);
			*pDest++=out_mapper(
				my_weight*color_odd[3]+(my_weight_n)*color_even[3]);
		}
	}
}

EError NIMPLibSwInternal::checkers_aaInt32GenericC(Layer &layer1,
												   int divisions_x,
												   int divisions_y,
												   float color_odd[4],
												   float color_even[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	checkers_aa_t<unsigned char,weight_mapper_linear>(layer1,divisions_x,divisions_y,
		color_odd,color_even);
	return eeOk;
}

EError NIMPLibSwInternal::checkers_aaInt64GenericC(Layer &layer1,
												   int divisions_x,
												   int divisions_y,
												   float color_odd[4],
												   float color_even[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	checkers_aa_t<unsigned short,weight_mapper_linear>(layer1,divisions_x,divisions_y,
		color_odd,color_even);
	return eeOk;
}

EError NIMPLibSwInternal::checkers_aaFp128GenericC(Layer &layer1,
												   int divisions_x,
												   int divisions_y,
												   float color_odd[4],
												   float color_even[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	checkers_aa_t<float,weight_mapper_linear>(layer1,divisions_x,divisions_y,
		color_odd,color_even);
	return eeOk;
}

template<typename T,typename WeightMapper>
void rgradient_t(Layer &layer1,float center_x,float center_y,float radius,
				   float color_inner[4],float color_outer[4])
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	output_float<T>		out_mapper;
	WeightMapper		weight_mapper;
	float inv_radius=1.0f/radius;
	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			float fcoord_x=center_x-(float)j/size_x;
			float fcoord_y=center_y-(float)i/size_y;
			if(fcoord_x>0.5f)
			{
				fcoord_x=1.0f-fcoord_x;
			}
			if(fcoord_x<-0.5f)
			{
				fcoord_x=-1.0f-fcoord_x;
			}
			if(fcoord_y>0.5f)
			{
				fcoord_y=1.0f-fcoord_y;
			}
			if(fcoord_y<-0.5f)
			{
				fcoord_y=-1.0f-fcoord_y;
			}
			float distance=sqrtf(fcoord_x*fcoord_x+fcoord_y*fcoord_y);
			float my_weight_n=distance*inv_radius;
			my_weight_n=weight_mapper(my_weight_n);
			float my_weight=1.0f-my_weight_n;
			*pDest++=out_mapper(
				my_weight*color_inner[0]+(my_weight_n)*color_outer[0]);
			*pDest++=out_mapper(
				my_weight*color_inner[1]+(my_weight_n)*color_outer[1]);
			*pDest++=out_mapper(
				my_weight*color_inner[2]+(my_weight_n)*color_outer[2]);
			*pDest++=out_mapper(
				my_weight*color_inner[3]+(my_weight_n)*color_outer[3]);
		}
	}
}

EError NIMPLibSwInternal::rgradient_cInt32GenericC(Layer &layer1,float center_x,
												   float center_y,float radius,
												   float color_inner[4],
												   float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned char,weight_mapper_linear_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_cInt64GenericC(Layer &layer1,float center_x,
												   float center_y,float radius,
												   float color_inner[4],
												   float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned short,weight_mapper_linear_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_cFp128GenericC(Layer &layer1,float center_x,
												   float center_y,float radius,
												   float color_inner[4],
												   float color_outer[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<float,weight_mapper_linear_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_cubicInt32GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned char,weight_mapper_cubic_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_cubicInt64GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned short,weight_mapper_cubic_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_cubicFp128GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<float,weight_mapper_cubic_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_sinInt32GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned char,weight_mapper_sin_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_sinInt64GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned short,weight_mapper_sin_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_c_sinFp128GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<float,weight_mapper_sin_clamp_up>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}


EError NIMPLibSwInternal::rgradient_eInt32GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned char,weight_mapper_linear>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_eInt64GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned short,weight_mapper_linear>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_eFp128GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<float,weight_mapper_linear>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}


EError NIMPLibSwInternal::rgradient_e_sinInt32GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned char,weight_mapper_sin>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_e_sinInt64GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<unsigned short,weight_mapper_sin>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

EError NIMPLibSwInternal::rgradient_e_sinFp128GenericC(Layer &layer1,float center_x,
														 float center_y,float radius,
														 float color_inner[4],
														 float color_outer[4])
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	rgradient_t<float,weight_mapper_sin>
		(layer1,center_x,center_y,radius,color_inner,color_outer);
	return eeOk;
}

static const unsigned int noise_tbl_size=64;
static const unsigned int noise_tbl_half_size=noise_tbl_size/2;
static const unsigned int noise_tbl_size_mask=63;

//TODO: perhaps should be expressed as weight mapper?
//this is in fact based on smoothstep ...
static inline float noise_weight(float t)
{
	return (2.0f*fabsf(t)-3.0f)*t*t+1.0f;
}

static int noise_perm_tbl[noise_tbl_size];
static float noise_grad_tbl[noise_tbl_size*2];

static float noise_offset;
static float noise_factor;
static unsigned int	octaves;
static unsigned int noise_scale;

static void init_noise_tables()
{
	//initialize the permutation table
	int i=0;
	int j=0;
	int k=0;
	for(i=0;i<noise_tbl_size;i++)
	{
		noise_perm_tbl[i]=i;
	}
	for(i=0;i<(noise_tbl_half_size);i++)
	{
		int t=0;
		j=rand()&noise_tbl_size_mask;
		k=rand()&noise_tbl_size_mask;
		t=noise_perm_tbl[j];
		noise_perm_tbl[j]=noise_perm_tbl[k];
		noise_perm_tbl[k]=t;
	}
  
	//initialize the gradient table
	for(i=0;i<noise_tbl_size;i++)
	{
		float m=0.0f;
		do
		{
			noise_grad_tbl[2*i]=(float)(rand()-(RAND_MAX>>1))/(RAND_MAX>>1);
			noise_grad_tbl[2*i+1]=(float)(rand()-(RAND_MAX>>1))/(RAND_MAX>>1);
			m=noise_grad_tbl[2*i]*noise_grad_tbl[2*i]+noise_grad_tbl[2*i+1]*noise_grad_tbl[2*i+1];
		}
		while((m==0.0f) || (m>1.0f));

		m=1.0f/sqrtf(m);
		noise_grad_tbl[2*i]*=m;
		noise_grad_tbl[2*i+1]*=m;
	}
}

static float calc_plain_noise(float x,float y,unsigned int s)
{
	float vx,vy;
	int a,b,i,j,n;
	float sum=0.0f;

	x*=s;
	y*=s;
	a=(int)floorf(x);
	b=(int)floorf(y);

	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			n=noise_perm_tbl[(((a+i)%(noise_scale*s))+
				noise_perm_tbl[((b+j)%(noise_scale*s))&noise_tbl_size_mask])&noise_tbl_size_mask];
			vx=x-a-i;
			vy=y-b-j;
			sum+=noise_weight(vx)*noise_weight(vy)*
				(noise_grad_tbl[2*n]*vx+noise_grad_tbl[2*n+1]*vy);
		}
	}
	return sum;
}

struct calc_noise
{
	float operator()(float x,float y)
	{
		unsigned int i;
		unsigned int s=1;
		float inv_s=1.0f;
		float sum=0.0f;

		x*=noise_scale;
		y*=noise_scale;

		for(i=0;i<octaves;i++)
		{
			sum+=calc_plain_noise(x,y,s)*inv_s;
			s<<=1;
			inv_s*=0.5f;
		}
		return (sum+noise_offset)*noise_factor;
	}
};

struct calc_turbulent_noise
{
	float operator()(float x,float y)
	{
		unsigned int i;
		unsigned int s=1;
		float inv_s=1.0f;
		float sum=0.0f;

		x*=noise_scale;
		y*=noise_scale;

		for(i=0;i<octaves;i++)
		{
			sum+=fabsf(calc_plain_noise(x,y,s))*inv_s;
			s<<=1;
			inv_s*=0.5f;
		}
		return (sum+noise_offset)*noise_factor;
	}
};

template<typename T,typename NoiseCalc>
void gen_noise_t(Layer &layer1,float pColor1[4],float pColor2[4])
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	output_float<T>		output_mapper;
	NoiseCalc			noise_calculator;
	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			float factor=noise_calculator(j*inv_size_x,i*inv_size_y);
			float factor_n=1.0f-factor;

			*pDest++=output_mapper(factor*pColor1[0]+factor_n*pColor2[0]);
			*pDest++=output_mapper(factor*pColor1[1]+factor_n*pColor2[1]);
			*pDest++=output_mapper(factor*pColor1[2]+factor_n*pColor2[2]);
			*pDest++=output_mapper(factor*pColor1[3]+factor_n*pColor2[3]);
		}
	}
}

EError NIMPLibSwInternal::noiseInt32GenericC(Layer &layer1,int noise_scale_p,
											 int seed,int octaves_p,float color1[4],
											 float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.94f;
	noise_factor=0.526f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<unsigned char,calc_noise>(layer1,color1,color2);
	return eeOk;
}

EError NIMPLibSwInternal::noiseInt64GenericC(Layer &layer1,int noise_scale_p,
											 int seed,int octaves_p,float color1[4],
											 float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.94f;
	noise_factor=0.526f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<unsigned short,calc_noise>(layer1,color1,color2);
	return eeOk;
}

EError NIMPLibSwInternal::noiseFp128GenericC(Layer &layer1,int noise_scale_p,
											 int seed,int octaves_p,float color1[4],
											 float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.94f;
	noise_factor=0.526f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<float,calc_noise>(layer1,color1,color2);
	return eeOk;
}

EError NIMPLibSwInternal::turbulenceInt32GenericC(Layer &layer1,int noise_scale_p,
												  int seed,int octaves_p,
												  float color1[4],float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.0f;
	noise_factor=1.0f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<unsigned char,calc_turbulent_noise>(layer1,color1,color2);
	return eeOk;
}

EError NIMPLibSwInternal::turbulenceInt64GenericC(Layer &layer1,int noise_scale_p,
												  int seed,int octaves_p,
												  float color1[4],float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.0f;
	noise_factor=1.0f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<unsigned short,calc_turbulent_noise>(layer1,color1,color2);
	return eeOk;
}

EError NIMPLibSwInternal::turbulenceFp128GenericC(Layer &layer1,int noise_scale_p,
												  int seed,int octaves_p,
												  float color1[4],float color2[4])
{
	octaves=(octaves_p<1) ? 1 : octaves_p;
	if(octaves>15)
	{
		octaves=15;
	}
	noise_offset=0.0f;
	noise_factor=1.0f;
	srand(seed);
	init_noise_tables();
	noise_scale=noise_scale_p;
	gen_noise_t<float,calc_turbulent_noise>(layer1,color1,color2);
	return eeOk;
}

template<typename T,
		template<typename X> class TextureSampler>
void wave_x_t(Layer &layer1,Layer &layer2,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	TextureSampler<T>	texture_sampler(layer2);
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	float x_mult=inv_size_x*2.0f*pi*nwaves;
	float phase_premult=phase;
	for(i=0;i<size_y;i++)
	{
		float ycoord=(((float)i)+0.5f)*inv_size_y;
		for(j=0;j<size_x;j++)
		{
			float xcoord=j*x_mult;
			float wave=amplitude*sinf(xcoord-phase_premult);
			xcoord=(((float)j)+0.5f)*inv_size_x;
			texture_sampler(pDest,xcoord,ycoord+wave);
			pDest+=4;
		}
	}
}

EError NIMPLibSwInternal::wave_xInt32GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<unsigned char,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<unsigned char,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_xInt64GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<unsigned short,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<unsigned short,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_xFp128GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<float,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<float,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_x_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<unsigned char,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<unsigned char,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_x_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<unsigned short,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<unsigned short,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_x_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_x_t<float,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_x_t<float,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

template<typename T,
		template<typename X> class TextureSampler>
void lwave_x_t(Layer &layer1,Layer &layer2,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	TextureSampler<T>	texture_sampler(layer2);
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	float x_mult=inv_size_x*2.0f*pi*nwaves;
	float phase_premult=phase;
	for(i=0;i<size_y;i++)
	{
		float ycoord=(((float)i)+0.5f)*inv_size_y;
		for(j=0;j<size_x;j++)
		{
			float xcoord=j*x_mult;
			float wave=amplitude*sinf(xcoord-phase_premult);
			xcoord=(((float)j)+0.5f)*inv_size_x;
			texture_sampler(pDest,xcoord+wave,ycoord);
			pDest+=4;
		}
	}
}

EError NIMPLibSwInternal::lwave_xInt32GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<unsigned char,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<unsigned char,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_xInt64GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<unsigned short,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<unsigned short,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_xFp128GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<float,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<float,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_x_bicubicInt32GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<unsigned char,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<unsigned char,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_x_bicubicInt64GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<unsigned short,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<unsigned short,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_x_bicubicFp128GenericC(Layer &layer1,Layer &layer2,
													  float amplitude,float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_x_t<float,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_x_t<float,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

template<typename T,
		template<typename X> class TextureSampler>
void wave_y_t(Layer &layer1,Layer &layer2,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	TextureSampler<T>	texture_sampler(layer2);
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	float y_mult=inv_size_y*2.0f*pi*nwaves;
	float phase_premult=phase;
	for(i=0;i<size_y;i++)
	{
		float ycoord=(((float)i)+0.5f)*inv_size_y;
		float wave=amplitude*sinf(i*y_mult-phase_premult);
		for(j=0;j<size_x;j++)
		{
			float xcoord=(((float)j)+0.5f)*inv_size_x;
			texture_sampler(pDest,xcoord+wave,ycoord);
			pDest+=4;
		}
	}
}

EError NIMPLibSwInternal::wave_yInt32GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<unsigned char,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<unsigned char,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_yInt64GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<unsigned short,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<unsigned short,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_yFp128GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<float,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<float,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_y_bicubicInt32GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<unsigned char,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<unsigned char,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_y_bicubicInt64GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<unsigned short,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<unsigned short,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::wave_y_bicubicFp128GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		wave_y_t<float,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		wave_y_t<float,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

template<typename T,
		template<typename X> class TextureSampler>
void lwave_y_t(Layer &layer1,Layer &layer2,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	TextureSampler<T>	texture_sampler(layer2);
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	float y_mult=inv_size_y*2.0f*pi*nwaves;
	float phase_premult=phase;
	for(i=0;i<size_y;i++)
	{
		float ycoord=(((float)i)+0.5f)*inv_size_y;
		float wave=amplitude*sinf(i*y_mult-phase_premult);
		for(j=0;j<size_x;j++)
		{
			float xcoord=(((float)j)+0.5f)*inv_size_x;
			texture_sampler(pDest,xcoord,ycoord+wave);
			pDest+=4;
		}
	}
}

EError NIMPLibSwInternal::lwave_yInt32GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<unsigned char,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<unsigned char,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_yInt64GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<unsigned short,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<unsigned short,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_yFp128GenericC(Layer &layer1,Layer &layer2,
											  float amplitude,float phase,
											  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<float,texture_sampler_bilinear_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<float,texture_sampler_bilinear>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_y_bicubicInt32GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<unsigned char,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<unsigned char,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_y_bicubicInt64GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<unsigned short,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<unsigned short,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

EError NIMPLibSwInternal::lwave_y_bicubicFp128GenericC(Layer &layer1,
													  Layer &layer2,
													  float amplitude,
													  float phase,
													  int nwaves)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		lwave_y_t<float,texture_sampler_bicubic_pow2>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	else
	{
		lwave_y_t<float,texture_sampler_bicubic>
			(layer1,layer2,amplitude,phase,nwaves);
	}
	return eeOk;
}

template<typename T,
		template<typename X> class TextureSampler>
void vf_distort_t(Layer &layer1,Layer &layer2,Layer &layer3,float scale_x,
				  float scale_y)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	T* restrict_ptr pSrc=(T*)layer3.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	TextureSampler<T>		texture_sampler(layer2);
	input_float_signed<T>	input_mapper;
	float inv_size_x=1.0f/size_x;
	float inv_size_y=1.0f/size_y;
	for(i=0;i<size_y;i++)
	{
		float ycoord=(((float)i)+0.5f)*inv_size_y;
		for(j=0;j<size_x;j++)
		{
			float xcoord=(((float)j)+0.5f)*inv_size_x;
			float n_xcoord=xcoord+scale_x*input_mapper(*pSrc++);
			float n_ycoord=ycoord+scale_y*input_mapper(*pSrc++);
			texture_sampler(pDest,n_xcoord,n_ycoord);
			pDest+=4;
			pSrc+=2;
		}
	}
}

EError NIMPLibSwInternal::vf_distortInt32GenericC(Layer &layer1,Layer &layer2,
												  Layer &layer3,float scale_x,
												  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<unsigned char,texture_sampler_bilinear_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<unsigned char,texture_sampler_bilinear>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

EError NIMPLibSwInternal::vf_distortInt64GenericC(Layer &layer1,Layer &layer2,
												  Layer &layer3,float scale_x,
												  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<unsigned short,texture_sampler_bilinear_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<unsigned short,texture_sampler_bilinear>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

EError NIMPLibSwInternal::vf_distortFp128GenericC(Layer &layer1,Layer &layer2,
												  Layer &layer3,float scale_x,
												  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<float,texture_sampler_bilinear_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<float,texture_sampler_bilinear>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

EError NIMPLibSwInternal::vf_distort_bicubicInt32GenericC(Layer &layer1,
														  Layer &layer2,
														  Layer &layer3,
														  float scale_x,
														  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<unsigned char,texture_sampler_bicubic_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<unsigned char,texture_sampler_bicubic>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

EError NIMPLibSwInternal::vf_distort_bicubicInt64GenericC(Layer &layer1,
														  Layer &layer2,
														  Layer &layer3,
														  float scale_x,
														  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<unsigned short,texture_sampler_bicubic_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<unsigned short,texture_sampler_bicubic>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

EError NIMPLibSwInternal::vf_distort_bicubicFp128GenericC(Layer &layer1,
														  Layer &layer2,
														  Layer &layer3,
														  float scale_x,
														  float scale_y)
{
	if(!equalL3(layer1,layer2,layer3))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	if(is_layer_pow2(layer1))
	{
		vf_distort_t<float,texture_sampler_bicubic_pow2>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	else
	{
		vf_distort_t<float,texture_sampler_bicubic>
			(layer1,layer2,layer3,scale_x,scale_y);
	}
	return eeOk;
}

template<typename T>
void wave_x_vf_t(Layer &layer1,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float inv_size_x=1.0f/size_x;
	float x_mult=inv_size_x*2.0f*pi*nwaves;
	float phase_premult=phase*2.0f*pi*nwaves;
	output_float_signed<T>	output_mapper;
	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			*pDest++=output_mapper(0);
			*pDest++=output_mapper(amplitude*sinf(j*x_mult-phase_premult));
			*pDest++=0;
			*pDest++=output_mapper(1.0f);
		}
	}
}

EError NIMPLibSwInternal::wave_x_vfInt32GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_x_vf_t<unsigned char>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::wave_x_vfInt64GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_x_vf_t<unsigned short>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::wave_x_vfFp128GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_x_vf_t<float>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

template<typename T>
void lwave_x_vf_t(Layer &layer1,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float inv_size_x=1.0f/size_x;
	float x_mult=inv_size_x*2.0f*pi*nwaves;
	float phase_premult=phase*2.0f*pi*nwaves;
	output_float_signed<T>	output_mapper;
	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			*pDest++=output_mapper(amplitude*sinf(j*x_mult-phase_premult));
			*pDest++=output_mapper(0);
			*pDest++=0;
			*pDest++=output_mapper(1.0f);
		}
	}
}

EError NIMPLibSwInternal::lwave_x_vfInt32GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_x_vf_t<unsigned char>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::lwave_x_vfInt64GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_x_vf_t<unsigned short>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::lwave_x_vfFp128GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_x_vf_t<float>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

template<typename T>
void wave_y_vf_t(Layer &layer1,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float inv_size_y=1.0f/size_y;
	float y_mult=inv_size_y*2.0f*pi*nwaves;
	float phase_premult=phase*2.0f*pi*nwaves;
	output_float_signed<T>	output_mapper;
	for(i=0;i<size_y;i++)
	{
		float wave=amplitude*sinf(i*y_mult-phase_premult);
		for(j=0;j<size_x;j++)
		{
			*pDest++=output_mapper(wave);
			*pDest++=output_mapper(0);
			*pDest++=0;
			*pDest++=output_mapper(1.0f);
		}
	}
}

EError NIMPLibSwInternal::wave_y_vfInt32GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_y_vf_t<unsigned char>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::wave_y_vfInt64GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_y_vf_t<unsigned short>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::wave_y_vfFp128GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	wave_y_vf_t<float>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

template<typename T>
void lwave_y_vf_t(Layer &layer1,float amplitude,float phase,int nwaves)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	float inv_size_y=1.0f/size_y;
	float y_mult=inv_size_y*2.0f*pi*nwaves;
	float phase_premult=phase*2.0f*pi*nwaves;
	output_float_signed<T>	output_mapper;
	for(i=0;i<size_y;i++)
	{
		float wave=amplitude*sinf(i*y_mult-phase_premult);
		for(j=0;j<size_x;j++)
		{
			*pDest++=output_mapper(0);
			*pDest++=output_mapper(wave);
			*pDest++=0;
			*pDest++=output_mapper(1.0f);
		}
	}
}

EError NIMPLibSwInternal::lwave_y_vfInt32GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_y_vf_t<unsigned char>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::lwave_y_vfInt64GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_y_vf_t<unsigned short>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

EError NIMPLibSwInternal::lwave_y_vfFp128GenericC(Layer &layer1,float amplitude,
												 float phase,int nwaves)
{
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	lwave_y_vf_t<float>(layer1,amplitude,phase,nwaves);
	return eeOk;
}

template<typename T>
void hsv_adjust_t(Layer &layer1,Layer &layer2,float hsv_rotation_deg,float s_offset,float v_offset)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	T* restrict_ptr pSrc=(T*)layer2.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	output_float<T>	output_mapper;
	input_float<T>	input_mapper;

	hsv_rotation_deg=fmodf(hsv_rotation_deg,360.0f);
	hsv_rotation_deg+=360.0f;
	hsv_rotation_deg=fmodf(hsv_rotation_deg,360.0f);
	hsv_rotation_deg/=60.0f;

	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			float r=input_mapper(*pSrc++);
			float g=input_mapper(*pSrc++);
			float b=input_mapper(*pSrc++);
			float a=input_mapper(*pSrc++);
			float h,s,v;
			if((r>g) && (r>b))
			{
				//use R as MAX
				v=r;
				if(g>b)
				{
					//use B as MIN
					h=(g-b)/(r-b);
					s=(r-b)/r;
				}
				else
				{
					//use G as MIN
					h=(g-b)/(r-g);
					s=(r-g)/r;
				}
			}
			else if(g>b)
			{
				//use G as MAX
				v=g;
				if(r>b)
				{
					//use B as MIN
					h=2.0f+(b-r)/(g-b);
					s=(g-b)/g;
				}
				else
				{
					//use R as MIN
					h=2.0f+(b-r)/(g-r);
					s=(g-r)/g;
				}
			}
			else
			{
				//use B
				if(b>0.0f)
				{
					v=b;
					if(r>g)
					{
						//use G as MIN
						h=4.0f+(r-g)/(b-g);
						s=(b-g)/b;
					}
					else
					{
						//use R as MIN
						if(b==r)
						{
							h=0.0f;
						}
						else
						{
							h=4.0f+(r-g)/(b-r);
						}
						s=(b-r)/b;
					}
				}
				else
				{
					h=s=v=0.0f;
				}
			}
			//ok, we have HSV, now adjust values
			h+=hsv_rotation_deg;
			h+=6.0f;
			h=fmodf(h,6.0f);
			s+=s_offset;
			if(s>1.0f)
			{
				s=1.0f;
			}
			if(s<0.0f)
			{
				s=0.0f;
			}
			v+=v_offset;
			if(v>1.0f)
			{
				v=1.0f;
			}
			if(v<0.0f)
			{
				v=0.0f;
			}
			//transform back to RGB
			float fh=h-floor(h);
			float p=v*(1-s);
			float q=v*(1-s*fh);
			float t=v*(1-s*(1-fh));

			if(h>=5.0f)
			{
				r=v;
				g=p;
				b=q;
			}
			else if(h>=4.0f)
			{
				r=t;
				g=p;
				b=v;
			}
			else if(h>=3.0f)
			{
				r=p;
				g=q;
				b=v;
			}
			else if(h>=2.0f)
			{
				r=p;
				g=v;
				b=t;
			}
			else if(h>=1.0f)
			{
				r=q;
				g=v;
				b=p;
			}
			else
			{
				r=v;
				g=t;
				b=p;
			}
			*pDest++=output_mapper(r);
			*pDest++=output_mapper(g);
			*pDest++=output_mapper(b);
			*pDest++=output_mapper(a);
		}
	}
}

EError NIMPLibSwInternal::hsv_adjustInt32GenericC(Layer &layer1,Layer &layer2,
													 float h_rotation_deg,float s_offset,
													 float v_offset)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	hsv_adjust_t<unsigned char>(layer1,layer2,h_rotation_deg,s_offset,v_offset);
	return eeOk;
}

EError NIMPLibSwInternal::hsv_adjustInt64GenericC(Layer &layer1,Layer &layer2,
													 float h_rotation_deg,float s_offset,
													 float v_offset)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	hsv_adjust_t<unsigned short>(layer1,layer2,h_rotation_deg,s_offset,v_offset);
	return eeOk;
}

EError NIMPLibSwInternal::hsv_adjustFp128GenericC(Layer &layer1,Layer &layer2,
													 float h_rotation_deg,float s_offset,
													 float v_offset)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	hsv_adjust_t<float>(layer1,layer2,h_rotation_deg,s_offset,v_offset);
	return eeOk;
}

template<typename T>
void grayscale_t(Layer &layer1,Layer &layer2)
{
	unsigned int i=0;
	unsigned int j=0;
	T* restrict_ptr pDest=(T*)layer1.byte_data;
	T* restrict_ptr pSrc=(T*)layer2.byte_data;
	DWORD size_x=layer1.desc.dwSizeX;
	DWORD size_y=layer1.desc.dwSizeY;
	output_float<T>	output_mapper;
	input_float<T>	input_mapper;

	for(i=0;i<size_y;i++)
	{
		for(j=0;j<size_x;j++)
		{
			float r=input_mapper(*pSrc++);
			float g=input_mapper(*pSrc++);
			float b=input_mapper(*pSrc++);
			float gray=0.3f*r+0.59f*g+0.11f*b;
			T gray_c=output_mapper(gray);
			*pDest++=gray_c;
			*pDest++=gray_c;
			*pDest++=gray_c;
			*pDest++=*pSrc++;
		}
	}
}

EError NIMPLibSwInternal::grayscaleInt32GenericC(Layer &layer1,Layer &layer2)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt32Bit)
	{
		return eeUnsupportedFormat;
	}
	grayscale_t<unsigned char>(layer1,layer2);
	return eeOk;
}

EError NIMPLibSwInternal::grayscaleInt64GenericC(Layer &layer1,Layer &layer2)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfInt64Bit)
	{
		return eeUnsupportedFormat;
	}
	grayscale_t<unsigned short>(layer1,layer2);
	return eeOk;
}

EError NIMPLibSwInternal::grayscaleFp128GenericC(Layer &layer1,Layer &layer2)
{
	if(!equalL2(layer1,layer2))
	{
		return eeInvalidParameter;
	}
	if(layer1.desc.eFormat!=elfFloat128Bit)
	{
		return eeUnsupportedFormat;
	}
	grayscale_t<float>(layer1,layer2);
	return eeOk;
}

EError NIMPLibSwInternal::sobel_edge_detectInt32GenericC(Layer& /*layer1*/,Layer& /*layer2*/,float /*sample_spacing*/)
{
	return eeNotImplemented;
}

EError NIMPLibSwInternal::sobel_edge_detectInt64GenericC(Layer& /*layer1*/,Layer& /*layer2*/,float /*sample_spacing*/)
{
	return eeNotImplemented;
}

EError NIMPLibSwInternal::sobel_edge_detectFp128GenericC(Layer& /*layer1*/,Layer& /*layer2*/,float /*sample_spacing*/)
{
	return eeNotImplemented;
}

