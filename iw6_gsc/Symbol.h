#pragma once

typedef void(*SV_SetConfigstring_t)(uint32_t index, const char* val);
extern SV_SetConfigstring_t SV_SetConfigstring;

typedef char*(*Scr_GetString_t)(unsigned int index);
extern Scr_GetString_t Scr_GetString;

typedef void* (*PMem_AllocFromSource_NoDebug_t)(uint32_t size, uint32_t alignment, uint32_t type, int32_t source);
extern PMem_AllocFromSource_NoDebug_t PMem_AllocFromSource_NoDebug;

typedef void(*Cbuf_AddText_t)(int clientNum, const char* text);
extern Cbuf_AddText_t Cbuf_AddText;

void ResolveFunctions();
void AWMPFunctions();
void WW2MPFunctions();