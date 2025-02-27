// Copyright (c) 2019-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mfx_common.h"
#if defined(MFX_ENABLE_H265_VIDEO_ENCODE)

#include "hevcehw_base_lin.h"
#include "hevcehw_base_data.h"
#include "hevcehw_base_legacy.h"
#include "hevcehw_base_parser.h"
#include "hevcehw_base_recon_info_lin.h"
#include "hevcehw_base_packer.h"
#include "hevcehw_base_hrd.h"
#include "hevcehw_base_alloc.h"
#include "hevcehw_base_task.h"
#include "hevcehw_base_ext_brc.h"
#include "hevcehw_base_dirty_rect_lin.h"
#include "hevcehw_base_hdr_sei.h"
#include "hevcehw_base_va_lin.h"
#include "hevcehw_base_va_packer_lin.h"
#include "hevcehw_base_interlace_lin.h"
#include "hevcehw_base_encoded_frame_info_lin.h"
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
#include "hevcehw_base_weighted_prediction_lin.h"
#endif
#if defined(MFX_ENABLE_HEVCE_ROI)
#include "hevcehw_base_roi_lin.h"
#endif
#include "hevcehw_base_max_frame_size_lin.h"
#include "hevcehw_base_query_impl_desc.h"
#ifdef MFX_ENABLE_ENCTOOLS
#include "hevcehw_base_enctools.h"
#endif
#include <algorithm>

using namespace HEVCEHW;
using namespace HEVCEHW::Base;

Linux::Base::MFXVideoENCODEH265_HW::MFXVideoENCODEH265_HW(
    VideoCORE& core
    , mfxStatus& status
    , eFeatureMode mode)
    : HEVCEHW::Base::MFXVideoENCODEH265_HW(core)
{
    status = MFX_ERR_UNKNOWN;
    auto vaType = core.GetVAType();

    m_features.emplace_back(new Parser(FEATURE_PARSER));
    m_features.emplace_back(new Allocator(FEATURE_ALLOCATOR));

    if (vaType == MFX_HW_VAAPI)
    {
        m_features.emplace_back(new DDI_VA(FEATURE_DDI));
    }
    else
    {
        status = MFX_ERR_UNSUPPORTED;
        return;
    }

    m_features.emplace_back(new VAPacker(FEATURE_DDI_PACKER));
    m_features.emplace_back(new Legacy(FEATURE_LEGACY));
    m_features.emplace_back(new ReconInfo(FEATURE_RECON_INFO));
    m_features.emplace_back(new HRD(FEATURE_HRD));
    m_features.emplace_back(new TaskManager(FEATURE_TASK_MANAGER));
    m_features.emplace_back(new Packer(FEATURE_PACKER));
    m_features.emplace_back(new ExtBRC(FEATURE_EXT_BRC));
    m_features.emplace_back(new DirtyRect(FEATURE_DIRTY_RECT));
    m_features.emplace_back(new HdrSei(FEATURE_HDR_SEI));
    m_features.emplace_back(new Interlace(FEATURE_INTERLACE));
    m_features.emplace_back(new EncodedFrameInfo(FEATURE_ENCODED_FRAME_INFO));
#if defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
    m_features.emplace_back(new WeightPred(FEATURE_WEIGHTPRED));
#endif //defined(MFX_ENABLE_HEVCE_WEIGHTED_PREDICTION)
#if defined(MFX_ENABLE_HEVCE_ROI)
    m_features.emplace_back(new ROI(FEATURE_ROI));
#endif
    m_features.emplace_back(new MaxFrameSize(FEATURE_MAX_FRAME_SIZE));
    m_features.emplace_back(new QueryImplDesc(FEATURE_QUERY_IMPL_DESC));
#if defined(MFX_ENABLE_ENCTOOLS)
    m_features.emplace_back(new HevcEncTools(FEATURE_ENCTOOLS));
#endif

    InternalInitFeatures(status, mode);

    if (mode & INIT)
    {
        auto& qIA = BQ<BQ_InitAlloc>::Get(*this);
        Reorder(qIA
            , { FEATURE_LEGACY, Legacy::BLK_AllocRaw }
            , { FEATURE_RECON_INFO, ReconInfo::BLK_AllocRec }
        , PLACE_AFTER);
        Reorder(qIA
            , { FEATURE_RECON_INFO, ReconInfo::BLK_AllocRec }
            , { FEATURE_DDI, IDDI::BLK_CreateService }
        , PLACE_AFTER);
        qIA.splice(qIA.end(), qIA, Get(qIA, { FEATURE_DIRTY_RECT, DirtyRect::BLK_SetCallChains }));
#if defined(MFX_ENABLE_HEVCE_ROI)
        qIA.splice(qIA.end(), qIA, Get(qIA, { FEATURE_ROI, ROI::BLK_SetCallChains }));
#endif
    }
}

mfxStatus Linux::Base::MFXVideoENCODEH265_HW::Init(mfxVideoParam *par)
{
    mfxStatus sts = HEVCEHW::Base::MFXVideoENCODEH265_HW::Init(par);
    MFX_CHECK(sts >= MFX_ERR_NONE, sts);

    return sts;
}

#endif //defined(MFX_ENABLE_H265_VIDEO_ENCODE)
