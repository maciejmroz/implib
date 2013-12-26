/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float f1; //scale_x
float f2; //scale_y

sampler2D s1;
sampler2D s2;

float4 main(PS_INPUT p) : COLOR
{
	return tex2D(s1,
		p.t_position+2*float2(f1,f2)*(tex2D(s2,p.t_position).xy-float2(0.5,0.5)));
}
