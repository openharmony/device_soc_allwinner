/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DISPLAY_GRALLOC_INTERNAL_H
#define DISPLAY_GRALLOC_INTERNAL_H
#include "display_type.h"
#ifdef __cplusplus
extern "C" {
#endif
#define GRALLOC_PRIV_NUM_FDS 0
#define GRALLOC_PRIV_NUM_INTS ((sizeof(PriBufferHandle) - sizeof(BufferHandle)) / sizeof(int) - GRALLOC_PRIV_NUM_FDS)

#define INVALID_PIXEL_FMT 0
#define MAX_PLANES 3

typedef struct plane_info {
    uint32_t offset;
    uint32_t byte_stride;
    uint32_t alloc_width;
    uint32_t alloc_height;
}plane_info_t;

typedef struct {
    BufferHandle hdl;
    int yuv_info;
    int plan_num;
    plane_info_t plane_info[MAX_PLANES];
    uint64_t id;
} PriBufferHandle;

#ifdef __cplusplus
}
#endif

#endif