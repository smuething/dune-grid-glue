// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/*
 *  Filename:    GridGlue.hh
 *  Version:     1.0
 *  Created on:  Feb 2, 2009
 *  Author:      Gerrit Buse, Christian Engwer
 *  ---------------------------------
 *  Project:     dune-grid-glue
 *  Description: Central component of the module implementing the coupling of two grids.
 *  subversion:  $Id$
 *
 */
/**
 * @file
 * @brief Central component of the module implementing the coupling of two grids.
 */


#ifndef GRIDGLUE_HH
#define GRIDGLUE_HH

#include <dune/common/array.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/iteratorfacades.hh>

#define QUICKHACK_INDEX 1

#include "gridgluecommunicate.hh"
#include <dune/grid-glue/merging/merger.hh>

#if DUNE_VERSION_NEWER_REV(DUNE_COMMON,2,2,0)
#include <dune/common/parallel/mpitraits.hh>
#include <dune/common/parallel/mpicollectivecommunication.hh>
#else
#include <dune/common/mpitraits.hh>
#include <dune/common/mpicollectivecommunication.hh>
#endif
#if DUNE_VERSION_NEWER_REV(DUNE_COMMON,2,1,0)
  #include <dune/common/parallel/indexset.hh>
  #include <dune/common/parallel/plocalindex.hh>
  #include <dune/common/parallel/remoteindices.hh>
  #include <dune/common/parallel/communicator.hh>
  #include <dune/common/parallel/interface.hh>
#else
  #include <dune/istl/indexset.hh>
  #include <dune/istl/plocalindex.hh>
  #include <dune/istl/remoteindices.hh>
  #include <dune/istl/communicator.hh>
  #include <dune/istl/interface.hh>
#endif

/** Document the relation between the old grid names and the new numbering */
enum GridOrdering {
  Domain = 0,
  Target = 1
};

// forward declaration
template<typename P0, typename P1>
class GridGlue;

namespace Dune {
  namespace GridGlue {

    template<typename P0, typename P1>
    class IntersectionData;

    template<typename P0, typename P1, int inside, int outside>
    class Intersection;

    template<typename P0, typename P1, int inside, int outside>
    class IntersectionIterator;

    template<typename P0, typename P1, int inside, int outside>
    class CellIntersectionIterator;

  }
}

template<typename P0, typename P1, int P>
struct GridGlueView;

template<typename P0, typename P1>
struct GridGlueView<P0,P1,0>
{
  typedef P0 Patch;
  typedef Dune::GridGlue::CellIntersectionIterator<P0,P1,0,1> CellIntersectionIterator;
  typedef Dune::GridGlue::IntersectionIterator<P0,P1,0,1> IntersectionIterator;
  typedef typename Patch::GridView::template Codim<0>::Entity GridElement;
  static const P0& patch(const GridGlue<P0,P1>& g)
  {
    return g.patch0_;
  }
};

template<typename P0, typename P1>
struct GridGlueView<P0,P1,1>
{
  typedef P1 Patch;
  typedef Dune::GridGlue::CellIntersectionIterator<P0,P1,1,0> CellIntersectionIterator;
  typedef Dune::GridGlue::IntersectionIterator<P0,P1,1,0> IntersectionIterator;
  typedef typename Patch::GridView::template Codim<0>::Entity GridElement;
  static const P1& patch(const GridGlue<P0,P1>& g)
  {
    return g.patch1_;
  }
};

/**
 * @class GridGlue
 * @brief sequential adapter to couple two grids at specified close together boundaries
 *
 * @tparam P0 patch (extractor) to use for grid 0
 * @tparam P1 patch (extractor) to use for grid 1
 *
 * \todo adapt member names according to style guide
 */
template<typename P0, typename P1>
class GridGlue
{
private:

  /*   P R I V A T E   T Y P E S   */

  /** \brief GlobalId type of an intersection (used for communication) */
  typedef Dune::GridGlue::GlobalId GlobalId;

  /** \brief LocalIndex type of an intersection (used for communication) */
  typedef Dune::ParallelLocalIndex <Dune::PartitionType> LocalIndex;

