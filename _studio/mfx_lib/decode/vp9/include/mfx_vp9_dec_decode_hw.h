// Copyright (c) 2014-2020 Intel Corporation
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

#ifndef _MFX_VP9_DECODE_HW_H_
#define _MFX_VP9_DECODE_HW_H_

#include "mfx_common.h"

#if defined(MFX_ENABLE_VP9_VIDEO_DECODE)

#include "mfx_vp9_dec_decode.h"
#include "mfx_vp9_dec_decode_utils.h"
#include "mfx_umc_alloc_wrapper.h"
#include "mfx_task.h"
#include "mfx_critical_error_handler.h"
#include "umc_mutex.h"
#include "umc_vp9_dec_defs.h"
#include "umc_vp9_frame.h"
#include <list>
#include <set>
#include <deque>

namespace UMC_VP9_DECODER { class Packer; }


class FrameStorage;

class VideoDECODEVP9_HW : public VideoDECODE, public MfxCriticalErrorHandler
{
public:

    VideoDECODEVP9_HW(VideoCORE *pCore, mfxStatus *sts);
    virtual ~VideoDECODEVP9_HW();

    static mfxStatus Query(VideoCORE *pCore, mfxVideoParam *pIn, mfxVideoParam *pOut);
    static mfxStatus QueryIOSurf(VideoCORE *pCore, mfxVideoParam *pPar, mfxFrameAllocRequest *pRequest);
    static mfxStatus QueryImplsDescription(VideoCORE&, mfxDecoderDescription::decoder&, mfx::PODArraysHolder&);

    virtual mfxStatus Init(mfxVideoParam *par) override;
    virtual mfxStatus Reset(mfxVideoParam *pPar) override;
    virtual mfxStatus Close() override;

    virtual mfxTaskThreadingPolicy GetThreadingPolicy() override;
    virtual mfxStatus GetVideoParam(mfxVideoParam *pPar) override;
    virtual mfxStatus GetDecodeStat(mfxDecodeStat *pStat) override;

    static mfxStatus DecodeHeader(VideoCORE * core, mfxBitstream *bs, mfxVideoParam *params);
    virtual mfxStatus DecodeFrameCheck(mfxBitstream *bs, mfxFrameSurface1 *pSurfaceWork, mfxFrameSurface1 **ppSurfaceOut, MFX_ENTRY_POINT *pEntryPoint) override;

    virtual mfxStatus GetUserData(mfxU8 *pUserData, mfxU32 *pSize, mfxU64 *pTimeStamp);
    virtual mfxStatus GetPayload(mfxU64 *pTimeStamp, mfxPayload *pPayload) override;
    virtual mfxStatus SetSkipMode(mfxSkipMode mode) override;

    virtual mfxFrameSurface1* GetSurface() override;

protected:
    void CalculateTimeSteps(mfxFrameSurface1 *);
    static mfxStatus QueryIOSurfInternal(eMFXPlatform, mfxVideoParam *, mfxFrameAllocRequest *);

    mfxStatus UpdateRefFrames();
    mfxStatus CleanRefList();

    mfxStatus DecodeSuperFrame(mfxBitstream *in, UMC_VP9_DECODER::VP9DecoderFrame & info);
    mfxStatus DecodeFrameHeader(mfxBitstream *in, UMC_VP9_DECODER::VP9DecoderFrame & info);
    mfxStatus PackHeaders(mfxBitstream *bs, UMC_VP9_DECODER::VP9DecoderFrame const & info);

    mfxStatus GetOutputSurface(mfxFrameSurface1 **, mfxFrameSurface1 *, UMC::FrameMemID);

private:
    bool                    m_isInit;
    VideoCORE*              m_core;
    eMFXPlatform            m_platform;

    mfxVideoParamWrapper    m_vInitPar;
    mfxVideoParamWrapper    m_vPar;

    mfxU32                  m_num_output_frames;
    mfxF64                  m_in_framerate;
    mfxU16                  m_frameOrder;
    mfxU32                  m_statusReportFeedbackNumber;

    UMC::Mutex              m_mGuard;
    std::deque<UMC::Mutex>  m_mCopyGuard; //For handling repeated frames

    bool                    m_adaptiveMode;
    mfxU32                  m_index;

    std::unique_ptr<SurfaceSource>  m_surface_source;


    std::unique_ptr<UMC_VP9_DECODER::Packer>  m_Packer;
    std::unique_ptr<FrameStorage> m_framesStorage;

    mfxFrameAllocRequest     m_request;
    mfxFrameAllocResponse    m_response;
    mfxFrameAllocResponse    m_response_alien;
    mfxDecodeStat            m_stat;

    friend mfxStatus MFX_CDECL VP9DECODERoutine(void *p_state, void *pp_param, mfxU32 thread_number, mfxU32);
    friend mfxStatus VP9CompleteProc(void *p_state, void *pp_param, mfxStatus);

    mfxStatus ReportDecodeStatus(mfxFrameSurface1* surface_work);

    void ResetFrameInfo();

    mfxStatus PrepareInternalSurface(UMC::FrameMemID &mid);

    struct VP9DECODERoutineData
    {
        VideoDECODEVP9_HW* decoder;
        mfxFrameSurface1* surface_work;
        UMC::FrameMemID currFrameId;
        UMC::FrameMemID copyFromFrame;
        mfxU32     index;
        mfxU32 showFrame;
    };

    UMC::VideoAccelerator * m_va;

    typedef std::list<mfxFrameSurface1 *> StatuReportList;
    StatuReportList m_completedList;

    UMC_VP9_DECODER::VP9DecoderFrame m_frameInfo;

    typedef struct {
        mfxU32 width;
        mfxU32 height;
    } SizeOfFrame;

    SizeOfFrame m_firstSizes;

    SizeOfFrame m_sizesOfRefFrame[UMC_VP9_DECODER::NUM_REF_FRAMES];
    
    mfxBitstream m_bs;

    mfxI32 m_baseQIndex;
};

#endif // MFX_ENABLE_VP9_VIDEO_DECODE
#endif // _MFX_VP9_DECODE_HW_H_
