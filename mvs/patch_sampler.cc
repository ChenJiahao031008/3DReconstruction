/*
 * Copyright (C) 2015, Ronny Klowsky, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include "math/defines.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "mvs/defines.h"
#include "mvs/mvs_tools.h"
#include "mvs/patch_sampler.h"

MVS_NAMESPACE_BEGIN

PatchSampler::PatchSampler(std::vector<SingleView::Ptr> const& _views,
    Settings const& _settings,
    int _x, int _y, float _depth, float _dzI, float _dzJ)
    : views(_views)
    , settings(_settings)
    , midPix(_x,_y)
    , masterMeanCol(0.f)
    , depth(_depth)
    , dzI(_dzI)
    , dzJ(_dzJ)
    , success(views.size(), false){

    // 获取参考视角
    SingleView::Ptr refV(views[settings.refViewNr]);

    // 获取设定尺度的参考图像
    core::ByteImage::ConstPtr masterImg(refV->getScaledImg());

    // patch的大小是5x5
    offset = settings.filterWidth / 2;

    // 需要采样的点的个数是5x5
    nrSamples = sqr(settings.filterWidth);

    /* initialize arrays */
    patchPoints.resize(nrSamples);  // patch 对应的三维空间点
    masterColorSamples.resize(nrSamples); // patch 的三维点在参考图像中的颜色
    masterViewDirs.resize(nrSamples); // 每个点对应的视角方向

    /* compute patch position and check if it's valid */
    // 在图像上确定一个5x5的patch，并判断其是否位于图像范围内
    math::Vec2i h;
    h[0] = h[1] = offset;
    topLeft = midPix - h;       // 在参考图像上的的BBox左上角
    bottomRight = midPix + h;   // 在参考图像上的BBox的右下角
    if (topLeft[0] < 0 || topLeft[1] < 0
        || bottomRight[0] > masterImg->width()-1
        || bottomRight[1] > masterImg->height()-1)
        return;

    /* initialize viewing rays from master view */
    // 对于patch中的每个像素，计算世界坐标系中的视线向量(每个像素对应空间中的一条射线）
    std::size_t count = 0;
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j)
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i)
            masterViewDirs[count++] = refV->viewRayScaled(i, j);

    /* initialize master color samples and 3d patch points */
    success[settings.refViewNr] = true;

    /**计算在参考视角中的每个patch点的颜色值，颜色均值以及协方差矩阵**/
    computeMasterSamples();

    /**利用patch几何模型的公式d(i,j) = d + i * hs(s,t) + j* ht(s,t) 计算每个点的3D坐标**/
    computePatchPoints();
}

/**
 * 快速的计算
 * @param v
 * @param color
 * @param deriv
 */
void PatchSampler::fastColAndDeriv(std::size_t v, Samples& color, Samples& deriv){

    success[v] = false;
    SingleView::Ptr refV = views[settings.refViewNr];

    /*第i个视角的图像位置*/
    PixelCoords& imgPos = neighPosSamples[v];
    imgPos.resize(nrSamples);

    // patch的3D中心点
    math::Vec3f const& p0 = patchPoints[nrSamples/2];
    /* compute pixel prints and decide on which MipMap-Level to draw
       the samples */
    float mfp = refV->footPrintScaled(p0);
    float nfp = views[v]->footPrint(p0);
    if (mfp <= 0.f) {
        std::cerr << "Error in getFastColAndDerivSamples! "
                  << "footprint in master view: " << mfp << std::endl;
        throw std::out_of_range("Negative pixel footprint");
    }
    if (nfp <= 0.f)
        return;

    // 邻域视角的分辨率比参考视角高2倍以上
    float ratio = nfp / mfp;
    int mmLevel = 0;
    while (ratio < 0.5f) {
        ++mmLevel;
        ratio *= 2.f;
    }
    mmLevel = views[v]->clampLevel(mmLevel);

    /* compute step size for derivative */
    math::Vec3f p1(p0 + masterViewDirs[nrSamples/2]);
    float d = (views[v]->worldToScreen(p1, mmLevel) - views[v]->worldToScreen(patchPoints[12], mmLevel)).norm();
    if (!(d > 0.f)) {
        return;
    }
    stepSize[v] = 1.f / d;

    /* request according undistorted color image */
    core::ByteImage::ConstPtr img(views[v]->getPyramidImg(mmLevel));
    int const w = img->width();
    int const h = img->height();

    /* compute image position and gradient direction for each sample
       point in neighbor image v */
    std::vector<math::Vec2f> gradDir(nrSamples);
    for (std::size_t i = 0; i < nrSamples; ++i){
        math::Vec3f p0(patchPoints[i]);
        math::Vec3f p1(patchPoints[i] + masterViewDirs[i] * stepSize[v]);
        imgPos[i] = views[v]->worldToScreen(p0, mmLevel);
        // imgPos should be away from image border
        if (!(imgPos[i][0] > 0 && imgPos[i][0] < w-1 &&
                imgPos[i][1] > 0 && imgPos[i][1] < h-1)) {
            return;
        }
        gradDir[i] = views[v]->worldToScreen(p1, mmLevel) - imgPos[i];
    }

    /* draw the samples in the image */
    color.resize(nrSamples, math::Vec3f(0.f));
    deriv.resize(nrSamples, math::Vec3f(0.f));
    colAndExactDeriv(*img, imgPos, gradDir, color, deriv);

    /* normalize the gradient */  //fixme?? 为什么要进行归一化
    for (std::size_t i = 0; i < nrSamples; ++i)
        deriv[i] /= stepSize[v];

    success[v] = true;
}

