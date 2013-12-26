/*
 * Created by Maciej Mróz
 * Copyright (C) 2005 Maciej Mróz, All Rights Reserved.
 */

#include "stdafx.h"
#include "IMPLibHw.h"

using namespace NIMPLib;

ExecutionEngine* NIMPLib::createHardwareExecutionEngine(PDIRECT3DDEVICE9 dev)
{
	ExecutionEngine *ec=NULL;
	NIMPLib::ExecutionEngineHw *ec_hw=NULL;
	ec_hw=new ExecutionEngineHw();
	ec_hw->init(dev);
	ec=static_cast<ExecutionEngine*>(ec_hw);
	return ec;
}
