// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_GRIDGLUECOMMUNICATE_HH
#define DUNE_GRIDGLUECOMMUNICATE_HH

/**@file
   @author Christian Engwer
   @brief Describes the parallel communication interface class for Dune::GridGlue
 */

#include <dune/common/version.hh>
#include <dune/common/bartonnackmanifcheck.hh>
#include <dune/grid/common/datahandleif.hh>
#include <dune/grid/common/gridenums.hh>

#if DUNE_VERSION_NEWER_REV(DUNE_COMMON,2,1,0)
  #include <dune/common/parallel/communicator.hh>
#else
  #include <dune/istl/communicator.hh>
#endif

namespace Dune {
  namespace GridGlue {

    typedef std::pair<int, int> RankPair;
    struct GlobalId : public std::pair<RankPair, unsigned int>
    {
      GlobalId() {
        this->first.first = 0;
        this->first.second = 0;
        this->second = 0;
      }
      GlobalId(int i) {
        this->first.first = i;
        this->first.second = i;
        this->second = 0;
      }
    };

    /**
       \brief describes the features of a data handle for
       communication in parallel runs using the GridGlue::communicate methods.

       Here the Barton-Nackman trick is used to interprete data handle objects
       as its interface. Therefore usable data handle classes need to be
       derived from this class.

       \tparam DataHandleImp implementation of the users data handle
       \tparam DataTypeImp type of data that are going to be communicated which is exported as \c DataType (for example double)
       \ingroup GridGlueCommunication
     */
    template <class DataHandleImp, class DataTypeImp>
    class CommDataHandle
    {
    public:
      //! data type of data to communicate
      typedef DataTypeImp DataType;

    protected:
      // one should not create an explicit instance of this inteface object
      CommDataHandle() {}

    public:

      /*! how many objects of type DataType have to be sent for a given intersection
         Note: Both sender and receiver side need to know this size.
       */
      template<class RISType>
      size_t size (RISType& i) const
      {
        CHECK_INTERFACE_IMPLEMENTATION((asImp().size(i)));
        return asImp().size(i);
      }

      /** @brief pack data from user to message buffer
          @param buff message buffer provided by the grid
          @param e entity for which date should be packed to buffer
          @param i Intersection for which date should be packed to buffer
       */
      template<class MessageBufferImp, class EntityType, class RISType>
      void gather (MessageBufferImp& buff, const EntityType& e, const RISType & i) const
      {
        MessageBufferIF<MessageBufferImp> buffIF(buff);
        CHECK_AND_CALL_INTERFACE_IMPLEMENTATION((asImp().gather(buffIF,e,i)));
      }

      /*! unpack data from message buffer to user
         n is the number of objects sent by the sender
         @param buff message buffer provided by the grid
         @param e entity for which date should be unpacked from buffer
         @param n number of data written to buffer for this entity before
       */
      template<class MessageBufferImp, class EntityType, class RISType>
      void scatter (MessageBufferImp& buff, const EntityType& e, const RISType & i, size_t n)
      {
        MessageBufferIF<MessageBufferImp> buffIF(buff);
        CHECK_AND_CALL_INTERFACE_IMPLEMENTATION((asImp().scatter(buffIF,e,i,n)));
      }

    private:
      //!  Barton-Nackman trick
      DataHandleImp& asImp () {
        return static_cast<DataHandleImp &> (*this);
      }
      //!  Barton-Nackman trick
      const DataHandleImp& asImp () const
      {
        return static_cast<const DataHandleImp &>(*this);
      }
    }; // end class CommDataHandleIF

    /**
       Streaming MessageBuffer for the GridGlue communication
       \ingroup GridGlueCommunication
     */
    template<typename DT>
    class StreamingMessageBuffer {
    public:
      typedef DT value_type;

      // Constructor
      StreamingMessageBuffer (DT *p)
      {
        a=p;
        i=0;
        j=0;
      }

      // write data to message buffer, acts like a stream !
      template<class Y>
      void write (const Y& data)
      {
        dune_static_assert(( is_same<DT,Y>::value ), "DataType missmatch");
        a[i++] = data;
      }

      // read data from message buffer, acts like a stream !
      template<class Y>
      void read (Y& data) const
      {
        dune_static_assert(( is_same<DT,Y>::value ), "DataType missmatch");
        data = a[j++];
      }

      size_t counter() const { return i; }

      void clear()
      {
        i = 0;
        j = 0;
      }

      // we need access to these variables in an assertion
#ifdef NDEBUG
    private:
#endif
      DT *a;
      size_t i;
      mutable size_t j;
    };

