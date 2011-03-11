// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*
 *  Filename:    SimplexGeometry.hh
 *  Version:     1.0
 *  Created on:  Feb 17, 2009
 *  Author:      Gerrit Buse
 *  ---------------------------------
 *  Project:     dune-grid-glue
 *  Description: classes used for local and global geometries of Intersection implementations
 *  subversion:  $Id$
 *
 */
/**
 * @file
 * @brief contains customized geometry implementations that can be used to
 * describe the mappings between simplex element, their faces and simplicial
 * parts inside those faces
 */

#ifndef SIMPLEXGEOMETRY_HH
#define SIMPLEXGEOMETRY_HH

#include <dune/common/static_assert.hh>
#include <dune/common/array.hh>
#include <dune/grid/genericgeometry/geometry.hh>
#include <dune/grid/genericgeometry/geometrytraits.hh>
#include <dune/grid/genericgeometry/cornermapping.hh>
#include <dune/grid/genericgeometry/referenceelements.hh>
#include <dune/grid/genericgeometry/topologytypes.hh>


/**
 * @class LocalSimplexGeometryTraits
 * @brief tweaked geometry traits passed to GenericGeometry::BasicGeometry
 *
 * This geometry traits class configures BasicGeometry to use only affine mappings
 * for simplicial (i.e. especially non-hybrid) grid structures.
 * The traits class is suited for the use with full-dimensional structures as well as
 * manifolds and flat hyperdimensional meshes. It chooses the correct mapping from
 * a subface simplex in the surface (or the grid mesh) to it's parent codimension 0
 * entity. Take the 3D case with (dimension, dimensionworld) for example:
 * - surfaces    (3,3): triangle -> tetrahedron
 * - manifolds   (2,3): triangle -> triangle
 * - flat meshes (2,2): triangle -> triangle
 */
template<typename G, bool is_manifold = false>
struct LocalSimplexGeometryTraits
  : public Dune::GenericGeometry::DefaultGeometryTraits<typename G::ctype, G::dimension, G::dimensionworld - static_cast<int>(is_manifold), true>
{
  typedef typename Dune::GenericGeometry::DefaultGeometryTraits<typename G::ctype, G::dimension, G::dimensionworld - static_cast<int>(is_manifold), true> Base;

  // This traits class represents a single type only ...
  static const bool hybrid = false;
  // ... and this type is 'simplex'.
  static const unsigned int topologyId = Dune::GenericGeometry::SimplexTopology< G::dimension >::type::id;

  /**
   * Note:
   * The coorddimension of the mapping is set to the adjusted value of dimWorld!
   * It won't work if you use G::dimensionworld, instead the construct  "G::dimensionworld - static_cast<int>(is_manifold)"
   * has to be used, which is identical with Base::dimWorld.
   * The problem only occurs when working with manifold, hence the name of the template parameter.
   */
  template<class Topology>
  struct Mapping
  {
    typedef Dune::GenericGeometry::CoordStorage<typename Base::CoordTraits, Topology, Base::dimWorld>                        CornerStorage;
    typedef Dune::GenericGeometry::CornerMapping<typename Base::CoordTraits, Topology, Base::dimWorld, CornerStorage, true>  type;
  };

  // caching configuration chosen in base class is fine

#define DUNE_COMMON_VERSION_NUMBER (DUNE_COMMON_VERSION_MAJOR * 10 + DUNE_COMMON_VERSION_MINOR)
#if DUNE_COMMON_VERSION_NUMBER < 21
  // The type used for 1d reference elements
  static const Dune::GeometryType::BasicType dunetype = Dune::GeometryType::simplex;
#endif
};


/**
 * @class SimplexGeometryTraits
 * @brief tweaked geometry traits passed to Dune::GenericGeometry::BasicGeometry
 *
 * This geometry traits class configures BasicGeometry to use only affine mappings
 * for simplicial (i.e. especially non-hybrid) grid structures.
 * The traits class is suited for the use with full-dimensional structures as well as
 * manifolds and flat hyperdimensional meshes. It chooses the correct mapping from
 * a subface simplex in the surface (or the grid mesh) to it's parent codimension 0
 * entity. Take the 3D case with (dimension, dimensionworld) for example:
 * - surfaces    (3,3): triangle -> tetrahedron
 * - manifolds   (2,3): triangle -> tetrahedron
 * - flat meshes (2,2): triangle -> triangle
 */
