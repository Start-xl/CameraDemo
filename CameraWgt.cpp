#include "CameraWgt.h"
#include "ui_CameraWgt.h"
#include <QDebug>
#include <QPainter>
#include <QTimer>
#include <QtGlobal>

#define DEFAULT_SHOW_RATE (30) // 默认显示帧率 | defult display frequency
#define DEFAULT_ERROR_STRING ("N/A")
#define MAX_FRAME_STAT_NUM (50)
#define MIN_LEFT_LIST_NUM (2)
#define MAX_STATISTIC_INTERVAL (5000000000) // List的更新时与最新一帧的时间最大间隔 |  The maximum time interval between the update of list and the latest frame

// 取流回调函数
// get frame callback function
static void FrameCallback(IMV_Frame* pFrame, void* pUser)
{
    CameraWgt* pCammerWidget = (CameraWgt*)pUser;
    if (!pCammerWidget)
    {
        printf("pCammerWidget is NULL!");
        return;
    }

    CFrameInfo frameInfo;
    frameInfo.m_nWidth = (int)pFrame->frameInfo.width;
    frameInfo.m_nHeight = (int)pFrame->frameInfo.height;
    frameInfo.m_nBufferSize = (int)pFrame->frameInfo.size;
    frameInfo.m_nPaddingX = (int)pFrame->frameInfo.paddingX;
    frameInfo.m_nPaddingY = (int)pFrame->frameInfo.paddingY;
    frameInfo.m_ePixelType = pFrame->frameInfo.pixelFormat;
    frameInfo.m_pImageBuf = (unsigned char *)malloc(sizeof(unsigned char) * frameInfo.m_nBufferSize);
    frameInfo.m_nTimeStamp = pFrame->frameInfo.timeStamp;

    // 内存申请失败，直接返回
    // memory application failed, return directly
    if (frameInfo.m_pImageBuf != NULL)
    {
        memcpy(frameInfo.m_pImageBuf, pFrame->pData, frameInfo.m_nBufferSize);

        if (pCammerWidget->m_qDisplayFrameQueue.size() > 16)
        {
            CFrameInfo frameOld;
            if (pCammerWidget->m_qDisplayFrameQueue.get(frameOld))
            {
                free(frameOld.m_pImageBuf);
                frameOld.m_pImageBuf = NULL;
            }
        }

        pCammerWidget->m_qDisplayFrameQueue.push_back(frameInfo);
    }

    pCammerWidget->recvNewFrame(pFrame->frameInfo.size);
}

// 显示线程
// display thread
static unsigned int __stdcall displayThread(void* pUser)
{
    CameraWgt* pCammerWidget = (CameraWgt*)pUser;
    if (!pCammerWidget)
    {
        printf("pCammerWidget is NULL!");
        return -1;
    }

    pCammerWidget->display();

    return 0;
}

CameraWgt::CameraWgt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraWgt)
    , m_currentCameraKey("")
    , m_devHandle(NULL)
    , m_nDisplayInterval(0)
    , m_nFirstFrameTime(0)
    , m_nLastFrameTime(0)
    , m_handler(NULL)
    , m_bNeedUpdate(true)
    , m_nTotalFrameCount(0)
    , m_isExitDisplayThread(false)
    , m_threadHandle(NULL)
    , m_displayWnd(nullptr)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_scaleFactor(1.0)
    , m_offsetX(0)
    , m_offsetY(0)
    , m_isPanning(false)
{
    ui->setupUi(this);

    // 创建一个 native 子窗口，让 VideoRender 渲染到这个子窗口
    m_displayWnd = new QWidget(this);
    m_displayWnd->setAttribute(Qt::WA_NativeWindow);      // 确保有原生窗口句柄
    m_displayWnd->show();

    // 使用子窗口 winId作为 VideoRender的目标窗口
    m_hWnd = (VR_HWND)m_displayWnd->winId();

    // 默认显示30帧
    // defult display 30 frames
    setDisplayFPS(DEFAULT_SHOW_RATE);

    m_elapsedTimer.start();

    // 启动显示线程
    // start display thread
    m_threadHandle = (HANDLE)_beginthreadex(NULL,
                                             0,
                                             displayThread,
                                             this,
                                             CREATE_SUSPENDED,
                                             NULL);

    if (!m_threadHandle) {
        printf("Failed to create display thread!\n");
    } else {
        ResumeThread(m_threadHandle);
        m_isExitDisplayThread = false;
    }
}

