//Automatically generated header file
//Do not edit manually!!!

namespace image_processing
{
	namespace add
	{
		const unsigned int index=0;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum sampler_ids
		{
			sid_source1=0,				//sampler2D
			sid_source2=1,				//sampler2D
		}

	}

	namespace add_color
	{
		const unsigned int index=1;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum constant_ids
		{
			cid_color=0,				//float4
		}

		enum sampler_ids
		{
			sid_source_layer=0,			//sampler2D
		}

	}

	namespace mul
	{
		const unsigned int index=2;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum sampler_ids
		{
			sid_s1=0,					//sampler2D
			sid_s2=1,					//sampler2D
		}

	}

	namespace mul_color
	{
		const unsigned int index=3;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum constant_ids
		{
			cid_c1=0,					//float4
		}

		enum sampler_ids
		{
			sid_s1=0,					//sampler2D
		}

	}

	namespace sub
	{
		const unsigned int index=4;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum sampler_ids
		{
			sid_s1=0,					//sampler2D
			sid_s2=1,					//sampler2D
		}

	}

	namespace sub_color
	{
		const unsigned int index=5;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum constant_ids
		{
			cid_c1=0,					//float4
		}

		enum sampler_ids
		{
			sid_s1=0,					//sampler2D
		}

	}

	namespace signed_add
	{
		const unsigned int index=6;
		typedef PDIRECT3DPIXELSHADER9 sh_type;

		enum sampler_ids
		{
			sid_s1=0,					//sampler2D
			sid_s2=1,					//sampler2D
		}

	}

}
