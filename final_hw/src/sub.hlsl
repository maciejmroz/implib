/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

sampler2D s1;
sampler2D s2;

float4 main(PS_INPUT p) : COLOR
{
	return tex2D(s1,p.t_position.xy)-
		tex2D(s2,p.t_position.xy);
}
