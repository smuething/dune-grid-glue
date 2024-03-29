# Additional checks needed to build the module
AC_DEFUN([DUNE_GRID_GLUE_CHECKS],[
    AC_REQUIRE([DUNE_PATH_PSURFACE])
])


# Additional checks needed to find the module
AC_DEFUN([DUNE_GRID_GLUE_CHECK_MODULE],
[
    DUNE_CHECK_MODULES([dune-grid-glue], [grid-glue/merging/standardmerge.hh], [[
      &StandardMerge<double,1,1,1>::build;]])
])
