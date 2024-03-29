// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef MIXED_DIM_OVERLAPPING_MERGE_HH
#define MIXED_DIM_OVERLAPPING_MERGE_HH

#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

#include <dune/common/fmatrix.hh>
#include <dune/common/fvector.hh>

#include <dune/geometry/referenceelements.hh>
#include <dune/geometry/genericgeometry/geometry.hh>

#include <dune/grid/common/grid.hh>

#include <dune/grid-glue/merging/standardmerge.hh>

/** \brief Computing overlapping grid intersections for grids of different dimensions

   \tparam dim1 Grid dimension of grid 1
   \tparam dim2 Grid dimension of grid 2
   \tparam dimworld World dimension
   \tparam T Type used for coordinates
 */
template<int dim1, int dim2, int dimworld, typename T = double>
class MixedDimOverlappingMerge
  : public StandardMerge<T,dim1,dim2,dimworld>
{

public:

  /*   E X P O R T E D   T Y P E S   A N D   C O N S T A N T S   */

  /// @brief the numeric type used in this interface
  typedef T ctype;

  /// @brief the coordinate type used in this interface
  typedef Dune::FieldVector<T, dimworld>  WorldCoords;

  /// @brief the coordinate type used in this interface
  //typedef Dune::FieldVector<T, dim>  LocalCoords;

  MixedDimOverlappingMerge()
  {}

private:

  typedef typename StandardMerge<T,dim1,dim2,dimworld>::RemoteSimplicialIntersection RemoteSimplicialIntersection;

  /** \brief Compute the intersection between two overlapping elements

     The result is a set of simplices.

     \param grid1ElementType Type of the first element to be intersected
     \param grid1ElementCorners World coordinates of the corners of the first element

     \param grid2ElementType Type of the second element to be intersected
     \param grid2ElementCorners World coordinates of the corners of the second element

   */
  void computeIntersection(const Dune::GeometryType& grid1ElementType,
                           const std::vector<Dune::FieldVector<T,dimworld> >& grid1ElementCorners,
                           unsigned int grid1Index,
                           std::bitset<(1<<dim1)>& neighborIntersects1,
                           const Dune::GeometryType& grid2ElementType,
                           const std::vector<Dune::FieldVector<T,dimworld> >& grid2ElementCorners,
                           unsigned int grid2Index,
                           std::bitset<(1<<dim2)>& neighborIntersects2)
  {
    std::cout << "MixedDimOverlappingMerge::computeIntersection: Not implemented yet!" << std::endl;
  }

};

#endif // MIXED_DIM_OVERLAPPING_MERGE_HH