CameraWgt::~CameraWgt()
{
    // 关闭显示线程
    // close display thread
    m_isExitDisplayThread = true;
    WaitForSingleObject(m_threadHandle, INFINITE);
    CloseHandle(m_threadHandle);
    m_threadHandle = NULL;

    delete ui;
}

// 设置曝光
// set exposeTime
bool CameraWgt::SetExposeTime(double dExposureTime)
{
    int ret = IMV_OK;

    ret = IMV_SetDoubleFeatureValue(m_devHandle, "ExposureTime", dExposureTime);
    if (IMV_OK != ret)
    {
        printf("set ExposureTime value = %0.2f fail, ErrorCode[%d]\n", dExposureTime, ret);
        return false;
    }

    return true;
}

// 设置增益
// set gain
bool CameraWgt::SetAdjustPlus(double dGainRaw)
{
    int ret = IMV_OK;

    ret = IMV_SetDoubleFeatureValue(m_devHandle, "GainRaw", dGainRaw);
    if (IMV_OK != ret)
    {
        printf("set GainRaw value = %0.2f fail, ErrorCode[%d]\n", dGainRaw, ret);
        return false;
    }

    return true;
}

// 打开相机
// open camera
bool CameraWgt::CameraOpen(void)
{
    int ret = IMV_OK;

    if (m_currentCameraKey.length() == 0) {
        return false;
    }

    if (m_devHandle) {
        return false;
    }
    QByteArray cameraKeyArray = m_currentCameraKey.toLocal8Bit();
    char* cameraKey = cameraKeyArray.data();

    //  创建相机句柄
    ret = IMV_CreateHandle(&m_devHandle, modeByCameraKey, (void*)cameraKey);
    if (IMV_OK != ret) {
        printf("create devHandle failed! cameraKey[%s], ErrorCode[%d]\n", cameraKey, ret);
        return false;
    }

    // 打开相机
    // Open camera
    ret = IMV_Open(m_devHandle);
    if (IMV_OK != ret)
    {
        printf("open camera failed! ErrorCode[%d]\n", ret);
        return false;
    }

    return true;
}

// 关闭相机
// close camera
bool CameraWgt::CameraClose(void)
{
    int ret = IMV_OK;

    if (!m_devHandle)
    {
        printf("close camera fail. No camera.\n");
        return false;
    }

    if (false == IMV_IsOpen(m_devHandle))
    {
        printf("camera is already close.\n");
        return false;
    }

    ret = IMV_Close(m_devHandle);
    if (IMV_OK != ret)
    {
        printf("close camera failed! ErrorCode[%d]\n", ret);
        return false;
    }

    ret = IMV_DestroyHandle(m_devHandle);
    if (IMV_OK != ret)
    {
        printf("destroy devHandle failed! ErrorCode[%d]\n", ret);
        return false;
    }

    m_devHandle = NULL;

    return true;
}

// 开始采集
// start grabbing
bool CameraWgt::CameraStart()
{
    int ret = IMV_OK;

    if (IMV_IsGrabbing(m_devHandle))
    {
        printf("camera is already grebbing.\n");
        return false;
    }

    ret = IMV_AttachGrabbing(m_devHandle, FrameCallback, this);
    if (IMV_OK != ret)
    {
        printf("Attach grabbing failed! ErrorCode[%d]\n", ret);
        return false;
    }

    ret = IMV_StartGrabbing(m_devHandle);
    if (IMV_OK != ret)
    {
        printf("start grabbing failed! ErrorCode[%d]\n", ret);
        return false;
    }

    return true;
}