  /** \brief ParallelIndexSet type (used for communication communication) */
  typedef Dune::ParallelIndexSet <GlobalId, LocalIndex> PIndexSet;

public:

  /*   P U B L I C   T Y P E S   A N D   C O N S T A N T S   */

  /** \brief Grid view of grid 0 (the domain grid) */
  typedef typename P0::GridView Grid0View;

  /** \brief Grid 0 (domain grid) type */
  typedef typename Grid0View::Grid DomainGridType;

  /** \brief Coupling patch of grid 0 */
  typedef P0 Grid0Patch;

  /** \brief Dimension of the grid 0 extractor */
  enum {
    /** \brief Dimension of the grid 0 extractor */
    domdim = Grid0Patch::dim,
    grid0dim = Grid0Patch::dim,
    /** \brief World dimension of the grid 0 extractor */
    domdimworld = Grid0Patch::dimworld,
    grid0dimworld = Grid0Patch::dimworld
  };

  /** \brief Grid view of grid 1 (the target grid) */
  typedef typename P1::GridView Grid1View;

  /** \brief Gird 1 (target grid) type */
  typedef typename Grid1View::Grid TargetGridType;

  /** \brief Coupling patch of grid 1 */
  typedef P1 Grid1Patch;

  /** \todo */
  typedef unsigned int IndexType;

  /** \brief Dimension of the grid 1 extractor */
  enum {
    /** \brief Dimension of the grid 1 extractor */
    tardim = Grid1Patch::dim,
    grid1dim = Grid1Patch::dim,
    /** \brief World dimension of the grid 1 extractor */
    tardimworld = Grid1Patch::dimworld,
    grid1dimworld = Grid1Patch::dimworld
  };


  /** \brief export the world dimension */
  enum {
    /** \brief export the world dimension :
        maximum of the two extractor world dimensions */
    dimworld = ((int)Grid0Patch::dimworld > (int)Grid1Patch::dimworld) ? (int)Grid0Patch::dimworld : (int)Grid1Patch::dimworld

  };

  /** \brief The type used for coordinates
      \todo maybe use traits class to decide which has more precision (DomainGridType::ctype or TargetGridType::ctype) and then take this one
   */
  typedef typename DomainGridType::ctype ctype;

  /** \brief The type used for coordinate vectors */
  typedef Dune::FieldVector<ctype, dimworld>                   Coords;

  /** \brief The type of the domain grid elements */
  typedef typename Grid0View::Traits::template Codim<0>::Entity DomainElement;

  /** \brief Pointer type to domain grid elements */
  typedef typename Grid0View::Traits::template Codim<0>::EntityPointer DomainElementPtr;

  /** \brief The type of the domain grid vertices */
  typedef typename Grid0View::Traits::template Codim<DomainGridType::dimension>::Entity DomainVertex;

  /** \brief Pointer type to domain grid vertices */
  typedef typename Grid0View::Traits::template Codim<DomainGridType::dimension>::EntityPointer DomainVertexPtr;

  /** \brief The type of the target grid elements */
  typedef typename Grid1View::Traits::template Codim<0>::Entity TargetElement;

  /** \brief Pointer type to target grid elements */
  typedef typename Grid1View::Traits::template Codim<0>::EntityPointer TargetElementPtr;

  /** \brief The type of the target grid vertices */
  typedef typename Grid1View::Traits::template Codim<TargetGridType::dimension>::Entity TargetVertex;

  /** \brief Pointer type to target grid vertices */
  typedef typename Grid1View::Traits::template Codim<TargetGridType::dimension>::EntityPointer TargetVertexPtr;

  /** \brief Instance of a Merger */
  typedef ::Merger<typename DomainGridType::ctype,
      DomainGridType::dimension - Grid0Patch::codim,
      TargetGridType::dimension - Grid1Patch::codim,
      dimworld>                         Merger;

  /** \brief Type of remote intersection objects */
  typedef Dune::GridGlue::Intersection<P0,P1,0,1> Intersection;

