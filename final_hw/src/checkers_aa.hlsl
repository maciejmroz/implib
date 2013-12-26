/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */


//antialiased checkerboard pattern

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

//shader matches i2_c2 prototype
float i1;
float i2;
float4 c1;
float4 c2;

//special constants
//pixel size is in fact two pairs of values
//(xy) - real pixel sizes but multipled by number of divisions
//(to avoid in shader multiplication)
//(zw) - normally half of real pixel sizes (0.5/width,0.5/height),
//but can additionally be used to offset whole shading coordinate
//system
float4 pixel_size;
float pixel_area_inv;

float4 main(PS_INPUT p) : COLOR
{
	float fcoord_x=(p.t_position.x-pixel_size.z)*i1;
	float fcoord_y=(p.t_position.y-pixel_size.w)*i2;
	float fl_fcoord_x=floor(fcoord_x);
	float fl_fcoord_y=floor(fcoord_y);
	//beware of possible floating point innacuracies!
	float odd_even_choose=fmod(fl_fcoord_x+fl_fcoord_y,2.0)-0.5;
	float xdiv=0.0;
	if(floor(fcoord_x+pixel_size.x)>fl_fcoord_x)
	{
		xdiv=frac(fcoord_x+pixel_size.x);
	}
	float ydiv=0.0;
	if(floor(fcoord_y+pixel_size.y)>fl_fcoord_y)
	{
		ydiv=frac(fcoord_y+pixel_size.y);
	}
	float my_weight=((pixel_size.x-xdiv)*(pixel_size.y-ydiv)+xdiv*ydiv)*pixel_area_inv;
	if(odd_even_choose>0.0)
	{
		my_weight=1.0-my_weight;
	}
	return lerp(c2,c1,my_weight);
}