// 停止采集
// stop grabbing
bool CameraWgt::CameraStop()
{
    int ret = IMV_OK;
    if (!IMV_IsGrabbing(m_devHandle))
    {
        printf("camera is already stop grebbing.\n");
        return false;
    }

    ret = IMV_StopGrabbing(m_devHandle);
    if (IMV_OK != ret)
    {
        printf("Stop grabbing failed! ErrorCode[%d]\n", ret);
        return false;
    }

    // 清空显示队列
    // clear display queue
    CFrameInfo frameOld;
    while (m_qDisplayFrameQueue.get(frameOld))
    {
        free(frameOld.m_pImageBuf);
        frameOld.m_pImageBuf = NULL;
    }

    m_qDisplayFrameQueue.clear();

    return true;
}

// 切换采集方式、触发方式 （连续采集、外部触发、软件触发）
// Switch acquisition mode and triggering mode (continuous acquisition, external triggering and software triggering)
bool CameraWgt::CameraChangeTrig(ETrigType trigType)
{
    int ret = IMV_OK;

    if (trigContinous == trigType)
    {
        // 设置触发模式
        // set trigger mode
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "Off");
        if (IMV_OK != ret)
        {
            printf("set TriggerMode value = Off fail, ErrorCode[%d]\n", ret);
            return false;
        }
    }
    else if (trigSoftware == trigType)
    {
        // 设置触发器
        // set trigger
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
        if (IMV_OK != ret)
        {
            printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
            return false;
        }

        // 设置触发模式
        // set trigger mode
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
        if (IMV_OK != ret)
        {
            printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
            return false;
        }

        // 设置触发源为软触发
        // set triggerSource as software trigger
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Software");
        if (IMV_OK != ret)
        {
            printf("set TriggerSource value = Software fail, ErrorCode[%d]\n", ret);
            return false;
        }
    }
    else if (trigLine == trigType)
    {
        // 设置触发器
        // set trigger
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSelector", "FrameStart");
        if (IMV_OK != ret)
        {
            printf("set TriggerSelector value = FrameStart fail, ErrorCode[%d]\n", ret);
            return false;
        }

        // 设置触发模式
        // set trigger mode
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerMode", "On");
        if (IMV_OK != ret)
        {
            printf("set TriggerMode value = On fail, ErrorCode[%d]\n", ret);
            return false;
        }

        // 设置触发源为Line1触发
        // set trigggerSource as Line1 trigger
        ret = IMV_SetEnumFeatureSymbol(m_devHandle, "TriggerSource", "Line1");
        if (IMV_OK != ret)
        {
            printf("set TriggerSource value = Line1 fail, ErrorCode[%d]\n", ret);
            return false;
        }
    }

    return true;
}

// 执行一次软触发
// execute one software trigger
bool CameraWgt::ExecuteSoftTrig(void)
{
    int ret = IMV_OK;

    ret = IMV_ExecuteCommandFeature(m_devHandle, "TriggerSoftware");
    if (IMV_OK != ret)
    {
        printf("ExecuteSoftTrig fail, ErrorCode[%d]\n", ret);
        return false;
    }

    printf("ExecuteSoftTrig success.\n");
    return true;
}

// 设置当前相机
// set current camera
void CameraWgt::SetCamera(const QString& strKey)
{
    m_currentCameraKey = strKey;
}

