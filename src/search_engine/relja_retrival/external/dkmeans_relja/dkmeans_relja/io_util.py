import os
import numpy as np



# if new types are added - add to the end in order not to break previously created files!
supportedDtypes= np.array( [np.dtype(x) for x in ['uint8', 'uint16', 'uint32', 'uint64', 'float32', 'float64']] )
dtypeByteSizes= [1,2,4,8,4,8]

pntsHeaderSize= 4 + 1



def dtypeToCode(dtype):
    whereDtype= np.flatnonzero(supportedDtypes == dtype)
    if len(whereDtype)==0:
        raise RuntimeError, "unsupported type: %s, please use one of: %s" % (dtype, ",".join([str(d) for d in supportedDtypes]))
    return whereDtype[0]



def codeToDtype(dtypeCode):
    if dtypeCode >= len(supportedDtypes):
        return None, None
    dtype= supportedDtypes[dtypeCode]
    dtypeByteSize= dtypeByteSizes[dtypeCode]
    return dtype, dtypeByteSize



def dkmeans3_save_clusters(clst_fn, clst_data, iter_num, niters, pnt_shape, seed, distortion):
    clst_out = open(clst_fn, 'wb')

    # dtype code
    header = np.array( dtypeToCode(clst_data.dtype), 'uint8' )
    header.tofile(clst_out)

    # shape
    header= np.array( [clst_data.shape[0], clst_data.shape[1]], 'uint32' )
    header.tofile(clst_out)

    # additional info
    header= np.array( [iter_num, niters, pnt_shape[0], pnt_shape[1], seed], 'uint32' )
    header.tofile(clst_out)
    header= np.array( distortion, 'float32' )
    header.tofile(clst_out)

    # clst_data
    clst_data.tofile(clst_out)

    clst_out.close()



def dkmeans3_read_clusters(clst_fn, returnAll= False):
    clst_in = open(clst_fn, 'rb')

    # dtype code
    dtypeCode= np.fromfile(clst_in, 'uint8', 1)[0]
    dtype, dtypeByteSize= codeToDtype(dtypeCode)

    # shape
    clst_shape= np.fromfile(clst_in, 'uint32', 2)

    # additional info
    iter_num, niters, pnt_shape0, pnt_shape1, seed= np.fromfile(clst_in, 'uint32', 5)
    distortion= np.fromfile(clst_in, 'float32', 1)

    # clst_data
    clst_data= np.fromfile(clst_in, dtype, clst_shape[0]*clst_shape[1]).reshape(clst_shape[0], clst_shape[1])

    clst_in.close()

    if returnAll:
        return (clst_data, iter_num, niters, [pnt_shape0, pnt_shape1], seed, distortion)
    return (iter_num, clst_data, distortion)



def clusters_are_corrupt(clst_fn):
    clstHeaderSize= 1 + 4*2 + 5*4 + 4
    byteSize= os.path.getsize(clst_fn)
    if byteSize <= clstHeaderSize:
        return True

    clst_in = open(clst_fn, 'rb')

    dtypeCode= np.fromfile(clst_in, 'uint8', 1)[0]
    dtype, dtypeByteSize= codeToDtype(dtypeCode)
    if dtype == None:
        return True
    clst_shape= np.fromfile(clst_in, 'uint32', 2)

    clst_in.close()

    if byteSize - clstHeaderSize != dtypeByteSize * clst_shape[0] * clst_shape[1]:
        return True

    return False



class pointsObj:

    def __init__(self, pnts_fn):
        # do not load the file here, every worker will only need one portion
        self.pnts_fn= pnts_fn

        # only loading header stuff
        f= open(self.pnts_fn,'rb')
        self.ndims= np.fromfile(f, 'uint32', 1)[0]
        dtypeCode= np.fromfile(f, 'uint8', 1)[0]
        f.close()

        self.dtype, self.dtypeByteSize= codeToDtype(dtypeCode)
        if self.dtype == None:
            raise RuntimeError, "Unsupported dtypeCode %d, corrupt file?" % dtypeCode

        byteSize= os.path.getsize(self.pnts_fn)
        assert( (byteSize - pntsHeaderSize) % (self.ndims * self.dtypeByteSize) == 0 )
        self.npnts= (byteSize - pntsHeaderSize) / (self.ndims * self.dtypeByteSize)

        self.shape= [self.npnts, self.ndims]

    def __getitem__(self, val):
        if type(val)!=slice:
            val= slice(val, val+1, 1)
        ind= val.indices(self.npnts)
        assert(ind[2]==1)
        numToRead= ind[1]-ind[0]

        f= open(self.pnts_fn,'rb')
        f.seek( 4 + 1 + ind[0] * self.ndims * self.dtypeByteSize )
        pnts= np.fromfile(f, self.dtype, numToRead * self.ndims).reshape(numToRead, self.ndims)
        f.close()

        return pnts



