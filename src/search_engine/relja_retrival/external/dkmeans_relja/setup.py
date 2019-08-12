import sys

if not (sys.version_info[0] >= 2 and sys.version_info[1] >= 5):
  print 'You must be running Python 2.5 or greater'
  sys.exit(-1)

def exit_with_error(s):
  print s
  sys.exit(-1)

def check_import(pkgname, pkgver, pkglongname):
    try:
        mod = __import__(pkgname)
    except ImportError:
        exit_with_error(
            "Can't find a local %s Python installation."
            "Please read carefully the ``README`` file "
            "and remember that we need the %s package "
            "to compile and run." % (pkglongname, pkglongname) )
    else:
        if mod.__version__ < pkgver:
            exit_with_error(
                "You need %(pkgname)s %(pkgver)s or greater to run!"
                % {'pkgname': pkglongname, 'pkgver': pkgver} )

    print ( "* Found %(pkgname)s %(pkgver)s package installed."
            % {'pkgname': pkglongname, 'pkgver': mod.__version__} )
    globals()[pkgname] = mod

check_import('numpy', '1.0.4', 'numpy')
#check_import('tables', '2.0.0', 'PyTables')
check_import('pypar', '2.1.4', 'Pypar')

from distutils.core import setup, Extension
import numpy
from numpy.distutils.misc_util import get_numpy_include_dirs

inc_dirs = ['dkmeans_relja/']
inc_dirs.extend(get_numpy_include_dirs())

extensions = [
  Extension("dkmeans_relja/nn",
            include_dirs=inc_dirs,
            sources=['dkmeans_relja/nn.cpp'],
            depends=['dkmeans_relja/jp_dist2.hpp','dkmeans_relja/jp_nn.hpp','dkmeans_relja/jp_nn_kdtree.hpp','dkmeans_relja/jp_stats.hpp'],
            extra_compile_args=['-msse2']
            ),
  Extension("dkmeans_relja/dkmeans_utils",
            include_dirs=inc_dirs,
            sources=['dkmeans_relja/dkmeans_utils.cpp'],
            depends=['dkmeans_relja/dkmeans_utils.hpp']
            )
]

setup(name='dkmeans_relja',
      version='0.15.1',
      description='Distributed K-means',
      author='James Philbin, Relja Arandjelovic',
      author_email='philbinj@gmail.com, relja@robots.ox.ac.uk',
      packages=['dkmeans_relja'],
      ext_modules=extensions)
