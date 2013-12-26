/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

#define NUM_QUADS 15

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

sampler2D source_layer;

float texel_x_size;
float center_weight;
float4 coeffs[NUM_QUADS];

float4 main(PS_INPUT p) : COLOR
{
	float4 offset=float4(texel_x_size,2*texel_x_size,3*texel_x_size,4*texel_x_size);
	float4 offset_inc=float4(4*texel_x_size,4*texel_x_size,4*texel_x_size,4*texel_x_size);
	float4 accum=center_weight*tex2D(source_layer,p.t_position.xy);
	for(int i=0;i<NUM_QUADS;i++)
	{
		accum+=coeffs[i].x*tex2D(source_layer,float2(p.t_position.x+offset.x,p.t_position.y));
		accum+=coeffs[i].x*tex2D(source_layer,float2(p.t_position.x-offset.x,p.t_position.y));
		accum+=coeffs[i].y*tex2D(source_layer,float2(p.t_position.x+offset.y,p.t_position.y));
		accum+=coeffs[i].y*tex2D(source_layer,float2(p.t_position.x-offset.y,p.t_position.y));
		accum+=coeffs[i].z*tex2D(source_layer,float2(p.t_position.x+offset.z,p.t_position.y));
		accum+=coeffs[i].z*tex2D(source_layer,float2(p.t_position.x-offset.z,p.t_position.y));
		accum+=coeffs[i].w*tex2D(source_layer,float2(p.t_position.x+offset.w,p.t_position.y));
		accum+=coeffs[i].w*tex2D(source_layer,float2(p.t_position.x-offset.w,p.t_position.y));
		offset+=offset_inc;
	}
	return accum;
}
