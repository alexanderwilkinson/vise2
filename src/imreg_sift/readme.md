./sift -v --frames /home/tlm/dev/imcomp/src/vlfeat_register/a.frame --descriptors /home/tlm/dev/imcomp/src/vlfeat_register/a.descr /home/tlm/dev/imcomp/src/vlfeat_register/a.pgm


$ cd /data/mybin/vlfeat/matlab_install/vlfeat-0.9.20
$ make clean
$ make clean && make MEX=/usr/local/MATLAB/R2015b/bin/mex

> run('/data/mybin/vlfeat/matlab_install/vlfeat-0.9.20/toolbox/vl_setup')
> vl_version verbose
