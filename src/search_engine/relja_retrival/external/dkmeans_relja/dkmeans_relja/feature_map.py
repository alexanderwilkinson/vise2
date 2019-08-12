import numpy as np;

def featureNoWrapper(feats):
    return feats;

def toHellinger(feats):
    if feats.dtype!='float32' and feats.dtype!='float64':
        d= feats.astype('float32');
    else:
        d= feats;
    if len(d.shape)==1:
        # single desc: (nDims,)
        res= np.sqrt( d/d.sum() );
    else:
        # many desc: (numDesc, nDims)
        res= np.sqrt( d / np.tile( np.reshape( d.sum(axis=1), (d.shape[0],1) ), (1,d.shape[1]) ) );
    res[ np.isnan(res) ]= 0; # just in case though it shouldn't happen
    return res;