  friend class Dune::GridGlue::IntersectionData<P0,P1>;
  friend class Dune::GridGlue::Intersection<P0,P1,0,1>;
  friend class Dune::GridGlue::Intersection<P0,P1,1,0>;
  friend class Dune::GridGlue::IntersectionIterator<P0,P1,0,1>;
  friend class Dune::GridGlue::IntersectionIterator<P0,P1,1,0>;
  friend class Dune::GridGlue::CellIntersectionIterator<P0,P1,0,1>;
  friend class Dune::GridGlue::CellIntersectionIterator<P0,P1,1,0>;
  friend class GridGlueView<P0,P1,0>;
  friend class GridGlueView<P0,P1,1>;

  /** \brief Type of the iterator that iterates over remove intersections */

  /** \todo Please doc me! */
  typedef Dune::GridGlue::IntersectionIterator<P0,P1,0,1> Grid0IntersectionIterator;
  /** \todo Please doc me! */
  typedef Dune::GridGlue::IntersectionIterator<P0,P1,1,0> Grid1IntersectionIterator;

  /** \todo Please doc me! */
  typedef Dune::GridGlue::CellIntersectionIterator<P0,P1,0,1> Grid0CellIntersectionIterator;
  /** \todo Please doc me! */
  typedef Dune::GridGlue::CellIntersectionIterator<P0,P1,1,0> Grid1CellIntersectionIterator;

private:

  /*   M E M B E R   V A R I A B L E S   */

  /// @brief the domain surface extractor
  const Grid0Patch&       patch0_;

  /// @brief the target surface extractor
  const Grid1Patch&       patch1_;

  /// @brief the surface merging utility
  Merger*                 merger_;

  /// @brief number of intersections
  IndexType index__sz;

#if HAVE_MPI
  /// @brief MPI_Comm which this GridGlue is working on
  MPI_Comm mpicomm_;

  /// @brief parallel indexSet for the intersections with a local domain entity
  PIndexSet domain_is_;

  /// @brief parallel indexSet for the intersections with a local target entity
  PIndexSet target_is_;

  /// @brief keeps information about which process has which intersection
  Dune::RemoteIndices<PIndexSet> remoteIndices_;
#endif // HAVE_MPI

  /// \todo
  typedef Dune::GridGlue::IntersectionData<P0,P1> IntersectionData;

  /// @brief a vector with intersection elements
  mutable std::vector<IntersectionData>   intersections_;

protected:

  /**
   * @brief after building the merged grid the intersection can be updated
   * through this method (for internal use)
   *
   * @param patch0coords the patch0 vertices' coordinates ordered like e.g. in 3D x_0 y_0 z_0 x_1 y_1 ... y_(n-1) z_(n-1)
   * @param patch0entities array with all patch0 entities represented as corner indices into @c patch0coords;
   * the entities are just written to this array one after another
   * @param patch0types array with all patch0 entities types
   * @param patch0rank  rank of the process where patch0 was extracted
   *
   * @param patch1coords the patch2 vertices' coordinates ordered like e.g. in 3D x_0 y_0 z_0 x_1 y_1 ... y_(n-1) z_(n-1)
   * @param patch1entities just like with the patch0entities and patch0corners
   * @param patch1types array with all patch1 entities types
   * @param patch1rank  rank of the process where patch1 was extracted
   *
   */
  void mergePatches(const std::vector<Dune::FieldVector<ctype,dimworld> >& patch0coords,
                    const std::vector<unsigned int>& patch0entities,
                    const std::vector<Dune::GeometryType>& patch0types,
                    const int patch0rank,
                    const std::vector<Dune::FieldVector<ctype,dimworld> >& patch1coords,
                    const std::vector<unsigned int>& patch1entities,
                    const std::vector<Dune::GeometryType>& patch1types,
                    const int patch1rank);


  template<typename Extractor>
  void extractGrid (const Extractor & extractor,
                    std::vector<Dune::FieldVector<ctype, dimworld> > & coords,
                    std::vector<unsigned int> & faces,
                    std::vector<Dune::GeometryType>& geometryTypes) const;

public:

