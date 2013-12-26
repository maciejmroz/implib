/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float f1; //sample_spacing

sampler2D s1;

float4 main(PS_INPUT p) : COLOR
{
	float4 p1=tex2D(s1,p.t_position+float2(-f1,-f1));
	float4 p2=tex2D(s1,p.t_position+float2(0,-f1));
	float4 p3=tex2D(s1,p.t_position+float2(f1,-f1));
	float4 p4=tex2D(s1,p.t_position+float2(-f1,0));
	float4 p5=tex2D(s1,p.t_position);
	float4 p6=tex2D(s1,p.t_position+float2(f1,0));
	float4 p7=tex2D(s1,p.t_position+float2(-f1,f1));
	float4 p8=tex2D(s1,p.t_position+float2(0,f1));
	float4 p9=tex2D(s1,p.t_position+float2(f1,f1));
	float3 G=abs((p1+2*p2+p3)-(p7+2*p8+p9))+
			abs((p3+2*p6+p9)-(p1+2*p4+p7));
	return float4(G,p5.a);
}