def points_are_corrupt(pnts_fn):
    byteSize= os.path.getsize(pnts_fn)
    if byteSize <= pntsHeaderSize:
        return True

    f = open(pnts_fn,'rb')
    ndims= np.fromfile(f, 'uint32', 1)[0]
    dtypeCode= np.fromfile(f, 'uint8', 1)[0]
    f.close()

    dtype, dtypeByteSize= codeToDtype(dtypeCode)
    if dtype == None:
        return True

    byteSize= os.path.getsize(pnts_fn)
    if (byteSize - pntsHeaderSize) % (ndims * dtypeByteSize) != 0:
        return True

    return False



# functions for converting between previously used format for clusters/points (tables) and the new ones (simple hand-made binary)

def dkmeans3_save_clusters_tables(clst_fn, clst_data, iter_num, niters, pnt_shape, seed, distortion):
    import tables
    filters = tables.Filters(complevel=1, complib='zlib')
    clst_out = tables.openFile(clst_fn, 'w')

    clst_out.createCArray(clst_out.root, 'clusters', tables.Atom.from_dtype(clst_data.dtype), clst_data.shape, filters=filters)
    clst_out.root.clusters[:] = clst_data

    clst_out.root.clusters.attrs.iter = iter_num
    clst_out.root.clusters.attrs.niters = niters
    clst_out.root.clusters.attrs.pnt_shape = pnt_shape
    clst_out.root.clusters.attrs.distortion = distortion
    clst_out.root.clusters.attrs.seed = seed

    clst_out.close()
    del clst_out



def dkmeans3_read_clusters_tables(clst_fn, returnAll= False):
    import tables
    clst_obj = tables.openFile(clst_fn, 'r')

    iter_num = clst_obj.root.clusters.attrs.iter
    distortion = clst_obj.root.clusters.attrs.distortion

    clst_data = clst_obj.root.clusters[:]

    if returnAll:
        niters = clst_obj.root.clusters.attrs.niters
        pnt_shape = clst_obj.root.clusters.attrs.pnt_shape
        seed = clst_obj.root.clusters.attrs.seed

    clst_obj.close()
    del clst_obj

    if returnAll:
        return (clst_data, iter_num, niters, pnt_shape, seed, distortion)
    return (iter_num, clst_data, distortion)



def convert_clusters_tables_to_bin(clst_fn_tables, clst_fn_bin):
    clst_data, iter_num, niters, pnt_shape, seed, distortion= dkmeans3_read_clusters_tables(clst_fn_tables, returnAll= True)
    dkmeans3_save_clusters(clst_fn_bin, clst_data, iter_num, niters, pnt_shape, seed, distortion)

def convert_clusters_bin_to_tables(clst_fn_tables, clst_fn_bin):
    clst_data, iter_num, niters, pnt_shape, seed, distortion= dkmeans3_read_clusters(clst_fn_tables, returnAll= True)
    dkmeans3_save_clusters_tables(clst_fn_bin, clst_data, iter_num, niters, pnt_shape, seed, distortion)

def convert_points_tables_to_bin(pnts_fn_tables, pnts_fn_bin, pnts_step=50000):
    import tables
    fi= tables.openFile(pnts_fn_tables, 'r')
    for pnts in fi.walkNodes('/', classname='Array'):
        break
    npnts= pnts.shape[0]
    ndims= pnts.shape[1]
    dtype= pnts.atom.dtype

    fo= open(pnts_fn_bin, 'wb')
    header= np.array( ndims, 'uint32' )
    header.tofile(fo)
    header= np.array( dtypeToCode(dtype), 'uint8' )
    header.tofile(fo)

    for l in range(0, npnts, pnts_step):
        r= min(l+pnts_step, npnts)
        pnts[l:r].tofile(fo)

    fo.close()
    fi.close()
    del fi



if __name__=='__main__':
    import sys
    if len(sys.argv) <= 3 or not(sys.argv[1] in ['c2b','c2t','p2b']):
        print "Usage: python io_util.py conversion_type input_file output_file";
        print "Conversion_type: c2b|c2t|p2b"
        print "c2b: clusters from old tables to new binary"
        print "c2t: clusters from binary to old tables"
        print "p2b: points from old tables to new binary"
        raise ValueError

    operation= sys.argv[1]
    inputFile= os.path.expanduser(sys.argv[2])
    outputFile= os.path.expanduser(sys.argv[3])
    if not(os.path.exists(inputFile)):
        print "Input file doesn't exist, %s" % inputFile

    if operation=='c2b':
        convert_clusters_tables_to_bin(inputFile, outputFile)
    elif operation=='c2t':
        if clusters_are_corrupt(inputFile):
            print "Cluster file (binary format) is corrupt, %s" % inputFile
        convert_clusters_bin_to_tables(inputFile, outputFile)
    elif operation=='p2b':
        convert_points_tables_to_bin(inputFile, outputFile)
    else:
        assert(0)
