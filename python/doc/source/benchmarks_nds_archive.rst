:orphan:

.. _benchmarks_nds_archive:

##########################
NDSortedArchive Benchmarks
##########################

The following plots benchmark the performance of
:func:`~moocore.NDSortedArchive` for various query strategies.

The sequences are generated using :func:`~moocore.generate_ndset` and
:func:`~moocore.generate_sequence`.

The source code for the benchmarks below can be found at
https://github.com/multi-objective/moocore/tree/main/python/benchmarks .

Each archive is given the number of points in the x-axis, one at a time.  We
measure the CPU time required to archive the whole sequence of points and
divide it by the number of points, which gives an estimation of the time to
process each point for different input sizes.

2D
~~

|nds_forest_add_bench-each-dominates-next-2d-time| |nds_forest_add_bench-each-dominates-previous-2d-time|

|nds_forest_add_bench-glas2017,c=0.5-2d-time| |nds_forest_add_bench-glas2017,c=1-2d-time|

|nds_forest_add_bench-glas2017,c=2.0-2d-time| |nds_forest_add_bench-hypercube-ndsort=-1-2d-time|

|nds_forest_add_bench-hypercube-ndsort=1-2d-time| |nds_forest_add_bench-hypercube-uniform-2d-time|

|nds_forest_add_bench-hypercube-uniform-rep4-2d-time| |nds_forest_add_bench-within-concave-sphere-2d-time|

.. |nds_forest_add_bench-each-dominates-next-2d-time|     image:: _static/bench/nds_forest_add_bench-each-dominates-next-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-each-dominates-previous-2d-time| image:: _static/bench/nds_forest_add_bench-each-dominates-previous-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=0.5-2d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=0.5-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=1-2d-time|            image:: _static/bench/nds_forest_add_bench-glas2017,c=1-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=2.0-2d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=2.0-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=-1-2d-time|     image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=-1-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=1-2d-time|      image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=1-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-2d-time|       image:: _static/bench/nds_forest_add_bench-hypercube-uniform-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-rep4-2d-time|  image:: _static/bench/nds_forest_add_bench-hypercube-uniform-rep4-2d-time.png
   :width: 49%

.. |nds_forest_add_bench-within-concave-sphere-2d-time|   image:: _static/bench/nds_forest_add_bench-within-concave-sphere-2d-time.png
   :width: 49%



3D
~~

|nds_forest_add_bench-each-dominates-next-3d-time| |nds_forest_add_bench-each-dominates-previous-3d-time|

|nds_forest_add_bench-glas2017,c=0.5-3d-time| |nds_forest_add_bench-glas2017,c=1-3d-time|

|nds_forest_add_bench-glas2017,c=2.0-3d-time| |nds_forest_add_bench-hypercube-ndsort=-1-3d-time|

|nds_forest_add_bench-hypercube-ndsort=1-3d-time| |nds_forest_add_bench-hypercube-uniform-3d-time|

|nds_forest_add_bench-hypercube-uniform-rep4-3d-time| |nds_forest_add_bench-within-concave-sphere-3d-time|

.. |nds_forest_add_bench-each-dominates-next-3d-time|     image:: _static/bench/nds_forest_add_bench-each-dominates-next-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-each-dominates-previous-3d-time| image:: _static/bench/nds_forest_add_bench-each-dominates-previous-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=0.5-3d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=0.5-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=1-3d-time|            image:: _static/bench/nds_forest_add_bench-glas2017,c=1-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=2.0-3d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=2.0-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=-1-3d-time|     image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=-1-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=1-3d-time|      image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=1-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-3d-time|       image:: _static/bench/nds_forest_add_bench-hypercube-uniform-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-rep4-3d-time|  image:: _static/bench/nds_forest_add_bench-hypercube-uniform-rep4-3d-time.png
   :width: 49%

.. |nds_forest_add_bench-within-concave-sphere-3d-time|   image:: _static/bench/nds_forest_add_bench-within-concave-sphere-3d-time.png
   :width: 49%

5D
~~

|nds_forest_add_bench-each-dominates-next-5d-time| |nds_forest_add_bench-each-dominates-previous-5d-time|

|nds_forest_add_bench-glas2017,c=0.5-5d-time| |nds_forest_add_bench-glas2017,c=1-5d-time|

|nds_forest_add_bench-glas2017,c=2.0-5d-time| |nds_forest_add_bench-hypercube-ndsort=-1-5d-time|

|nds_forest_add_bench-hypercube-ndsort=1-5d-time| |nds_forest_add_bench-hypercube-uniform-5d-time|

|nds_forest_add_bench-hypercube-uniform-rep4-5d-time| |nds_forest_add_bench-within-concave-sphere-5d-time|

.. |nds_forest_add_bench-each-dominates-next-5d-time|     image:: _static/bench/nds_forest_add_bench-each-dominates-next-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-each-dominates-previous-5d-time| image:: _static/bench/nds_forest_add_bench-each-dominates-previous-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=0.5-5d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=0.5-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=1-5d-time|            image:: _static/bench/nds_forest_add_bench-glas2017,c=1-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=2.0-5d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=2.0-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=-1-5d-time|     image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=-1-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=1-5d-time|      image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=1-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-5d-time|       image:: _static/bench/nds_forest_add_bench-hypercube-uniform-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-rep4-5d-time|  image:: _static/bench/nds_forest_add_bench-hypercube-uniform-rep4-5d-time.png
   :width: 49%

.. |nds_forest_add_bench-within-concave-sphere-5d-time|   image:: _static/bench/nds_forest_add_bench-within-concave-sphere-5d-time.png
   :width: 49%


10D
~~~

|nds_forest_add_bench-each-dominates-next-10d-time| |nds_forest_add_bench-each-dominates-previous-10d-time|

|nds_forest_add_bench-glas2017,c=0.5-10d-time| |nds_forest_add_bench-glas2017,c=1-10d-time|

|nds_forest_add_bench-glas2017,c=2.0-10d-time| |nds_forest_add_bench-hypercube-ndsort=-1-10d-time|

|nds_forest_add_bench-hypercube-ndsort=1-10d-time| |nds_forest_add_bench-hypercube-uniform-10d-time|

|nds_forest_add_bench-hypercube-uniform-rep4-10d-time| |nds_forest_add_bench-within-concave-sphere-10d-time|

.. |nds_forest_add_bench-each-dominates-next-10d-time|     image:: _static/bench/nds_forest_add_bench-each-dominates-next-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-each-dominates-previous-10d-time| image:: _static/bench/nds_forest_add_bench-each-dominates-previous-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=0.5-10d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=0.5-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=1-10d-time|            image:: _static/bench/nds_forest_add_bench-glas2017,c=1-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-glas2017,c=2.0-10d-time|          image:: _static/bench/nds_forest_add_bench-glas2017,c=2.0-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=-1-10d-time|     image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=-1-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-ndsort=1-10d-time|      image:: _static/bench/nds_forest_add_bench-hypercube-ndsort=1-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-10d-time|       image:: _static/bench/nds_forest_add_bench-hypercube-uniform-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-hypercube-uniform-rep4-10d-time|  image:: _static/bench/nds_forest_add_bench-hypercube-uniform-rep4-10d-time.png
   :width: 49%

.. |nds_forest_add_bench-within-concave-sphere-10d-time|   image:: _static/bench/nds_forest_add_bench-within-concave-sphere-10d-time.png
   :width: 49%
