# Additional checks needed to build the module
AC_DEFUN([DUNE_GRID_GLUE_CHECKS],[
    AC_REQUIRE([ACX_CHECK_CGAL])
])


# Additional checks needed to find the module
AC_DEFUN([DUNE_GRID_GLUE_CHECK_MODULE],
[
    DUNE_CHECK_MODULES([dune-grid-glue], [glue/extractors/extractor.hh])    
])
