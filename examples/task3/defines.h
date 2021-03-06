//
// Created by caoqi on 2018/8/28.
//

#ifndef IMAGEBASEDMODELLINGEDU_DEFINES_H
#define IMAGEBASEDMODELLINGEDU_DEFINES_H

#include <string>
#define  MAX_PIXELS 6000000

// 重建一个三维点需要的最少的视角个数
#define MIN_VIEWS_PER_TRACK  2

// 用于对Track进行滤波
#define TRACK_ERROR_THRES_FACTOR 10.0f

// 用于重建新的Ttrakc
#define NEW_TRACK_ERROR_THRES 0.01f

static const std::string undistorted_name = "undistorted";
static const std::string original_name = "original";
static const std::string exif_name = "exif";
static const std::string prebundle_file = "prebundle.sfm";
#endif //IMAGEBASEDMODELLINGEDU_DEFINES_H
