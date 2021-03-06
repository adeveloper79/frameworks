/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NU_CACHED_SOURCE_2_H_

#define NU_CACHED_SOURCE_2_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AHandlerReflector.h>
#include <media/stagefright/DataSource.h>

namespace android {

struct ALooper;
struct PageCache;

struct NuCachedSource2 : public DataSource {
    static sp<NuCachedSource2> Create(
            const sp<DataSource> &source,
            const char *cacheConfig = NULL,
            bool disconnectAtHighwatermark = false);

    virtual status_t initCheck() const;

    virtual ssize_t readAt(off64_t offset, void *data, size_t size);

    virtual void disconnect();

    virtual status_t getSize(off64_t *size);
    virtual uint32_t flags();

    virtual sp<DecryptHandle> DrmInitialization(const char* mime);
    virtual void getDrmInfo(sp<DecryptHandle> &handle, DrmManagerClient **client);
    virtual String8 getUri();

    virtual String8 getMIMEType() const;

    ////////////////////////////////////////////////////////////////////////////
#ifndef  MTK_AOSP_ENHANCEMENT
    size_t cachedSize();
    size_t approxDataRemaining(status_t *finalStatus) const;

    void resumeFetchingIfNecessary();

    // The following methods are supported only if the
    // data source is HTTP-based; otherwise, ERROR_UNSUPPORTED
    // is returned.
    status_t getEstimatedBandwidthKbps(int32_t *kbps);
    status_t setCacheStatCollectFreq(int32_t freqMs);
#endif

    static void RemoveCacheSpecificHeaders(
            KeyedVector<String8, String8> *headers,
            String8 *cacheConfig,
            bool *disconnectAtHighwatermark);

protected:
    virtual ~NuCachedSource2();

private:
    friend struct AHandlerReflector<NuCachedSource2>;

    NuCachedSource2(
            const sp<DataSource> &source,
            const char *cacheConfig,
            bool disconnectAtHighwatermark
#ifdef MTK_AOSP_ENHANCEMENT
            ,off64_t cacheOffset = 0
#endif
            );

    enum {
        kPageSize                       = 65536,
        kDefaultHighWaterThreshold      = 20 * 1024 * 1024,
        kDefaultLowWaterThreshold       = 4 * 1024 * 1024,

        // Read data after a 15 sec timeout whether we're actively
        // fetching or not.
        kDefaultKeepAliveIntervalUs     = 15000000,
    };

    enum {
        kWhatFetchMore  = 'fetc',
        kWhatRead       = 'read',
#ifdef MTK_AOSP_ENHANCEMENT
        kWhatRestartCache = 'rstc',
#endif
    };

    enum {
        kMaxNumRetries = 10,
    };

    sp<DataSource> mSource;
    sp<AHandlerReflector<NuCachedSource2> > mReflector;
    sp<ALooper> mLooper;

    Mutex mSerializer;
    mutable Mutex mLock;
    Condition mCondition;

    PageCache *mCache;
    off64_t mCacheOffset;
    status_t mFinalStatus;
    off64_t mLastAccessPos;
    sp<AMessage> mAsyncResult;
    bool mFetching;
    bool mDisconnecting;
    int64_t mLastFetchTimeUs;

    int32_t mNumRetriesLeft;

    size_t mHighwaterThresholdBytes;
    size_t mLowwaterThresholdBytes;

    // If the keep-alive interval is 0, keep-alives are disabled.
    int64_t mKeepAliveIntervalUs;

    bool mDisconnectAtHighwatermark;

    void onMessageReceived(const sp<AMessage> &msg);
    void onFetch();
    void onRead(const sp<AMessage> &msg);

    void fetchInternal();
    ssize_t readInternal(off64_t offset, void *data, size_t size);
    status_t seekInternal_l(off64_t offset);

    size_t approxDataRemaining_l(status_t *finalStatus) const;

    void restartPrefetcherIfNecessary_l(
            bool ignoreLowWaterThreshold = false, bool force = false);

    void updateCacheParamsFromSystemProperty();
    void updateCacheParamsFromString(const char *s);

    DISALLOW_EVIL_CONSTRUCTORS(NuCachedSource2);
#ifdef MTK_AOSP_ENHANCEMENT

public:
    NuCachedSource2();
    // [Feature:non-interleave]
    virtual size_t cachedSize();
    virtual size_t approxDataRemaining(status_t *finalStatus) const;

    virtual void resumeFetchingIfNecessary();

    virtual void resumeFetchingIfNecessary2();
    // The following methods are supported only if the
    // data source is HTTP-based; otherwise, ERROR_UNSUPPORTED
    // is returned.
    virtual status_t getEstimatedBandwidthKbps(int32_t *kbps);
    virtual status_t setCacheStatCollectFreq(int32_t freqMs);
    bool estimateBandwidth(int32_t *kbps);
    int64_t getMaxCacheSize();
    status_t getRealFinalStatus() {return mFinalStatus;};
    void finishCache() { mDying = true; };
    void setInterleaveMode(bool isInterleave, double factor);
    void setOffsetLimit(off64_t limit);
    bool disconnectAtHighwatermark() {return mDisconnectAtHighwatermark;};
    String8 getConfigStr() {return mConfigStr;};

private:
    struct {
    bool mCacheMissing;
    off64_t mMissingOffset;
    size_t mMissingSize;
    } mTryReadState;

    String8 mConfigStr;
    bool mDying;  // interrupt operations for quick resetting when it is set true
    bool mInterleave;  // indicate whether non-interleave mode or not
    off64_t mOffsetLimit;
    bool mIsCacheMissed;     // used for approxDataRemaining_l
    void onRestartCache(const sp<AMessage> &msg);   // restart the cache offset
    ssize_t tryRead_l(off64_t offset, size_t size);
    void checkTryReadState();
    void showBW();
    void init(const char *cacheConfig, off64_t cacheOffset);
#endif
};

}  // namespace android

#endif  // NU_CACHED_SOURCE_2_H_