  /*   C O N S T R U C T O R S   A N D   D E S T R U C T O R S   */

  /**
   * @brief constructor
   *
   * Initializes components but does not "glue" the surfaces. The surfaces
   * are extracted from the grids here though.
   * @param gv1 the domain grid view
   * @param gv2 the target grid view
   * @param matcher The matcher object that is used to compute the merged grid. This class has
   * to be a model of the SurfaceMergeConcept.
   */
  GridGlue(const Grid0Patch& gp1, const Grid1Patch& gp2, Merger* merger);
  /*   G E T T E R S   */

  /** \todo Please doc me! */
  template<int P>
  const typename GridGlueView<P0,P1,P>::Patch & patch() const
  {
    return GridGlueView<P0,P1,P>::patch(*this);
  }

  /**
   * @brief getter for the GridView of patch P
   * @return the object
   */
  template<int P>
  const typename GridGlueView<P0,P1,P>::Patch::GridView & gridView() const
  {
    return GridGlueView<P0,P1,P>::patch(*this).gridView();
  }


  /*   F U N C T I O N A L I T Y   */

  void build();

  /*   I N T E R S E C T I O N S   A N D   I N T E R S E C T I O N   I T E R A T O R S   */

  /**
   * @brief gets an iterator over all remote intersections in the merged grid between domain and target
   *
   * @return the iterator
   */
  template<int I>
  typename GridGlueView<P0,P1,I>::IntersectionIterator ibegin() const
  {
    return typename GridGlueView<P0,P1,I>::IntersectionIterator(this, 0);
  }


  /**
   * @brief gets the (general) end-iterator for iterations over domain codim 0 entities' faces
   *
   * @return the iterator
   */
  template<int I>
  typename GridGlueView<P0,P1,I>::IntersectionIterator iend() const
  {
    return typename GridGlueView<P0,P1,I>::IntersectionIterator(this, index__sz);
  }