    /**
       \brief forward gather scatter to user defined CommInfo class

       - implements the ParallelIndexset gather/scatter interface
       - redirects all calls to the GridGlueCommDataHandleIF gather/scatter methods
     */
    template<int dir>
    class CommunicationOperator
    {
    public:
      template<class CommInfo>
      static const typename CommInfo::DataType& gather(const CommInfo& commInfo, size_t i, size_t j = 0)
      {
        // get Intersection
        typedef typename CommInfo::GridGlue::Intersection Intersection;
        Intersection ris(commInfo.gridglue->getIntersection(i));

        // fill buffer if we have a new intersection
        if (j == 0)
        {
          commInfo.mbuffer.clear();
          if (dir == Dune::ForwardCommunication)
          {
            // read from domain
            if(ris.self())
              commInfo.data->gather(commInfo.mbuffer, ris.inside(), ris);
          }
          else   // (dir == Dune::BackwardCommunication)
          {
            // read from target
            if(ris.neighbor())
              commInfo.data->gather(commInfo.mbuffer, ris.outside(), ris);
          }
        }

        // return the j'th value in the buffer
        assert(j < commInfo.mbuffer.i);
        return commInfo.buffer[j];
      }

      template<class CommInfo>
      static void scatter(CommInfo& commInfo, const typename CommInfo::DataType& v, std::size_t i, std::size_t j = 0)
      {
        // extract GridGlue objects...
        typedef typename CommInfo::GridGlue::Intersection Intersection;
        Intersection ris(commInfo.gridglue->getIntersection(i));

        // get size if we have a new intersection
        if (j == 0)
        {
          commInfo.mbuffer.clear();
          commInfo.currentsize = commInfo.data->size(ris);
        }

        // write entry to buffer
        commInfo.buffer[j] = v;

        // write back the buffer if we are at the end of this intersection
        if (j == commInfo.currentsize-1)
        {
          if (dir == Dune::ForwardCommunication)
          {
            // write to target
            if(ris.neighbor())
              commInfo.data->scatter(commInfo.mbuffer, ris.outside(), ris, commInfo.currentsize);
          }
          else   // (dir == Dune::BackwardCommunication)
          {
            // write to domain
            if(ris.self())
              commInfo.data->scatter(commInfo.mbuffer, ris.inside(), ris, commInfo.currentsize);
          }
          assert(commInfo.mbuffer.j <= commInfo.currentsize);
        }
      }
    };

    typedef CommunicationOperator<Dune::ForwardCommunication> ForwardOperator;
    typedef CommunicationOperator<Dune::BackwardCommunication> BackwardOperator;

    /**
       \brief collects all GridGlue data requried for communication
       \ingroup GridGlueCommunication
     */
    template <typename GG, class DataHandleImp, class DataTypeImp>
    struct CommInfo
    {
      typedef DataTypeImp value_type;
      typedef GG GridGlue;
      typedef DataTypeImp DataType;

      CommInfo() : buffer(100), mbuffer(&buffer[0])
      {}

      // tunnel information to the policy and the operators
      const GridGlue * gridglue;
      ::Dune::GridGlue::CommDataHandle<DataHandleImp, DataTypeImp> * data;

      // state variables
      std::vector<DataType> buffer;
      mutable ::Dune::GridGlue::StreamingMessageBuffer<DataType> mbuffer;
      size_t currentsize;
      Dune::CommunicationDirection dir;
    };

  } // end namespace GridGlue

#if HAVE_MPI
  /**
     \brief specialization of the CommPolicy struct, required for the ParallelIndexsets
   */
  template<typename GG, class DataHandleImp, class DataTypeImp>
  struct CommPolicy< ::Dune::GridGlue::CommInfo<GG, DataHandleImp, DataTypeImp> >
  {
    /**
     * @brief The type of the GridGlueCommInfo
     */
    typedef ::Dune::GridGlue::CommInfo<GG, DataHandleImp, DataTypeImp> Type;

    /**
     * @brief The datatype that should be communicated.
     */
    typedef DataTypeImp IndexedType;

    /**
     * @brief Each intersection can communicate a different number of objects.
     */
    // typedef SizeOne IndexedTypeFlag;
    typedef VariableSize IndexedTypeFlag;

    /**
     * @brief Get the number of objects at an intersection.
     */
    static size_t getSize(const Type& commInfo, size_t i)
    {
      // get Intersection
      typedef typename Type::GridGlue::Intersection Intersection;
      Intersection ris(commInfo.gridglue->getIntersection(i));

      if (commInfo.dir == Dune::ForwardCommunication)
      {
        if (!ris.self()) return 0;
      }
      else
      {
        if (!ris.neighbor()) return 0;
      }

      // ask data handle for size
      return commInfo.data->size(ris);
    }
  };
#endif

} // end namespace Dune
#endif