/**
 * 计算参考视角和第v个视角之间的NCC
 * @param v
 * @return
 */
float PatchSampler::getFastNCC(std::size_t v){

    /**计算第v个视角上patch点的颜色向量**/
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);

    if (!success[v])
        return -1.f;

    assert(success[settings.refViewNr]);

    /**计算颜色均值**/
    math::Vec3f meanY(0.f);
    for (std::size_t i = 0; i < nrSamples; ++i)
        meanY += neighColorSamples[v][i];
    meanY /= (float) nrSamples;

    /**计算NCC的颜色值**/
    float sqrDevY = 0.f;
    float devXY = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i){
        sqrDevY += (neighColorSamples[v][i] - meanY).square_norm();
        // Note: master color samples are normalized!
        devXY += (masterColorSamples[i] - meanX).dot(neighColorSamples[v][i] - meanY);
    }
    float tmp = sqrt(sqrDevX * sqrDevY);
    assert(!MATH_ISNAN(tmp) && !MATH_ISNAN(devXY));
    if (tmp > 0)
        return (devXY / tmp);
    else
        return -1.f;
}

float PatchSampler::getNCC(std::size_t u, std::size_t v){

    if (neighColorSamples[u].empty())
        computeNeighColorSamples(u);
    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[u] || !success[v])
            return -1.f;

    math::Vec3f meanX(0.f);
    math::Vec3f meanY(0.f);
    for (std::size_t i = 0; i < nrSamples; ++i) {
        meanX += neighColorSamples[u][i];
        meanY += neighColorSamples[v][i];
    }
    meanX /= nrSamples;
    meanY /= nrSamples;

    float sqrDevX = 0.f;
    float sqrDevY = 0.f;
    float devXY = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i) {
        sqrDevX += (neighColorSamples[u][i] - meanX).square_norm();
        sqrDevY += (neighColorSamples[v][i] - meanY).square_norm();
        devXY += (neighColorSamples[u][i] - meanX)
            .dot(neighColorSamples[v][i] - meanY);
    }

    float tmp = sqrt(sqrDevX * sqrDevY);
    if (tmp > 0)
        return (devXY / tmp);
    else
        return -1.f;
}

float PatchSampler::getSAD(std::size_t v, math::Vec3f const& cs){

    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[v])
        return -1.f;

    float sum = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i) {
        for (int c = 0; c < 3; ++c) {
            sum += std::abs(cs[c] * neighColorSamples[v][i][c] -
                masterColorSamples[i][c]);
        }
    }
    return sum;
}

float PatchSampler::getSSD(std::size_t v, math::Vec3f const& cs){

    if (neighColorSamples[v].empty())
        computeNeighColorSamples(v);
    if (!success[v])
        return -1.f;

    float sum = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i) {
        for (int c = 0; c < 3; ++c) {
            float diff = cs[c] * neighColorSamples[v][i][c] -
                masterColorSamples[i][c];
            sum += diff * diff;
        }
    }
    return sum;
}

math::Vec3f PatchSampler::getPatchNormal() const{

    std::size_t right = nrSamples/2 + offset;
    std::size_t left = nrSamples/2 - offset;
    std::size_t top = offset;
    std::size_t bottom = nrSamples - 1 - offset;

    math::Vec3f a(patchPoints[right] - patchPoints[left]);
    math::Vec3f b(patchPoints[top] - patchPoints[bottom]);
    math::Vec3f normal(a.cross(b));
    normal.normalize();

    return normal;
}

void PatchSampler::update(float newDepth, float newDzI, float newDzJ){

    // 更新depth, dzI, dzJ,并重新计算patch的三维点
    success.clear();
    success.resize(views.size(), false);
    depth = newDepth;
    dzI = newDzI;
    dzJ = newDzJ;
    success[settings.refViewNr] = true;
    computePatchPoints();
    neighColorSamples.clear();
    neighDerivSamples.clear();
    neighPosSamples.clear();
}

