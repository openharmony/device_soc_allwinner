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

#include "seed_gbm.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <securec.h>
#include "display_common.h"
#include "seed_gbm_internal.h"

typedef struct {
    uint32_t wPixelAlign;
    uint32_t hPixelAlign;
} PlaneLayoutInfo;

typedef struct {
    uint32_t format;
    uint32_t bitsPerPixel;
    uint32_t numPlanes;
    const PlaneLayoutInfo *planes;
} FormatInfo;

static const PlaneLayoutInfo g_defaultLayout = {
    32,1
};

static const PlaneLayoutInfo g_yuvLayout = {
    16,1
};

static const FormatInfo *GetFormatInfo(uint32_t format)
{
    static const FormatInfo fmtInfos[] = {
        {DRM_FORMAT_RGBX8888,  32, 1, &g_defaultLayout},  {DRM_FORMAT_RGBA8888, 32,  1, &g_defaultLayout},
        {DRM_FORMAT_BGRX8888,  32, 1, &g_defaultLayout},  {DRM_FORMAT_BGRA8888, 32,  1, &g_defaultLayout},
        {DRM_FORMAT_RGB888,    24, 1, &g_defaultLayout},  {DRM_FORMAT_RGB565,  16, 1, &g_defaultLayout},
        {DRM_FORMAT_BGRX4444,  16, 1, &g_defaultLayout},  {DRM_FORMAT_BGRA4444, 16,  1, &g_defaultLayout},
        {DRM_FORMAT_RGBA4444,  16, 1, &g_defaultLayout},  {DRM_FORMAT_RGBX4444, 16,  1, &g_defaultLayout},
        {DRM_FORMAT_BGRX5551,  16, 1, &g_defaultLayout},  {DRM_FORMAT_BGRA5551, 16,  1, &g_defaultLayout},
        {DRM_FORMAT_NV12, 12, 2, &g_yuvLayout}, {DRM_FORMAT_NV21, 12, 2, &g_yuvLayout},
        {DRM_FORMAT_NV16, 16, 2, &g_yuvLayout},  {DRM_FORMAT_NV61, 16, 2, &g_yuvLayout},
        {DRM_FORMAT_YUV420, 12, 3, &g_yuvLayout}, {DRM_FORMAT_YVU420, 12, 3, &g_yuvLayout},
        {DRM_FORMAT_YUV422, 16, 3, &g_yuvLayout}, {DRM_FORMAT_YVU422, 16, 3, &g_yuvLayout},
    };

    for (uint32_t i = 0; i < sizeof(fmtInfos) / sizeof(FormatInfo); i++) {
        if (fmtInfos[i].format == format) {
            return &fmtInfos[i];
        }
    }
    DISPLAY_LOGE("the format can not support");
    return NULL;
}

void InitGbmBo(struct gbm_bo *bo, const struct drm_mode_create_dumb *dumb)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((dumb == NULL), DISPLAY_LOGE("dumb is null"));
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == NULL), DISPLAY_LOGE("bo is null"));
    bo->stride = dumb->pitch;
    bo->size = dumb->size;
    bo->handle = dumb->handle;
}

static bool AdjustStrideFromFormat(const FormatInfo *fmtInfo, uint32_t *heightStride, uint32_t *widthStride)
{
    if (fmtInfo != NULL) {
        *heightStride = ALIGN_UP((*heightStride), fmtInfo->planes->hPixelAlign);
        *widthStride = ALIGN_UP((*widthStride), fmtInfo->planes->wPixelAlign);
        return true;
    }
    return false;
}

struct gbm_bo *hdi_gbm_bo_create(struct gbm_device *gbm, uint32_t width, uint32_t height, uint32_t format,
    uint32_t usage)
{
    DISPLAY_UNUSED(usage);
    int ret;
    struct drm_mode_create_dumb dumb = { 0 };
    const FormatInfo *fmtInfo = GetFormatInfo(format);
    DISPLAY_CHK_RETURN((fmtInfo == NULL), NULL, DISPLAY_LOGE("formt: 0x%{public}x can not get layout info", format));
    struct gbm_bo *bo  = (struct gbm_bo *)calloc(1, sizeof(struct gbm_bo));
    DISPLAY_CHK_RETURN((bo == NULL), NULL, DISPLAY_LOGE("gbm bo create fialed no memery"));
    (void)memset_s(bo, sizeof(struct gbm_bo), 0, sizeof(struct gbm_bo));
    AdjustStrideFromFormat(fmtInfo, &height, &width);
    bo->width = width;
    bo->height = height;
    bo->gbm = gbm;
    bo->format = format;
    dumb.height = height;
    dumb.width = width;
    dumb.flags = 0;
    dumb.bpp = fmtInfo->bitsPerPixel;
    ret = drmIoctl(gbm->fd, DRM_IOCTL_MODE_CREATE_DUMB, &dumb);
    DISPLAY_LOGI("fmt 0x%{public}x create dumb width: %{public}d  height: %{public}d bpp: %{public}u pitch %{public}d "
        "size %{public}llu",
        format, dumb.width, dumb.height, dumb.bpp, dumb.pitch, dumb.size);
    DISPLAY_CHK_RETURN((ret != 0), NULL, DISPLAY_LOGE("DRM_IOCTL_MODE_CREATE_DUMB failed errno %{public}d", errno));
    InitGbmBo(bo, &dumb);
    DISPLAY_LOGI(
        "fmt 0x%{public}x create dumb width: %{public}d  height: %{public}d  stride %{public}d size %{public}u", format,
        bo->width, bo->height, bo->stride, bo->size);
    return bo;
}

struct gbm_device *hdi_gbm_create_device(int fd)
{
    struct gbm_device *gbm;
    gbm = (struct gbm_device *)calloc(1, sizeof(struct gbm_device));
    DISPLAY_CHK_RETURN((gbm == NULL), NULL, DISPLAY_LOGE("memory calloc failed"));
    gbm->fd = fd;
    return gbm;
}

void hdi_gbm_device_destroy(struct gbm_device *gbm)
{
    free(gbm);
}

uint32_t hdi_gbm_bo_get_stride(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == NULL), 0, DISPLAY_LOGE("the bo is null"));
    return bo->stride;
}

uint32_t hdi_gbm_bo_get_width(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == NULL), 0, DISPLAY_LOGE("the bo is null"));
    return bo->width;
}

uint32_t hdi_gbm_bo_get_height(struct gbm_bo *bo)
{
    DISPLAY_CHK_RETURN((bo == NULL), 0, DISPLAY_LOGE("the bo is null"));
    return bo->height;
}

void hdi_gbm_bo_destroy(struct gbm_bo *bo)
{
    int ret;
    DISPLAY_CHK_RETURN_NOT_VALUE((bo == NULL), DISPLAY_LOGE("the bo is null"));
    struct drm_mode_destroy_dumb dumb = { 0 };
    dumb.handle = bo->handle;
    ret = drmIoctl(bo->gbm->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dumb);
    DISPLAY_CHK_RETURN_NOT_VALUE((ret), DISPLAY_LOGE("dumb buffer destroy failed errno %{public}d", errno));
    free(bo);
}

int hdi_gbm_bo_get_fd(struct gbm_bo *bo)
{
    int fd, ret;
    ret = drmPrimeHandleToFD(bo->gbm->fd, bo->handle, DRM_CLOEXEC | DRM_RDWR, &fd);
    DISPLAY_CHK_RETURN((ret), -1,
        DISPLAY_LOGE("drmPrimeHandleToFD  failed ret: %{public}d  errno: %{public}d", ret, errno));
    return fd;
}
