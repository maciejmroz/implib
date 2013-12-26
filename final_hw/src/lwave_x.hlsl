/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

sampler2D s1;

float f1; //amplitude
float f2; //phase
float i1; //nwaves

//normally half of real pixel sizes (0.5/width,0.5/height),
//but can additionally be used to offset whole shading coordinate
//system
float2 shading_space_offset;

float4 main(PS_INPUT p) : COLOR
{
	float2 shading_space_coords=p.t_position-shading_space_offset;
	float2 sampling_coords=
		float2(p.t_position.x+f1*sin(6.28318*(shading_space_coords.x*i1-f2)),
				p.t_position.y);
	return tex2D(s1,sampling_coords);
}
