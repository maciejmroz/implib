float4 catmull_rom(float s,float4 v1,float4 v2,float4 v3,float4 v4)
{
/*	return ((((-v1+3*v2-3*v3+v4)*s+
		(2*v1-5*v2+4*v3-v4))*s+
		(-v1+v3))*s)*0.5+v2;*/

	//1.3 times faster
	float s2=s*s;
	float s3=s*s2;
	float4 s_v=float4(s3,s2,s,1);
	
	return v1*dot(s_v,float4(-0.5,1,-0.5,0))+
		v2*dot(s_v,float4(1.5,-2.5,0,1))+
		v3*dot(s_v,float4(-1.5,2,0.5,0))+
		v4*dot(s_v,float4(0.5,-0.5,0,0));
}

float4 texture_sampler_bicubic(sampler2D s,float2 coords,float4 tx_sizes)
{
	float fx=(tx_sizes.x*frac(coords.x))-0.5;
	float fy=(tx_sizes.y*frac(coords.y))-0.5;
    
	float frac_fx=frac(fx);
	float frac_fy=frac(fy);
	
	float weight_x=frac_fx;
	float weight_y=frac_fy;

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

	return catmull_rom(weight_y,
		catmull_rom(weight_x,t11,t12,t13,t14),
		catmull_rom(weight_x,t21,t22,t23,t24),
		catmull_rom(weight_x,t31,t32,t33,t34),
		catmull_rom(weight_x,t41,t42,t43,t44));
}
