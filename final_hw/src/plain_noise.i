//assume 64 entry permutation table
//gradient table is two channels, so scaling is the same
#define INV_NOISE_TBL_SIZE 0.015625
#define INV_GRAD_TBL_SIZE 0.015625

sampler1D noise_perm_tbl;	//in fact 1D texture
sampler1D noise_grad_tbl;	//in fact 1D texture

float4 noise_weight4(float4 t)
{
	return (float4(2.0,2.0,2.0,2.0)*abs(t)-float4(3.0,3.0,3.0,3.0))*t*t+float4(1.0,1.0,1.0,1.0);
}

float calc_plain_noise(float2 coords,int s,float scale)
{
	float4 b_ofs=float4(0.001,1.001,0.001,1.001);
	float4 a_ofs=float4(0.001,0.001,1.001,1.001);
	float4 hv=float4(0.5,0.5,0.5,0.5);
	float4 j4=float4(0,1,0,1);
	float4 i4=float4(0,0,1,1);
	
	coords*=s;
	
	int a=floor(coords.x);
	int b=floor(coords.y);
	
	float4 a4=float4(a,a,a,a);
	float4 b4=float4(b,b,b,b);
	float nss=scale*s;
	float4 nss4=float4(nss,nss,nss,nss);
	
	float4 perm_tbl_internal_lookup_index4=fmod(b4+b_ofs,nss4);
	float4 perm_tbl_internal_lookup_coord4=(perm_tbl_internal_lookup_index4+hv)*INV_NOISE_TBL_SIZE;
	float4 perm_tbl_internal_lookup4=float4(
		(float)tex1D(noise_perm_tbl,perm_tbl_internal_lookup_coord4.x),
		(float)tex1D(noise_perm_tbl,perm_tbl_internal_lookup_coord4.y),
		(float)tex1D(noise_perm_tbl,perm_tbl_internal_lookup_coord4.z),
		(float)tex1D(noise_perm_tbl,perm_tbl_internal_lookup_coord4.w)
	);
	perm_tbl_internal_lookup4*=63.75;
	float4 perm_tbl_lookup_index4=fmod(a4+a_ofs,nss4)+perm_tbl_internal_lookup4;
	float4 perm_tbl_lookup_coord4=(perm_tbl_lookup_index4+hv)*INV_NOISE_TBL_SIZE;
	float4 n4=float4(
		(float)tex1D(noise_perm_tbl,perm_tbl_lookup_coord4.x),
		(float)tex1D(noise_perm_tbl,perm_tbl_lookup_coord4.y),
		(float)tex1D(noise_perm_tbl,perm_tbl_lookup_coord4.z),
		(float)tex1D(noise_perm_tbl,perm_tbl_lookup_coord4.w)
	);
	n4*=63.75;
	float4 grad_tbl_coord4=(n4+hv)*INV_GRAD_TBL_SIZE;
	float4 grad_v_a=float4(
		(float2)tex1D(noise_grad_tbl,grad_tbl_coord4.x),
		(float2)tex1D(noise_grad_tbl,grad_tbl_coord4.y)
	);
	float4 grad_v_b=float4(
		(float2)tex1D(noise_grad_tbl,grad_tbl_coord4.z),
		(float2)tex1D(noise_grad_tbl,grad_tbl_coord4.w)
	);
	float4 vx4=float4(coords.x,coords.x,coords.x,coords.x)-a4-i4;
	float4 vy4=float4(coords.y,coords.y,coords.y,coords.y)-b4-j4;
	float4 product=noise_weight4(vx4)*noise_weight4(vy4);
	
	float4 grad_v_c=float4(grad_v_a.xz,grad_v_b.xz);
	float4 grad_v_d=float4(grad_v_a.yw,grad_v_b.yw);
	float4 grad_v_product=grad_v_c*vx4+grad_v_d*vy4;

	return dot(product,grad_v_product);
}