// 显示
// diaplay
bool CameraWgt::ShowImage(unsigned char* pRgbFrameBuf, int nWidth, int nHeight, IMV_EPixelType nPixelFormat)
{
    if (NULL == pRgbFrameBuf || nWidth == 0 || nHeight == 0) {
        return false;
    }

    bool firstFrame = false;

    // 第一次收到视频分辨率
    if (m_videoWidth == 0 || m_videoHeight == 0) {
        m_videoWidth  = nWidth;
        m_videoHeight = nHeight;
        firstFrame = true;
    }

    if (NULL == m_handler) {
        if (!openRender(nWidth, nHeight) && NULL == m_handler) {
                return false;
        }
    }

    VR_FRAME_S renderParam = { 0 };
    renderParam.data[0] = pRgbFrameBuf;
    renderParam.stride[0] = nWidth;
    renderParam.nWidth = nWidth;
    renderParam.nHeight = nHeight;
    if (nPixelFormat == gvspPixelMono8) {
        renderParam.format = VR_PIXEL_FMT_MONO8;
    } else {
        renderParam.format = VR_PIXEL_FMT_RGB24;
    }

    if (VR_SUCCESS == VR_RenderFrame(m_handler, &renderParam, NULL)) {
        if (firstFrame) {
            QMetaObject::invokeMethod(this, [this]() {
                updateDisplayGeometry();
            }, Qt::QueuedConnection);
        }
        return true;
    }
    return false;
}

// 显示线程
// display thread
void CameraWgt::display()
{
    while (!m_isExitDisplayThread)
    {
        CFrameInfo frameInfo;

        if (false == m_qDisplayFrameQueue.get(frameInfo))
        {
            Sleep(1);
            continue;
        }

        // 判断是否要显示。超过显示上限（30帧），就不做转码、显示处理
        // Judge whether to display. If the upper display limit (30 frames) is exceeded, transcoding and display processing will not be performed
        if (!isTimeToDisplay())
        {
            // 释放内存
            // release memory
            free(frameInfo.m_pImageBuf);
            continue;
        }

        // mono8格式可不做转码，直接显示，其他格式需要经过转码才能显示
        // mono8 format can be displayed directly without transcoding. Other formats can be displayed only after transcoding
        if (gvspPixelMono8 == frameInfo.m_ePixelType)
        {
            // 显示线程中发送显示信号，在主线程中显示图像
            // Send display signal in display thread and display image in main thread
            if (false == ShowImage(frameInfo.m_pImageBuf, (int)frameInfo.m_nWidth, (int)frameInfo.m_nHeight, frameInfo.m_ePixelType))
            {
                printf("_render.display failed.\n");
            }
            // 释放内存
            // release memory
            free(frameInfo.m_pImageBuf);
        }
        else
        {
            // 转码
            unsigned char *pRGBbuffer = NULL;
            int nRgbBufferSize = 0;
            nRgbBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 3;
            pRGBbuffer = (unsigned char *)malloc(nRgbBufferSize);
            if (pRGBbuffer == NULL)
            {
                // 释放内存
                // release memory
                free(frameInfo.m_pImageBuf);
                printf("RGBbuffer malloc failed.\n");
                continue;
            }

            IMV_PixelConvertParam stPixelConvertParam;
            stPixelConvertParam.nWidth = frameInfo.m_nWidth;
            stPixelConvertParam.nHeight = frameInfo.m_nHeight;
            stPixelConvertParam.ePixelFormat = frameInfo.m_ePixelType;
            stPixelConvertParam.pSrcData = frameInfo.m_pImageBuf;
            stPixelConvertParam.nSrcDataLen = frameInfo.m_nBufferSize;
            stPixelConvertParam.nPaddingX = frameInfo.m_nPaddingX;
            stPixelConvertParam.nPaddingY = frameInfo.m_nPaddingY;
            stPixelConvertParam.eBayerDemosaic = demosaicNearestNeighbor;
            stPixelConvertParam.eDstPixelFormat = gvspPixelBGR8;
            stPixelConvertParam.pDstBuf = pRGBbuffer;
            stPixelConvertParam.nDstBufSize = nRgbBufferSize;

            int ret = IMV_PixelConvert(m_devHandle, &stPixelConvertParam);
            if (IMV_OK != ret)
            {
                // 释放内存
                // release memory
                printf("image convert to BGR failed! ErrorCode[%d]\n", ret);
                free(frameInfo.m_pImageBuf);
                free(pRGBbuffer);
                continue;
            }

            // 释放内存
            // release memory
            free(frameInfo.m_pImageBuf);

            // 显示
            // display
            if (false == ShowImage(pRGBbuffer, stPixelConvertParam.nWidth, stPixelConvertParam.nHeight, stPixelConvertParam.eDstPixelFormat))
            {
                printf("_render.display failed.");
            }
            free(pRGBbuffer);
        }
    }
}

