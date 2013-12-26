/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float f1; //noise offset
float f2; //noise factor

float4 c1; //color1
float4 c2; //color2

sampler2D s1;

float4 main(PS_INPUT p) : COLOR
{
	float factor=tex2D(s1,p.t_position.xy);
	return lerp(c2,c1,(factor+f1)*f2);
}
