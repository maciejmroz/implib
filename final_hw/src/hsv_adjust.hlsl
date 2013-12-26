/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

struct PS_INPUT {
 float2 t_position : TEXCOORD0;
};

float3 hsv_adjust_vec; //hue rotation,saturation offset,value offset

//float f1; //hue rotation
//float f2; //saturation offset
//float f3; //value offset

float3 hsv_adjust(float3 input,float hue_rotation,float s_offset,float v_offset)
{
	float3 output;
	float h,s,v;
	if((input.r>input.g) && (input.r>input.b))
	{
		//use R as MAX
		v=input.r;
		if(input.g>input.b)
		{
			//use B as MIN
			h=(input.g-input.b)/(input.r-input.b);
			s=(input.r-input.b)/input.r;
		}
		else
		{
			//use G as MIN
			h=(input.g-input.b)/(input.r-input.g);
			s=(input.r-input.g)/input.r;
		}
	}
	else if(input.g>input.b)
	{
		//use G as MAX
		v=input.g;
		if(input.r>input.b)
		{
			//use B as MIN
			h=2.0+(input.b-input.r)/(input.g-input.b);
			s=(input.g-input.b)/input.g;
		}
		else
		{
			//use R as MIN
			h=2.0+(input.b-input.r)/(input.g-input.r);
			s=(input.g-input.r)/input.g;
		}
	}
	else
	{
		//use B
		if(input.b>0.0)
		{
			v=input.b;
			if(input.r>input.g)
			{
				//use G as MIN
				h=4.0+(input.r-input.g)/(input.b-input.g);
				s=(input.b-input.g)/input.b;
			}
			else
			{
				//use R as MIN
				if(input.b==input.r)
				{
					h=0.0;
				}
				else
				{
					h=4.0+(input.r-input.g)/(input.b-input.r);
				}
				s=(input.b-input.r)/input.b;
			}
		}
		else
		{
			h=s=v=0.0f;
		}
	}
	//ok, we have HSV, now adjust values
	h+=hue_rotation;
	h+=6.0;
	h=fmod(h,6.0);
	s=saturate(s+s_offset);
	v=saturate(v+v_offset);
	//transform back to RGB
	float fh=h-floor(h);
	float p=v*(1-s);
	float q=v*(1-s*fh);
	float t=v*(1-s*(1-fh));

	if(h>=5.0)
	{
		output.r=v;
		output.g=p;
		output.b=q;
	}
	else if(h>=4.0)
	{
		output.r=t;
		output.g=p;
		output.b=v;
	}
	else if(h>=3.0)
	{
		output.r=p;
		output.g=q;
		output.b=v;
	}
	else if(h>=2.0)
	{
		output.r=p;
		output.g=v;
		output.b=t;
	}
	else if(h>=1.0)
	{
		output.r=q;
		output.g=v;
		output.b=p;
	}
	else
	{
		output.r=v;
		output.g=t;
		output.b=p;
	}
	return output;
}

sampler2D s1;

float4 main(PS_INPUT p) : COLOR
{
	float4 input=tex2D(s1,p.t_position.xy);
	return float4(hsv_adjust(input.xyz,hsv_adjust_vec.x,hsv_adjust_vec.y,hsv_adjust_vec.z),input.w);
}