template<typename G>
struct GlobalSimplexGeometryTraits
  : public Dune::GenericGeometry::DefaultGeometryTraits<typename G::ctype, G::dimension, G::dimensionworld, true>
{
  typedef typename Dune::GenericGeometry::DefaultGeometryTraits<typename G::ctype, G::dimension, G::dimensionworld, true> Base;

  // This traits class represents a single type only...
  static const bool hybrid = false;
  // ... and this type is 'simplex'.
  static const unsigned int topologyId = Dune::GenericGeometry::SimplexTopology< G::dimensionworld >::type::id;

  /**
   * Note:
   * In contrast to LocalSimplexGeometryTraits it doesn't matter which one you use,
   * Base::dimWorld or G::dimensionworld. For reasons of conformance the Traits' class'
   * constant is used.
   */
  template<class Topology>
  struct Mapping
  {
    typedef Dune::GenericGeometry::CoordStorage<typename Base::CoordTraits, Topology, Base::dimWorld>                        CornerStorage;
    typedef Dune::GenericGeometry::CornerMapping<typename Base::CoordTraits, Topology, Base::dimWorld, CornerStorage, true>  type;
  };

  // caching configuration chosen in base class is fine


#define DUNE_COMMON_VERSION_NUMBER (DUNE_COMMON_VERSION_MAJOR * 10 + DUNE_COMMON_VERSION_MINOR)
#if DUNE_COMMON_VERSION_NUMBER < 21
  // The type used for 1d reference elements
  static const Dune::GeometryType::BasicType dunetype = Dune::GeometryType::simplex;
#endif
};





/**
 * @class SimplexGeometry
 * @brief This class is derived from BasicGeometry using tuned geometry traits.
 *
 * The used geometry traits allow for a more efficient compilation of the default
 * implementation of CornerMapping. Using only affine mappings it is specialized
 * for the case of exclusively simplicial geometries.
 */
template<int mydim, int coorddim, typename G>
class SimplexGeometry : public Dune::GenericGeometry::BasicGeometry<mydim, GlobalSimplexGeometryTraits<G> >
{
  typedef SimplexGeometry<mydim, coorddim, G> This;

  typedef Dune::GenericGeometry::BasicGeometry<mydim, GlobalSimplexGeometryTraits<G> > Base;

  enum { simplex_corners = coorddim + static_cast<int>(mydim == coorddim) };

public:

  typedef typename Base::Mapping Mapping;

  template< class CoordVector>
  SimplexGeometry(const Dune::GeometryType &type, const CoordVector &coords) : Base(type, coords)
  {}


  SimplexGeometry()
  {}

  /**
   * @brief Setup method with a geometry type and a set of corners
   * @param type the geometry type of this subface, i.e. most likely a simplex in 1D or 2D
   * @param coordinates The corner coordinates in DUNE numbering
   */
  void setup(const Dune::GeometryType& type, const Dune::array<Dune::FieldVector<typename G::ctype, coorddim>, simplex_corners>& coordinates)
  {
    // Yes, a strange way, but the only way, as BasicGeometry doesn't have a setup method
    Base::operator=(Base(type, coordinates));
  }

};



/**
 * @class LocalSimplexGeometry
 * @brief This class is derived from BasicGeometry using tuned geometry traits.
 *
 * The used geometry traits allow for a more efficient compilation of the default
 * implementation of CornerMapping. Using only affine mappings it is specialized
 * for the case of exclusively simplicial geometries.
 * Note:
 * This class mostly does the same as the SimplexGeometry class. Behaviour only differs
 * in the case of manifolds. Then the local mapping is 1D->1D resp. 2D->2D here whereas
 * the global geometry maps to the world coordinate space, i.e. 1D->2D resp. 2D->3D.
 */
template<int mydim, int coorddim, typename G>
class LocalSimplexGeometry
  : public Dune::GenericGeometry::BasicGeometry<mydim, LocalSimplexGeometryTraits<G, (coorddim < static_cast<int>(G::dimensionworld))> >
{
  typedef Dune::GenericGeometry::BasicGeometry<mydim, LocalSimplexGeometryTraits<G, (coorddim < static_cast<int>(G::dimensionworld))> > Base;

  enum { simplex_corners = coorddim + static_cast<int>(mydim == coorddim) };

public:

  typedef typename Base::Mapping Mapping;


  template< class CoordVector>
  LocalSimplexGeometry(const Dune::GeometryType &type, const CoordVector &coords) : Base(type, coords)
  {}


  LocalSimplexGeometry()
  {}


  /**
   * @brief Setup method with a geometry type and a set of corners
   * @param type the geometry type of this subface, i.e. most likely a simplex in 1D or 2D
   * @param coordinates The corner coordinates in DUNE numbering
   */
  void setup(const Dune::GeometryType& type, const Dune::array<Dune::FieldVector<typename G::ctype, coorddim>, simplex_corners>& coordinates)
  {
    // set up base class
    // Yes, a strange way, but the only way, as BasicGeometry doesn't have a setup method
    Base::operator=(Base(type, coordinates));
  }

};


#endif // SIMPLEXGEOMETRY_HH
