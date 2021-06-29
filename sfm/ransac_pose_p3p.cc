/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <set>
#include <atomic>
#include <iostream>

#include "util/system.h"
#include "math/matrix_tools.h"
#include "sfm/ransac_pose_p3p.h"
#include "sfm/pose_p3p.h"

SFM_NAMESPACE_BEGIN

RansacPoseP3P::RansacPoseP3P (Options const& options)
    : opts(options)
{
}

void
RansacPoseP3P::estimate (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 3> const& k_matrix, Result* result) const{


    if (this->opts.verbose_output){
        std::cout << "RANSAC-3: Running for " << this->opts.max_iterations
            << " iterations, threshold " << this->opts.threshold
            << "..." << std::endl;
    }

    /* Pre-compute inverse K matrix to compute directions from corresp. */
    math::Matrix<double, 3, 3> inv_k_matrix = math::matrix_inverse(k_matrix);
    std::atomic<int> num_iterations(0);

#pragma omp parallel
    {
        std::vector<int> inliers;
        inliers.reserve(corresp.size());
#pragma omp for
        for (int i = 0; i < this->opts.max_iterations; ++i)
        {
            int iteration = i;
            if (this->opts.verbose_output)
                iteration = num_iterations++;

            /* Compute up to four poses [R|t] using P3P algorithm. */
            PutativePoses poses;
            this->compute_p3p(corresp, inv_k_matrix, &poses);

            /* Check all putative solutions and count inliers. */
            for (std::size_t j = 0; j < poses.size(); ++j){
                this->find_inliers(corresp, k_matrix, poses[j], &inliers);
#pragma omp critical
                if (inliers.size() > result->inliers.size()){
                    result->pose = poses[j];
                    std::swap(result->inliers, inliers);
                    inliers.reserve(corresp.size());

                    if (this->opts.verbose_output){
                        std::cout << "RANSAC-3: Iteration " << iteration
                            << ", inliers " << result->inliers.size() << " ("
                            << (100.0 * result->inliers.size() / corresp.size())
                            << "%)" << std::endl;
                    }
                }
            }
        }
    }
}

void
RansacPoseP3P::compute_p3p (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 3> const& inv_k_matrix,
    PutativePoses* poses) const{

    //std::cout<<"inverse k matrix: "<<inv_k_matrix<<std::endl;

    if (corresp.size() < 3)
        throw std::invalid_argument("At least 3 correspondences required");

    /* Draw 3 unique random numbers. */
    std::set<int> result;
    while (result.size() < 3)
        result.insert(util::system::rand_int() % corresp.size());

    std::set<int>::const_iterator iter = result.begin();
    Correspondence2D3D const& c1(corresp[*iter++]);
    Correspondence2D3D const& c2(corresp[*iter++]);
    Correspondence2D3D const& c3(corresp[*iter]);

    //std::cout<<"c1: "<<c1.p2d[0]<<" "<<c1.p2d[1]<<std::endl;
    //std::cout<<"c2: "<<c2.p2d[0]<<" "<<c2.p2d[1]<<std::endl;
    //std::cout<<"c3: "<<c3.p2d[0]<<" "<<c3.p2d[1]<<std::endl;

    pose_p3p_kneip(
        math::Vec3d(c1.p3d), math::Vec3d(c2.p3d), math::Vec3d(c3.p3d),
        inv_k_matrix.mult(math::Vec3d(c1.p2d[0], c1.p2d[1], 1.0)),
        inv_k_matrix.mult(math::Vec3d(c2.p2d[0], c2.p2d[1], 1.0)),
        inv_k_matrix.mult(math::Vec3d(c3.p2d[0], c3.p2d[1], 1.0)),
        poses);


}

void
RansacPoseP3P::find_inliers (Correspondences2D3D const& corresp,
    math::Matrix<double, 3, 3> const& k_matrix,
    Pose const& pose, std::vector<int>* inliers) const
{
    inliers->resize(0);
    double const square_threshold = MATH_POW2(this->opts.threshold);
    for (std::size_t i = 0; i < corresp.size(); ++i)
    {
        Correspondence2D3D const& c = corresp[i];
        math::Vec4d p3d(c.p3d[0], c.p3d[1], c.p3d[2], 1.0);
        math::Vec3d p2d = k_matrix * (pose * p3d);
        double square_error = MATH_POW2(p2d[0] / p2d[2] - c.p2d[0])
            + MATH_POW2(p2d[1] / p2d[2] - c.p2d[1]);
        if (square_error < square_threshold)
            inliers->push_back(i);
    }
}

SFM_NAMESPACE_END
