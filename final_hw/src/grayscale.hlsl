/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

sampler2D s1;

float4 main(PS_INPUT p) : COLOR
{
	float4 tx=tex2D(s1,p.t_position.xy);
	tx.rgb=dot(tx.rgb,float3(0.3,0.59,0.11));
	return tx;
}
