/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

#include "stdafx.h"
#include "IMPLibSw.h"

using namespace NIMPLib;

ExecutionEngine* NIMPLib::createSoftwareExecutionEngine(CodePath cp)
{
	ExecutionEngine *ec=NULL;
	NIMPLib::ExecutionEngineSw *ec_sw=NULL;
	ec_sw=new ExecutionEngineSw();
	ec_sw->init(cp);
	ec=static_cast<ExecutionEngine*>(ec_sw);
	return ec;
}
