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

#if SDL_VIDEO_DRIVER_WIIU

#include "SDL_wiiu_gfx_heap.h"

#include <stdio.h>
#include <malloc.h>

#include <coreinit/memheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memexpheap.h>
#include <coreinit/memfrmheap.h>

#define FRM_HEAP_STATE_TAG 0x53444C32 // 'SDL2'

static MEMHeapHandle GfxHeap_MEM1 = NULL;
static MEMHeapHandle GfxHeap_Foreground = NULL;

int WIIU_GfxHeap_MEM1Init(void)
{
    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    uint32_t size;
    void *base;

    if (!MEMRecordStateForFrmHeap(heap, FRM_HEAP_STATE_TAG)) {
        printf("%s: MEMRecordStateForFrmHeap failed\n", __FUNCTION__);
        return -1;
    }

    size = MEMGetAllocatableSizeForFrmHeapEx(heap, 4);
    if (!size) {
        printf("%s: MEMGetAllocatableSizeForFrmHeapEx == 0\n", __FUNCTION__);
        return -1;
    }

    base = MEMAllocFromFrmHeapEx(heap, size, 4);
    if (!base) {
        printf("%s: MEMAllocFromFrmHeapEx(heap, 0x%X, 4) failed\n", __FUNCTION__, size);
        return -1;
    }

    GfxHeap_MEM1 = MEMCreateExpHeapEx(base, size, 0);
    if (!GfxHeap_MEM1) {
        printf("%s: MEMCreateExpHeapEx(%p, 0x%X, 0) failed\n", __FUNCTION__, base, size);
        return -1;
    }

   return 0;
}

int WIIU_GfxHeap_MEM1Destroy(void)
{
    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);

    if (GfxHeap_MEM1) {
        MEMDestroyExpHeap(GfxHeap_MEM1);
        GfxHeap_MEM1 = NULL;
    }

    MEMFreeByStateToFrmHeap(heap, FRM_HEAP_STATE_TAG);
    return 0;
}

int WIIU_GfxHeap_ForegroundInit(void)
{
    MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);
    uint32_t size;
    void *base;

    size = MEMGetAllocatableSizeForFrmHeapEx(heap, 4);
    if (!size) {
        printf("%s: MEMAllocFromFrmHeapEx(heap, 0x%X, 4)\n", __FUNCTION__, size);
        return -1;
    }

    base = MEMAllocFromFrmHeapEx(heap, size, 4);
    if (!base) {
        printf("%s: MEMGetAllocatableSizeForFrmHeapEx == 0\n", __FUNCTION__);
        return -1;
    }

    GfxHeap_Foreground = MEMCreateExpHeapEx(base, size, 0);
    if (!GfxHeap_Foreground) {
        printf("%s: MEMCreateExpHeapEx(%p, 0x%X, 0)\n", __FUNCTION__, base, size);
        return -1;
    }

    return 0;
}

int WIIU_GfxHeap_ForegroundDestroy(void)
{
    MEMHeapHandle foreground = MEMGetBaseHeapHandle(MEM_BASE_HEAP_FG);

    if (GfxHeap_Foreground) {
        MEMDestroyExpHeap(GfxHeap_Foreground);
        GfxHeap_Foreground = NULL;
    }

    MEMFreeToFrmHeap(foreground, MEM_FRM_HEAP_FREE_ALL);
    return 0;
}

void *WIIU_GfxHeap_MEM1Alloc(uint32_t align, uint32_t size)
{
    void *block;

    if (!GfxHeap_MEM1) {
        return NULL;
    }

    if (align < 4) {
        align = 4;
    }

    block = MEMAllocFromExpHeapEx(GfxHeap_MEM1, size, align);
    return block;
}

void WIIU_GfxHeap_MEM1Free(void *block)
{
    if (!GfxHeap_MEM1) {
        return;
    }

    MEMFreeToExpHeap(GfxHeap_MEM1, block);
}

void *WIIU_GfxHeap_ForegroundAlloc(uint32_t align, uint32_t size)
{
    void *block;

    if (!GfxHeap_Foreground) {
        return NULL;
    }

    if (align < 4) {
        align = 4;
    }

    block = MEMAllocFromExpHeapEx(GfxHeap_Foreground, size, align);
    return block;
}

void WIIU_GfxHeap_ForegroundFree(void *block)
{
    if (!GfxHeap_Foreground) {
        return;
    }

    MEMFreeToExpHeap(GfxHeap_Foreground, block);
}

void *WIIU_GfxHeap_GX2RAlloc(GX2RResourceFlags flags, uint32_t size, uint32_t alignment)
{
    // Color, depth, scan buffers all belong in MEM1
    if ((flags & (GX2R_RESOURCE_BIND_COLOR_BUFFER
                | GX2R_RESOURCE_BIND_DEPTH_BUFFER
                | GX2R_RESOURCE_BIND_SCAN_BUFFER
                | GX2R_RESOURCE_USAGE_FORCE_MEM1))
        && !(flags & GX2R_RESOURCE_USAGE_FORCE_MEM2)) {
        return WIIU_GfxHeap_MEM1Alloc(alignment, size);
    } else {
        return memalign(alignment, size);
    }
}

void WIIU_GfxHeap_GX2RFree(GX2RResourceFlags flags, void *block)
{
    if ((flags & (GX2R_RESOURCE_BIND_COLOR_BUFFER
                | GX2R_RESOURCE_BIND_DEPTH_BUFFER
                | GX2R_RESOURCE_BIND_SCAN_BUFFER
                | GX2R_RESOURCE_USAGE_FORCE_MEM1))
        && !(flags & GX2R_RESOURCE_USAGE_FORCE_MEM2)) {
        return WIIU_GfxHeap_MEM1Free(block);
    } else {
        return free(block);
    }
}

#endif /* SDL_VIDEO_DRIVER_WIIU */
