// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include <config.h>

#include <iostream>

#include <dune/grid-glue/adapter/gridglue.hh>
#include <dune/grid-glue/merging/psurfacemerge.hh>

template <class ctype, int dimworld>
Dune::FieldVector<ctype,dimworld> makeVec(double c)
{
  Dune::FieldVector<ctype,dimworld> x;
  x[0] = c;
  for (size_t i=1; i<dimworld; i++) x[i] = 1;
  return x;
}

template <class ctype, int dimworld>
Dune::FieldVector<ctype,dimworld> makeVec(double c1, double c2)
{
  Dune::FieldVector<ctype,dimworld> x;
  x[0] = c1;
  x[1] = c2;
  for (size_t i=2; i<dimworld; i++) x[i] = 1;
  return x;
}

template<int dim>
struct setupGrid {};

template<>
struct setupGrid<1> {
  enum { dim = 1 };
  template<int dimworld, class ctype>
  static
  void fill(std::vector<Dune::FieldVector<ctype,dimworld> > & grid1_coords,
            std::vector<unsigned int> & grid1_elements,
            std::vector<Dune::GeometryType> & grid1_element_types,
            std::vector<Dune::FieldVector<ctype,dimworld> > & grid2_coords,
            std::vector<unsigned int> & grid2_elements,
            std::vector<Dune::GeometryType> & grid2_element_types)
  {
    /*
       0  2        5
       |--|--------|
       M |--|--|-----|
       |-----|-----|
       0     3     5
     */

    grid1_coords.push_back(makeVec<ctype, dimworld>(0));
    grid1_coords.push_back(makeVec<ctype, dimworld>(2));
    grid1_coords.push_back(makeVec<ctype, dimworld>(5));

    grid1_elements.push_back(0);
    grid1_elements.push_back(1);
    grid1_elements.push_back(1);
    grid1_elements.push_back(2);
    grid1_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
    grid1_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));

    grid2_coords.push_back(makeVec<ctype, dimworld>(0));
    grid2_coords.push_back(makeVec<ctype, dimworld>(3));
    grid2_coords.push_back(makeVec<ctype, dimworld>(5));

    grid2_elements.push_back(0);
    grid2_elements.push_back(1);
    grid2_elements.push_back(1);
    grid2_elements.push_back(2);
    grid2_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
    grid2_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
  }
};

template<>
struct setupGrid<2> {
  enum { dim = 2 };
  template<int dimworld, class ctype>
  static
  void fill(std::vector<Dune::FieldVector<ctype,dimworld> > & grid1_coords,
            std::vector<unsigned int> & grid1_elements,
            std::vector<Dune::GeometryType> & grid1_element_types,
            std::vector<Dune::FieldVector<ctype,dimworld> > & grid2_coords,
            std::vector<unsigned int> & grid2_elements,
            std::vector<Dune::GeometryType> & grid2_element_types)
  {
    /*
       0,1   1,1
       |-----|
     | \ / |
     |  X  |
     | / \ |
     ||-----|
       0,0   1,0
     */

    grid1_coords.push_back(makeVec<ctype, dimworld>(0,0));
    grid1_coords.push_back(makeVec<ctype, dimworld>(1,0));
    grid1_coords.push_back(makeVec<ctype, dimworld>(0,1));
    grid1_coords.push_back(makeVec<ctype, dimworld>(1,1));

    grid1_elements.push_back(0);
    grid1_elements.push_back(1);
    grid1_elements.push_back(2);
    grid1_elements.push_back(3);
    grid1_elements.push_back(2);
    grid1_elements.push_back(1);
    grid1_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
    grid1_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));

    grid2_coords.push_back(makeVec<ctype, dimworld>(0,0));
    grid2_coords.push_back(makeVec<ctype, dimworld>(0,1));
    grid2_coords.push_back(makeVec<ctype, dimworld>(1,0));
    grid2_coords.push_back(makeVec<ctype, dimworld>(1,1));

    grid2_elements.push_back(1);
    grid2_elements.push_back(3);
    grid2_elements.push_back(0);
    grid2_elements.push_back(2);
    grid2_elements.push_back(0);
    grid2_elements.push_back(3);
    grid2_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
    grid2_element_types.push_back(Dune::GeometryType(Dune::GeometryType::simplex, dim));
  }
};

/*
   we split the domain and target side into different subdomains and
   call the merger multiple times. This should ensure that the
   resulting merged grid is the same.

   This test currently only support coupling of elements of the same dimension
 */
template <class ctype, int dim, int dimworld>
void callMergerTwice(Merger<ctype, dim, dim, dimworld> * merger)
{
  std::cout << "============= callMergerTwice === dim "
            << dim << " === dimworld " << dimworld
            << " =============" << std::endl;

  //////////////////////////////////////////////////////////
  // setup grid info

  std::vector<Dune::FieldVector<ctype,dimworld> > grid1_coords;
  std::vector<unsigned int> grid1_elements;
  std::vector<Dune::GeometryType> grid1_element_types;
  std::vector<Dune::FieldVector<ctype,dimworld> > grid2_coords;
  std::vector<unsigned int> grid2_elements;
  std::vector<Dune::GeometryType> grid2_element_types;
  setupGrid<dim>::fill(grid1_coords, grid1_elements, grid1_element_types,
                       grid2_coords, grid2_elements, grid2_element_types);

  for (int i=0; i<2; i++)
  {

    //////////////////////////////////////////////////////////
    // start the actual build process

    merger->build(grid1_coords, grid1_elements, grid1_element_types,
                  grid2_coords, grid2_elements, grid2_element_types);

    std::cout << "created intersections: " << merger->nSimplices() << std::endl;

    //////////////////////////////////////////////////////////
    // verify data

    merger->clear();
  }

}

int main ()
{

#if HAVE_PSURFACE
  {
    typedef PSurfaceMerge<1,1,double> Merger;
    Merger merger;
    callMergerTwice(&merger);
  }

  {
    typedef PSurfaceMerge<1,2,double> Merger;
    Merger merger;
    callMergerTwice(&merger);
  }

  {
    typedef PSurfaceMerge<2,2,double> Merger;
    Merger merger;
    callMergerTwice(&merger);
  }

  {
    typedef PSurfaceMerge<2,3,double> Merger;
    Merger merger;
    callMergerTwice(&merger);
  }
#else
    #warning Not testing, because psurface backend is not available.
#endif

}
