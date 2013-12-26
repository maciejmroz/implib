/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float4 c1;

sampler2D s1;

float4 main(PS_INPUT p) : COLOR
{
	return tex2D(s1,p.t_position.xy)*c1;
}
