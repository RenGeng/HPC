make

mpirun -n 4 -bynode ./bin/shalw --export --export-path /tmp/3408406

./visu.py /tmp/3408406/shalw_256x256_T1000.sav 

rm -f /tmp/3408406/shalw*