/*
  Simple DirectMedia Layer
  Copyright (c) 2015-present devkitPro, wut Authors
  Copyright (C) 2022 GaryOderNichts <garyodernichts@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifndef SDL_wiiu_gfx_heap_h
#define SDL_wiiu_gfx_heap_h

#if SDL_VIDEO_DRIVER_WIIU

#include <gx2r/mem.h>

int WIIU_GfxHeap_MEM1Init(void);

int WIIU_GfxHeap_MEM1Destroy(void);

int WIIU_GfxHeap_ForegroundInit(void);

int WIIU_GfxHeap_ForegroundDestroy(void);

void *WIIU_GfxHeap_MEM1Alloc(uint32_t align, uint32_t size);

void WIIU_GfxHeap_MEM1Free(void *block);

void *WIIU_GfxHeap_ForegroundAlloc(uint32_t align, uint32_t size);

void WIIU_GfxHeap_ForegroundFree(void *block);

void *WIIU_GfxHeap_GX2RAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t alignment);

void WIIU_GfxHeap_GX2RFree(GX2RResourceFlags flags, void *block);

#endif /* SDL_VIDEO_DRIVER_WIIU */

#endif /* SDL_wiiu_gfx_heap_h */