void CameraWgt::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    if (!m_displayWnd || m_videoWidth == 0 || m_videoHeight == 0) {
        return;
    }

    QTimer::singleShot(50, this, [this]() {
        updateDisplayGeometry();
    });
}

bool CameraWgt::isTimeToDisplay()
{
    m_mxTime.lock();

    // 不显示
    // don't display
    if (m_nDisplayInterval <= 0)
    {
        m_mxTime.unlock();
        return false;
    }

    // 第一帧必须显示
    // the frist frame must be displayed
    if (m_nFirstFrameTime == 0 || m_nLastFrameTime == 0)
    {
        m_nFirstFrameTime = m_elapsedTimer.nsecsElapsed();
        m_nLastFrameTime = m_nFirstFrameTime;

        m_mxTime.unlock();
        return true;
    }

    // 当前帧和上一帧的间隔如果大于显示间隔就显示
    // display if the interval between the current frame and the previous frame is greater than the display interval
    uint64_t nCurTimeTmp = m_elapsedTimer.nsecsElapsed();
    uint64_t nAcquisitionInterval = nCurTimeTmp - m_nLastFrameTime;
    if (nAcquisitionInterval > m_nDisplayInterval)
    {
        m_nLastFrameTime = nCurTimeTmp;
        m_mxTime.unlock();
        return true;
    }

    // 当前帧相对于第一帧的时间间隔
    // Time interval between the current frame and the first frame
    uint64_t nPre = (m_nLastFrameTime - m_nFirstFrameTime) % m_nDisplayInterval;
    if (nPre + nAcquisitionInterval > m_nDisplayInterval)
    {
        m_nLastFrameTime = nCurTimeTmp;
        m_mxTime.unlock();
        return true;
    }

    m_mxTime.unlock();
    return false;
}

// 设置显示频率
// set display frequency
void CameraWgt::setDisplayFPS(int nFPS)
{
    m_mxTime.lock();
    if (nFPS > 0)
    {
        m_nDisplayInterval = 1000 * 1000 * 1000.0 / nFPS;
    }
    else
    {
        m_nDisplayInterval = 0;
    }
    m_mxTime.unlock();
}

// 窗口关闭响应函数
// window close response function
void CameraWgt::closeEvent(QCloseEvent * event)
{
    IMV_DestroyHandle(m_devHandle);
    m_devHandle = NULL;
}

