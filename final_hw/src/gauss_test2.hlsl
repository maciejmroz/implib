/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

//randomly rotated kernel with fixed sample count
#define NUM_SAMPLES 16

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

sampler2D source_layer;
sampler2D rotation_data_layer;

float3 offset_tbl[NUM_SAMPLES];

float4 main(PS_INPUT p) : COLOR
{
	float4 accum=float4(0,0,0,0);
	float4 rot_data=tex2D(rotation_data_layer,p.t_position.xy);
	for(int i=0;i<NUM_SAMPLES;i++)
	{
		accum+=offset_tbl[i].z*tex2D(
			source_layer,
			p.t_position.xy+float2(dot(offset_tbl[i].xy,rot_data.xy),dot(offset_tbl[i].xy,rot_data.zw)));
	}
	return accum;
}