  /*! \brief Communicate information on the MergedGrid of a GridGlue

     Template parameter is a model of Dune::GridGlue::CommDataHandle

     \param data GridGlueDataHandle
     \param iftype Interface for which the Communication should take place
     \param dir Communication direction (Forward means Domain to Target, Backward is the reverse)

     \todo seq->seq use commSeq
     \todo seq->par use commSeq
     \todo par->seq use commPar
     \todo par->par use commPar

     \todo add directed version communicate<FROM,TO, DH,DT>(data,iftype,dir)
   */
  template<class DataHandleImp, class DataTypeImp>
  void communicate (Dune::GridGlue::CommDataHandle<DataHandleImp,DataTypeImp> & data,
                    Dune::InterfaceType iftype, Dune::CommunicationDirection dir) const
  {
    typedef Dune::GridGlue::CommDataHandle<DataHandleImp,DataTypeImp> DataHandle;
    typedef typename DataHandle::DataType DataType;

#if HAVE_MPI

    if (mpicomm_ != MPI_COMM_SELF)
    {
      /*
       * P A R A L L E L   V E R S I O N
       */
      // setup communication interfaces
      typedef Dune::EnumItem <Dune::PartitionType, Dune::InteriorEntity> InteriorFlags;
      typedef Dune::EnumItem <Dune::PartitionType, Dune::OverlapEntity>  OverlapFlags;
      typedef Dune::EnumRange <Dune::PartitionType, Dune::InteriorEntity, Dune::GhostEntity>  AllFlags;
      Dune::Interface interface;
      switch (iftype)
      {
      case Dune::InteriorBorder_InteriorBorder_Interface :
        interface.build (remoteIndices_, InteriorFlags(), InteriorFlags() );
        break;
      case Dune::InteriorBorder_All_Interface :
        if (dir == Dune::ForwardCommunication)
          interface.build (remoteIndices_, InteriorFlags(), AllFlags() );
        else
          interface.build (remoteIndices_, AllFlags(), InteriorFlags() );
        break;
      case Dune::Overlap_OverlapFront_Interface :
        interface.build (remoteIndices_, OverlapFlags(), OverlapFlags() );
        break;
      case Dune::Overlap_All_Interface :
        if (dir == Dune::ForwardCommunication)
          interface.build (remoteIndices_, OverlapFlags(), AllFlags() );
        else
          interface.build (remoteIndices_, AllFlags(), OverlapFlags() );
        break;
      case Dune::All_All_Interface :
        interface.build (remoteIndices_, AllFlags(), AllFlags() );
        break;
      default :
        DUNE_THROW(Dune::NotImplemented, "GridGlue::communicate for interface " << iftype << " not implemented");
      }

      // setup communication info (class needed to tunnel all info to the operator)
      typedef Dune::GridGlue::CommInfo<GridGlue,DataHandleImp,DataTypeImp> CommInfo;
      CommInfo commInfo;
      commInfo.dir = dir;
      commInfo.gridglue = this;
      commInfo.data = &data;

      // create communicator
      Dune::BufferedCommunicator bComm ;
      bComm.template build< CommInfo >(commInfo, commInfo, interface);

      // do communication
      // choose communication direction.
      if (dir == Dune::ForwardCommunication)
        bComm.forward< Dune::GridGlue::ForwardOperator >(commInfo, commInfo);
      else
        bComm.backward< Dune::GridGlue::BackwardOperator >(commInfo, commInfo);
    }
    else
#endif // HAVE_MPI
    {
      /*
       * S E Q U E N T I A L   V E R S I O N
       */

      // get comm buffer size
      int ssz = indexSet_size() * 10;       // times data per intersection
      int rsz = indexSet_size() * 10;

      // allocate send/receive buffer
      DataType* sendbuffer = new DataType[ssz];
      DataType* receivebuffer = new DataType[rsz];

      // iterators
      Grid0IntersectionIterator rit = ibegin<0>();
      Grid0IntersectionIterator ritend = iend<0>();

      // gather
      Dune::GridGlue::StreamingMessageBuffer<DataType> gatherbuffer(sendbuffer);
      for (; rit != ritend; ++rit)
      {
        /*
           we need to have to variants depending on the communication direction.
         */
        if (dir == Dune::ForwardCommunication)
        {
          /*
             dir : Forward (domain -> target)
           */
          if (rit->self())
          {
            data.gather(gatherbuffer, rit->inside(), *rit);
          }
        }
        else         // (dir == Dune::BackwardCommunication)
        {
          /*
             dir : Backward (target -> domain)
           */
          if (rit->neighbor())
          {
            data.gather(gatherbuffer, rit->outside(), *rit);
          }
        }
      }

      assert(ssz == rsz);
      for (int i=0; i<ssz; i++)
        receivebuffer[i] = sendbuffer[i];

      // scatter
      Dune::GridGlue::StreamingMessageBuffer<DataType> scatterbuffer(receivebuffer);
      for (rit = ibegin<0>(); rit != ritend; ++rit)
      {
        /*
           we need to have to variants depending on the communication direction.
         */
        if (dir == Dune::ForwardCommunication)
        {
          /*
             dir : Forward (domain -> target)
           */
          if (rit->neighbor())
            data.scatter(scatterbuffer, rit->outside(), *rit,
                         data.size(*rit));
        }
        else         // (dir == Dune::BackwardCommunication)
        {
          /*
             dir : Backward (target -> domain)
           */
          if (rit->self())
            data.scatter(scatterbuffer, rit->inside(), *rit,
                         data.size(*rit));
        }
      }

      // cleanup pointers
      delete[] sendbuffer;
      delete[] receivebuffer;
    }
  }

#if QUICKHACK_INDEX
  /*
   * @brief return an IndexSet mapping from Intersection to IndexType
   */

  // indexset size
  size_t indexSet_size() const
  {
    return index__sz;
  }

#endif

  Intersection getIntersection(int i) const
  {
    return Intersection(this, & intersections_[i]);
  }

  size_t size() const
  {
    return index__sz;
  }

};

#include "gridglue.cc"

#include "intersection.hh"
#include "intersectioniterator.hh"

#endif // GRIDGLUE_HH