void PatchSampler::computePatchPoints(){

    /**获取参考视角**/
    SingleView::Ptr refV = views[settings.refViewNr];

    unsigned int count = 0;
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j) {
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i) {

            /**公式中的d(i,j) = d + i * hs(s,t) + j* ht(s,t) **/
            float tmpDepth = depth + (i - midPix[0]) * dzI + (j - midPix[1]) * dzJ;
            if (tmpDepth <= 0.f) {
                success[settings.refViewNr] = false;
                return;
            }
            /**计算每个patch点的坐标**/
            patchPoints[count] = refV->camPos + tmpDepth * masterViewDirs[count];
            ++count;
        }
    }
}

void PatchSampler::computeMasterSamples(){

    // 获取参考视角
    SingleView::Ptr refV = views[settings.refViewNr];

    // 获取参考视角的的图像（根据目标尺度空间进行了缩放）
    core::ByteImage::ConstPtr img(refV->getScaledImg());

    /* draw color samples from image and compute mean color */
    std::size_t count = 0;
    std::vector<math::Vec2i> imgPos(nrSamples);
    for (int j = topLeft[1]; j <= bottomRight[1]; ++j) {
        for (int i = topLeft[0]; i <= bottomRight[0]; ++i) {
            imgPos[count][0] = i;
            imgPos[count][1] = j;
            ++count;
        }
    }

    /**在参考图像位置处的颜色值**/
    getXYZColorAtPix(*img, imgPos, &masterColorSamples);

    /**将每个位置处的rgb三个通道的颜色值相加**/
    masterMeanCol = 0.f;
    for (std::size_t i = 0; i < nrSamples; ++i) {
        for (int c = 0; c < 3; ++c) {
            assert(masterColorSamples[i][c] >= 0 && masterColorSamples[i][c] <= 1);
            masterMeanCol += masterColorSamples[i][c];
        }
    }

    /**计算颜色均值**/
    masterMeanCol /= 3.f * nrSamples;
    if (masterMeanCol < 0.01f || masterMeanCol > 0.99f) {
        success[settings.refViewNr] = false;
        return;
    }

    meanX.fill(0.f);

    /**利用均值对颜色向量进行归一化**/
    /* normalize master samples so that average intensity over all channels is 1 and compute mean color afterwards */
    for (std::size_t i = 0; i < nrSamples; ++i) {
        masterColorSamples[i] /= masterMeanCol;
        meanX += masterColorSamples[i];
    }
    meanX /= nrSamples;
    sqrDevX = 0.f;

    /**计算颜色值的协方差**/
    /* compute variance (independent from actual mean) */
    for (std::size_t i = 0; i < nrSamples; ++i)
        sqrDevX += (masterColorSamples[i] - meanX).square_norm();
}

void PatchSampler::computeNeighColorSamples(std::size_t v){

    /**参考视角**/
    SingleView::Ptr refV = views[settings.refViewNr];

    // patch points在第v个视角上的颜色值
    Samples & color = neighColorSamples[v];
    // patch points在第v个视角上的像素坐标
    PixelCoords & imgPos = neighPosSamples[v];
    success[v] = false;

    /* compute pixel prints and decide on which MipMap-Level to draw the samples */
    /**利用中心点，计算在参考帧和当前帧图像上的分辨率，从而确定相邻视角的最佳尺度**/
    math::Vec3f const & p0 = patchPoints[nrSamples/2];
    float mfp = refV->footPrintScaled(p0);
    float nfp = views[v]->footPrint(p0);
    if (mfp <= 0.f) {
        std::cerr << "Error in computeNeighColorSamples! "
                  << "footprint in master view: " << mfp << std::endl;
        throw std::out_of_range("Negative pixel print");
    }
    if (nfp <= 0.f)
        return;
    float ratio = nfp / mfp;

    int mmLevel = 0;
    while (ratio < 0.5f) {
        ++mmLevel;
        ratio *= 2.f;
    }

    /**读取对应尺度的图像**/
    mmLevel = views[v]->clampLevel(mmLevel);
    core::ByteImage::ConstPtr img(views[v]->getPyramidImg(mmLevel));
    int const w = img->width();
    int const h = img->height();
    color.resize(nrSamples);
    imgPos.resize(nrSamples);


    /**将patch的3D点投影到视角v中，求得投影坐标，并获取投影点的像素**/
    for (std::size_t i = 0; i < nrSamples; ++i) {
        imgPos[i] = views[v]->worldToScreen(patchPoints[i], mmLevel);
        // imgPos should be away from image border
        if (!(imgPos[i][0] > 0 && imgPos[i][0] < w-1 &&
                imgPos[i][1] > 0 && imgPos[i][1] < h-1)) {
            return;
        }
    }

    /**获取图像坐标处的颜色值**/
    getXYZColorAtPos(*img, imgPos, &color);

    /**操作成功**/
    success[v] = true;
}


MVS_NAMESPACE_END
