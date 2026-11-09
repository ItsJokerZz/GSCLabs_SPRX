#include "stdafx.h"

SV_SetConfigstring_t SV_SetConfigstring = nullptr;
PMem_AllocFromSource_NoDebug_t PMem_AllocFromSource_NoDebug = nullptr;
Scr_GetString_t Scr_GetString = nullptr;
Cbuf_AddText_t Cbuf_AddText = nullptr;

void ResolveFunctions()
{
    SV_SetConfigstring = reinterpret_cast<SV_SetConfigstring_t>(0x7E9730);
    PMem_AllocFromSource_NoDebug = reinterpret_cast<PMem_AllocFromSource_NoDebug_t>(0x8820B0);
}

void AWMPFunctions()
{
    SV_SetConfigstring = reinterpret_cast<SV_SetConfigstring_t>(0x89E050);
    PMem_AllocFromSource_NoDebug = reinterpret_cast<PMem_AllocFromSource_NoDebug_t>(0x934F40);
    Scr_GetString = reinterpret_cast<Scr_GetString_t>(0x853E70);
    Cbuf_AddText = reinterpret_cast<Cbuf_AddText_t>(0x7F6020);
}

void WW2MPFunctions()
{
    PMem_AllocFromSource_NoDebug = reinterpret_cast<PMem_AllocFromSource_NoDebug_t>(0x10F4EB0);
}