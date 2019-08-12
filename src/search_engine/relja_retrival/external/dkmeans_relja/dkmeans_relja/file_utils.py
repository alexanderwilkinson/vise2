"""
File related utilities.
"""

import os

def remove_if_newer(fn, parent_fns):
  return
  if not os.path.exists(fn): return

  for pfn in parent_fns:
    if os.stat(fn)[os.path.stat.ST_CTIME] < os.stat(pfn)[os.path.stat.ST_CTIME]:
      os.remove(fn)
      return

def remove_if_corrupt_points(fn):
    if not os.path.exists(fn): return

    from io_util import points_are_corrupt
    try:
        if points_are_corrupt(fn):
            os.remove(fn)
    except IOError, e:
        os.remove(fn)

def remove_if_corrupt_clusters(fn):
    if not os.path.exists(fn): return

    from io_util import clusters_are_corrupt
    try:
        if clusters_are_corrupt(fn):
            os.remove(fn)
    except IOError, e:
        os.remove(fn)
