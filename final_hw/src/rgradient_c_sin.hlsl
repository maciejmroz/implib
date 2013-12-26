/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

//shader matches f3_c2 prototype
float f1;	//center_x
float f2;	//center_y
float f3;	//1.0/radius
float4 c1;
float4 c2;

float weight_map_sin_clamp(float t)
{
	float ret=0.5*(1.0+sin(3.1415926536*(t-0.5)));
	if(t>1.0)
	{
		ret=1.0;
	}
	return ret;
}

float4 main(PS_INPUT p) : COLOR
{
	float fcoord_x=f1-p.t_position.x;
	float fcoord_y=f2-p.t_position.y;
	//TODO: perhaps can be expressed in terms of some mul/fmod tricks??
	if(fcoord_x>0.5)
	{
		fcoord_x=1.0-fcoord_x;
	}
	if(fcoord_x<-0.5)
	{
		fcoord_x=-1.0-fcoord_x;
	}
	if(fcoord_y>0.5)
	{
		fcoord_y=1.0-fcoord_y;
	}
	if(fcoord_y<-0.5)
	{
		fcoord_y=-1.0-fcoord_y;
	}
	float distance=sqrt(fcoord_x*fcoord_x+fcoord_y*fcoord_y);
	float my_weight=weight_map_sin_clamp(distance*f3);
	return lerp(c1,c2,my_weight);
}
