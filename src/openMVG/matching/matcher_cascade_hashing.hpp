// Copyright (c) 2015 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "openMVG/matching/matching_interface.hpp"
#include "openMVG/matching/cascade_hasher.hpp"
#include "openMVG/matching/indMatch.hpp"
#include <memory>
#include <random>
#include <cmath>

namespace openMVG {
namespace matching {

//------------------
//-- Bibliography --
//------------------
//- [1] "Fast and Accurate Image Matching with Cascade Hashing for 3D Reconstruction"
//- Authors: Jian Cheng, Cong Leng, Jiaxiang Wu, Hainan Cui, Hanqing Lu.
//- Date: 2014.
//- Conference: CVPR.
//

// Implementation of descriptor matching using the cascade hashing method of [1].
// If you use this matcher, please cite the paper.
// template Metric parameter is ignored (by default compute square(L2 distance)).
template < typename Scalar = float, typename Metric = L2_Simple<Scalar> >
class ArrayMatcherCascadeHashing  : public ArrayMatcher<Scalar, Metric>
{
  public:
  typedef typename Metric::ResultType DistanceType;

  ArrayMatcherCascadeHashing()   {}
  virtual ~ArrayMatcherCascadeHashing() {
    memMapping.reset();
  }

  /**
   * Build the matching structure
   *
   * \param[in] dataset   Input data.
   * \param[in] nbRows    The number of component.
   * \param[in] dimension Length of the data contained in the dataset.
   *
   * \return True if success.
   */
  bool Build(const Scalar * dataset, int nbRows, int dimension) {
    if (nbRows < 1) {
      memMapping.reset(nullptr);
      return false;
    }
    memMapping.reset(new Eigen::Map<BaseMat>( (Scalar*)dataset, nbRows, dimension) );

    // Init the cascade hasher (hashing projection matrices)
    cascade_hasher_.Init(dimension);
    // Index the input descriptors
    zero_mean_descriptor_ = CascadeHasher::GetZeroMeanDescriptor(*memMapping);
    zero_mean_descriptor_.fill(zero_mean_descriptor_.mean());
    hashed_base_ = cascade_hasher_.CreateHashedDescriptions(
      *memMapping, zero_mean_descriptor_);

    return true;
  };

  /**
   * Search the nearest Neighbor of the scalar array query.
   *
   * \param[in]   query     The query array
   * \param[out]  indice    The indice of array in the dataset that
   *  have been computed as the nearest array.
   * \param[out]  distance  The distance between the two arrays.
   *
   * \return True if success.
   */
  bool SearchNeighbour( const Scalar * query,
                        int * indice, DistanceType * distance)
  {
    std::cerr << "This matcher is not made to match a single query" << std::endl;
    return false;
  }


/**
   * Search the N nearest Neighbor of the scalar array query.
   *
   * \param[in]   query     The query array
   * \param[in]   nbQuery   The number of query rows
   * \param[out]  indices   The corresponding (query, neighbor) indices
   * \param[out]  distances  The distances between the matched arrays.
   * \param[out]  NN        The number of maximal neighbor that will be searched.
   *
   * \return True if success.
   */
  bool SearchNeighbours
  (
    const Scalar * query, int nbQuery,
    IndMatches * pvec_indices,
    std::vector<DistanceType> * pvec_distances,
    size_t NN
  )
  {
    if (memMapping.get() == NULL)  {
      return false;
    }

    if (NN > (*memMapping).rows() || nbQuery < 1) {
      return false;
    }

    // Matrix representation of the input data;
    Eigen::Map<BaseMat> mat_query((Scalar*)query, nbQuery, (*memMapping).cols());

    pvec_distances->reserve(nbQuery * NN);
    pvec_indices->reserve(nbQuery * NN);

    // Index the query descriptors
    const HashedDescriptions hashed_query = cascade_hasher_.CreateHashedDescriptions(
      mat_query,
      zero_mean_descriptor_);
    // Match the query descriptors to the database
    cascade_hasher_.Match_HashedDescriptions(
      hashed_query, mat_query,
      hashed_base_, *memMapping,
      pvec_indices, pvec_distances,
      NN);

    return true;
  };

private:
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> BaseMat;
  /// Use a memory mapping in order to avoid memory re-allocation
  std::unique_ptr< Eigen::Map<BaseMat> > memMapping;
  CascadeHasher cascade_hasher_;
  HashedDescriptions hashed_base_;
  Eigen::VectorXf zero_mean_descriptor_;
};

}  // namespace matching
}  // namespace openMVG

