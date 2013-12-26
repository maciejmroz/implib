/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 *
 */

#include "plain_noise.i"

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float2 shading_space_offset;

float noise_scale; //int in real world
float octave_scale; //powers of two starting at 1
float octave_weight; //inverse of octave_scale

#ifndef OCTAVES
#define OCTAVES 1
#endif

float calc_octave(float2 coords)
{
	coords*=noise_scale;
	float sum=0.0;
	float os=octave_scale;
	float ow=octave_weight;
	for(int i=0;i<OCTAVES;i++)
	{
		sum+=abs(calc_plain_noise(coords,os,noise_scale))*ow;
		os*=2;
		ow*=0.5;
	}
	return sum;
}

float4 main(PS_INPUT p) : COLOR
{
	float2 shading_space_coords=p.t_position-shading_space_offset;
	float noise_val=calc_octave(shading_space_coords);
	return float4(noise_val,noise_val,noise_val,noise_val);
}
