
float cubic_weight(float t)
{
	float out_v=0.0;
	float t2=t*t;
	float t3=t2*t;
	if(t<1.0)
	{
		out_v=3*t3-4*t2+1;
	}
	else if(t<2.0)
	{
		out_v=t3-5*t2+8*t-4;
	}
	return out_v;
}

float4 cubic_weight_sum(float s,float4 v1,float4 v2,float4 v3,float4 v4)
{
	return cubic_weight(s)*v1+
		cubic_weight(s-1.0)*v2+
		cubic_weight(2.0-s)*v3+
		cubic_weight(3.0-s)*v4;
}

float4 texture_sampler_bicubic2(sampler2D s,float2 coords)
{
	float fx=(tx_sizes.x*frac(coords.x))-0.5;
	float fy=(tx_sizes.y*frac(coords.y))-0.5;
    
	float frac_fx=frac(fx);
	float frac_fy=frac(fy);
	
	float weight_x=(frac_fx+1.0);
	float weight_y=(frac_fy+1.0);

	float2 texel_base=coords-float2(frac_fx,frac_fy)*tx_sizes.zw;

	float4 sum=float4(0,0,0,0);
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			sum+=cubic_weight(length(float2(weight_x-i,weight_y-j)))*
				tex2D(s,texel_base+float2(i*tx_sizes.z,j*tx_sizes.w));
		}
	}
	return sum;
}

float cubic_weight(float t)
{
	float out_v;
	float t2=t*t;
	float t3=t2*t;
	if(t<1.0)
	{
		out_v=3*t3-4*t2+1;
	}
	else
	{
		out_v=t3-5*t2+8*t-4;
	}
	return out_v;
}

float4 cubic_weight_sum(float s,float4 v1,float4 v2,float4 v3,float4 v4)
{
	return cubic_weight(s)*v1+
		cubic_weight(s-1.0)*v2+
		cubic_weight(2.0-s)*v3+
		cubic_weight(3.0-s)*v4;
}

float4 texture_sampler_bicubic2(sampler2D s,float2 coords)
{
	float fx=(tx_sizes.x*frac(coords.x))-0.5;
	float fy=(tx_sizes.y*frac(coords.y))-0.5;
    
	float frac_fx=frac(fx);
	float frac_fy=frac(fy);
	
	float weight_x=(frac_fx+1.0);
	float weight_y=(frac_fy+1.0);

	float2 texel_base=coords-float2(frac_fx,frac_fy)*tx_sizes.zw;

	float2 temp_coords1=texel_base+float2(0,-tx_sizes.w);
	float4 t11=tex2D(s,temp_coords1+float2(-tx_sizes.z,0));
	float4 t12=tex2D(s,temp_coords1);
	float4 t13=tex2D(s,temp_coords1+float2(tx_sizes.z,0));
	float4 t14=tex2D(s,temp_coords1+float2(2*tx_sizes.z,0));

	float4 t21=tex2D(s,texel_base+float2(-tx_sizes.z,0));
	float4 t22=tex2D(s,texel_base);
	float4 t23=tex2D(s,texel_base+float2(tx_sizes.z,0));
	float4 t24=tex2D(s,texel_base+float2(2*tx_sizes.z,0));

	float2 temp_coords3=texel_base+float2(0,tx_sizes.w);
	float4 t31=tex2D(s,temp_coords3+float2(-tx_sizes.z,0));
	float4 t32=tex2D(s,temp_coords3);
	float4 t33=tex2D(s,temp_coords3+float2(tx_sizes.z,0));
	float4 t34=tex2D(s,temp_coords3+float2(2*tx_sizes.z,0));

	float2 temp_coords4=texel_base+float2(0,2*tx_sizes.w);
	float4 t41=tex2D(s,temp_coords4+float2(-tx_sizes.z,0));
	float4 t42=tex2D(s,temp_coords4);
	float4 t43=tex2D(s,temp_coords4+float2(tx_sizes.z,0));
	float4 t44=tex2D(s,temp_coords4+float2(2*tx_sizes.z,0));

	return cubic_weight_sum(weight_y,
		cubic_weight_sum(weight_x,t11,t12,t13,t14),
		cubic_weight_sum(weight_x,t21,t22,t23,t24),
		cubic_weight_sum(weight_x,t31,t32,t33,t34),
		cubic_weight_sum(weight_x,t41,t42,t43,t44));

}