// 状态栏统计信息 开始
// Status bar statistics begin
void CameraWgt::resetStatistic()
{
    QMutexLocker locker(&m_mxStatistic);
    m_nTotalFrameCount = 0;
    m_listFrameStatInfo.clear();
    m_bNeedUpdate = true;
}
QString CameraWgt::getStatistic()
{
    if (m_mxStatistic.tryLock(30))
    {
        if (m_bNeedUpdate)
        {
            updateStatistic();
        }

        m_mxStatistic.unlock();
        return m_strStatistic;
    }
    return "";
}
void CameraWgt::updateStatistic()
{
    size_t nFrameCount = m_listFrameStatInfo.size();
    QString strFPS = DEFAULT_ERROR_STRING;
    QString strSpeed = DEFAULT_ERROR_STRING;

    if (nFrameCount > 1)
    {
        quint64 nTotalSize = 0;
        FrameList::const_iterator it = m_listFrameStatInfo.begin();

        if (m_listFrameStatInfo.size() == 2)
        {
            nTotalSize = m_listFrameStatInfo.back().m_nFrameSize;
        }
        else
        {
            for (++it; it != m_listFrameStatInfo.end(); ++it)
            {
                nTotalSize += it->m_nFrameSize;
            }
        }

        const FrameStatInfo& first = m_listFrameStatInfo.front();
        const FrameStatInfo& last = m_listFrameStatInfo.back();

        qint64 nsecs = last.m_nPassTime - first.m_nPassTime;

        if (nsecs > 0)
        {
            double dFPS = (nFrameCount - 1) * ((double)1000000000.0 / nsecs);
            double dSpeed = nTotalSize * ((double)1000000000.0 / nsecs) / (1000.0) / (1000.0) * (8.0);
            strFPS = QString::number(dFPS, 'f', 2);
            strSpeed = QString::number(dSpeed, 'f', 2);
        }
    }

    m_strStatistic = QString("Stream: %1 Images   %2 FPS   %3 Mbps")
                         .arg(m_nTotalFrameCount)
                         .arg(strFPS)
                         .arg(strSpeed);
    m_bNeedUpdate = false;
}
void CameraWgt::recvNewFrame(quint32 frameSize)
{
    QMutexLocker locker(&m_mxStatistic);
    if (m_listFrameStatInfo.size() >= MAX_FRAME_STAT_NUM)
    {
        m_listFrameStatInfo.pop_front();
    }
    m_listFrameStatInfo.push_back(FrameStatInfo(frameSize, m_elapsedTimer.nsecsElapsed()));
    ++m_nTotalFrameCount;

    if (m_listFrameStatInfo.size() > MIN_LEFT_LIST_NUM)
    {
        FrameStatInfo infoFirst = m_listFrameStatInfo.front();
        FrameStatInfo infoLast = m_listFrameStatInfo.back();
        while (m_listFrameStatInfo.size() > MIN_LEFT_LIST_NUM && infoLast.m_nPassTime - infoFirst.m_nPassTime > MAX_STATISTIC_INTERVAL)
        {
            m_listFrameStatInfo.pop_front();
            infoFirst = m_listFrameStatInfo.front();
        }
    }

    m_bNeedUpdate = true;
}

// 状态栏统计信息 end
// Status bar statistics ending
// VedioRender显示相关 start
// VedioRender releative display start
bool CameraWgt::openRender(int width, int height)
{
    if (m_handler != NULL || width == 0 || height == 0) {
        return false;
    }

    memset(&m_params, 0, sizeof(m_params));
    m_params.eVideoRenderMode = VR_MODE_GDI;
    m_params.hWnd = m_hWnd;
    m_params.nWidth = width;
    m_params.nHeight = height;

    VR_ERR_E ret = VR_Open(&m_params, &m_handler);
    if (ret != VR_SUCCESS) {
        if (ret == VR_NOT_SUPPORT) {
            printf("%s can't display YUV on this computer\n", __FUNCTION__);
        } else {
            printf("%s open failed. error code[%d]\n", __FUNCTION__, ret);
        }
        return false;
    }

    return true; // 成功返回 true
}

