/*
 * Created by Maciej Mr�z
 * Copyright (C) 2005 Maciej Mr�z, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float4 color;

sampler2D source_layer;

float4 main(PS_INPUT p) : COLOR
{
	return tex2D(source_layer,p.t_position.xy)+color;
}
