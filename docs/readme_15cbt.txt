mpirun -np 12 compute_index_v2 trainDescs 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg
mpirun -np 12 python /home/tlm/dev/vise/src/search_engine/relja_retrival/src/v2/indexing/compute_clusters.py 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg 30
#mpirun -np 12 compute_index_v2 trainAssign 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg
./compute_index_v2 trainAssign 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg

#mpirun -np 12 compute_index_v2 trainHamm 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg
./compute_index_v2 trainHamm 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg

mpirun -np 12 compute_index_v2 index 15cbt_26jun2018 /media/tlm/data/dataset/15cbt/vise_data/15cbt_26jun2018/training_data/vise_config.cfg

LD_LIBRARY_PATH="/home/tlm/deps/vise/lib/lib" ./api_v2 9999 ox5k /home/tlm/dset/vise/ox5k_100/search_engine_config.txt

python webserver.py 8080 ox5k1 9999 0 /home/tlm/dset/vise/ox5k_100/search_engine_config.txt


curl -X POST localhost:9973/vise/repo/ox5k/1/add_image?filename="all_souls_00001.jpg"