bool CameraWgt::closeRender()
{
    if (m_handler != NULL)
    {
        VR_Close(m_handler);
        m_handler = NULL;
    }
    return true;
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 重写鼠标滚轮操作
 * @date 2025-08-23
 */
void CameraWgt::wheelEvent(QWheelEvent *event) {
    if (!m_displayWnd || m_videoWidth == 0 || m_videoHeight == 0) {
        return;
    }
    // 获取鼠标位置
    QPoint mousePos = event->position().toPoint();
    // 获取当前显示窗口的位置
    QRect currentRect = m_displayWnd->geometry();
    // 计算鼠标在显示窗口中的相对位置（0-1）
    float relativeX = 0.5f;
    float relativeY = 0.5f;

    if (currentRect.contains(mousePos)) {
        relativeX = (mousePos.x() - currentRect.x()) / (float)currentRect.width();
        relativeY = (mousePos.y() - currentRect.y()) / (float)currentRect.height();
    }
    float oldScale = m_scaleFactor;

    // 滚轮方向：向上放大，向下缩小
    if (event->angleDelta().y() > 0) {
        m_scaleFactor *= 1.1f;  // 放大 10%
    } else {
        m_scaleFactor /= 1.1f;  // 缩小 10%
    }

    // 限制缩放范围，避免太大或太小
    if (m_scaleFactor < 0.1f) m_scaleFactor = 0.1f;
    if (m_scaleFactor > 5.0f) m_scaleFactor = 5.0f;

    if (oldScale != m_scaleFactor) {
        // 计算新的显示大小
        float baseScale = qMin(width() / (float)m_videoWidth, height() / (float)m_videoHeight);
        float newScale = baseScale * m_scaleFactor;
        int newWidth = static_cast<int>(m_videoWidth * newScale);
        int newHeight = static_cast<int>(m_videoHeight * newScale);

        // 如果缩放后图像小于窗口，重置偏移为0（居中显示）
        if (newWidth <= width()) {
            m_offsetX = 0;
        } else {
            // 调整偏移量，使鼠标下的点保持不动
            float widthDelta = newWidth - currentRect.width();
            m_offsetX -= widthDelta * relativeX;
        }
        if (newHeight <= height()) {
            m_offsetY = 0;
        } else {
            float heightDelta = newHeight - currentRect.height();
            m_offsetY -= heightDelta * relativeY;
        }
        updateDisplayGeometry();
    }
}

void CameraWgt::updateDisplayGeometry() {
    if(!m_displayWnd || m_videoWidth == 0 || m_videoHeight == 0) {
        return;
    }

    // 计算基础缩放 (适应窗口)
    float baseScale = qMin(width() / (float)m_videoWidth, height() / (float)m_videoHeight);
    // 叠加缩放因子
    float scale = baseScale * m_scaleFactor;

    // 计算显示大小
    int displayWidth = static_cast<int>(m_videoWidth * scale);
    int displayHeight = static_cast<int>(m_videoHeight * scale);

    // 计算居中位置
    int centerX = (width() - displayWidth) / 2;
    int centerY = (height() - displayHeight) / 2;

    // 应用平移偏移
    int finalX = centerX + m_offsetX;
    int finalY = centerY + m_offsetY;

    // 限制拖动范围，确保图像不能拖出窗口外
    // 左边界不能大于0（图像左边不能离开窗口左边）
    if (finalX > 0) {
        finalX = 0;
        m_offsetX = finalX - centerX;
    }
    // 上边界不能大于0（图像上边不能离开窗口上边）
    if (finalY > 0) {
        finalY = 0;
        m_offsetY = finalY - centerY;
    }
    // 右边界不能小于窗口宽度（图像右边不能离开窗口右边）
    if (finalX + displayWidth < width()) {
        finalX = width() - displayWidth;
        m_offsetX = finalX - centerX;
    }
    // 下边界不能小于窗口高度（图像下边不能离开窗口下边）
    if (finalY + displayHeight < height()) {
        finalY = height() - displayHeight;
        m_offsetY = finalY - centerY;
    }

    // 如果图像比窗口小，则居中显示
    if (displayWidth <= width()) {
        finalX = centerX;
        m_offsetX = 0;
    }
    if (displayHeight <= height()) {
        finalY = centerY;
        m_offsetY = 0;
    }

    // 设置显示窗口的位置和大小
    m_displayWnd->setGeometry(finalX, finalY, displayWidth, displayHeight);
}

void CameraWgt::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // 只有点击在视频区域内才允许拖动
        if (m_displayWnd && m_displayWnd->geometry().contains(event->pos())) {
            m_isPanning = true;
            m_lastMousePos = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键重置缩放和位置
        m_scaleFactor = 1.0f;
        m_offsetX = 0;
        m_offsetY = 0;
        updateDisplayGeometry();
    }
}

void CameraWgt::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_offsetX += delta.x();
        m_offsetY += delta.y();
        m_lastMousePos = event->pos();
        updateDisplayGeometry();
    }
}

void CameraWgt::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor); // 恢复光标
    }
}
