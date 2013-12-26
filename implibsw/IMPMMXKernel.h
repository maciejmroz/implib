/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */



/*
 *	Internal part of software execution engine
 *  MMX optimized functions
 */

//function naming convention:
//OPERATOR NAME+FORMAT+"MMX" where:
//format is one of Int32, Int64

#ifndef _IMP_LIB_SW_MMX_KERNEL_
#define _IMP_LIB_SW_MMX_KERNEL_

namespace NIMPLib {
	namespace NIMPLibSwInternal {
		//add
		extern EError addInt32MMX(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError addInt64MMX(Layer &layer1,Layer &layer2,Layer &layer3);
		//add color
		extern EError addColorInt32MMX(Layer &layer1,Layer &layer2,float color[4]);
		extern EError addColorInt64MMX(Layer &layer1,Layer &layer2,float color[4]);
		//sub
		extern EError subInt32MMX(Layer &layer1,Layer &layer2,Layer &layer3);
		extern EError subInt64MMX(Layer &layer1,Layer &layer2,Layer &layer3);
		//sub color
		extern EError subColorInt32MMX(Layer &layer1,Layer &layer2,float color[4]);
		extern EError subColorInt64MMX(Layer &layer1,Layer &layer2,float color[4]);
	};
};

#endif
