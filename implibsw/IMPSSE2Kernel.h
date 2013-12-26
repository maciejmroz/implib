/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */



/*
 *	Internal part of software execution engine
 *  SSE2 optimized functions
 */

//function naming convention:
//OPERATOR NAME+FORMAT+"SSE2" where:
//format is one of Int32, Int64

#ifndef _IMP_LIB_SW_SSE2_KERNEL_
#define _IMP_LIB_SW_SSE2_KERNEL_

namespace NIMPLib {
	namespace NIMPLibSwInternal {
		//add
		extern EError addInt32SSE2(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError addInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3);
		//add color
		extern EError addColorInt32SSE2(Layer &layer1,Layer &layer2,float color[4]);
		extern EError addColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]);
		//sub
		extern EError subInt32SSE2(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError subInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3);
		//sub color
		extern EError subColorInt32SSE2(Layer &layer1,Layer &layer2,float color[4]);
		extern EError subColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]);
		//mul - TODO: int32 version!
		extern EError mulInt64SSE2(Layer &layer1,Layer &layer2,Layer &layer3);
		//mul color - TODO: int32 version!
		extern EError mulColorInt64SSE2(Layer &layer1,Layer &layer2,float color[4]);
	};
};

#endif
