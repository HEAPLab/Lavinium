cur_dir=`pwd`

# stty -F /dev/ttyACM0 9600

echo "Lavinium log" > progress.log

build () {
  echo "`date`: Running $policy" >> progress.log
  echo "Finished Lav analysis"
  # move results 
  mv results.csv /home/buoffio/e2_studio/workspace/Lavinium/compiled_outs/new_total/results_$policy.csv

  echo "Building elf"
  # build elf with current optimizations
  cd /home/buoffio/e2_studio/workspace/Lavinium/Lavinium
  make clean
  make all
  mv Lavinium.elf /home/buoffio/e2_studio/workspace/Lavinium/compiled_outs/Lavinium_$policy.elf
  cp Lavinium.srec /home/buoffio/e2_studio/workspace/Lavinium/compiled_outs/Lavinium_$policy.srec

  echo "Flashing board"
  # flash the board
  /home/buoffio/Projects/Rust/DumpSerial/JLinkExe -CommandFile /home/buoffio/Projects/Rust/DumpSerial/flash.jlink

  echo "Running serial"
  #gather the results
  /home/buoffio/Projects/Rust/DumpSerial/target/release/dump_serial /dev/ttyACM0 9600 "RESULTS_END" 600 > /home/buoffio/e2_studio/workspace/Lavinium/compiled_outs/execution_$policy.txt 
  cd $cur_dir
  echo "`date`: Done $policy" >> progress.log
}

apply_csv() {
  ./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='apply-csv' -mllvm -csv-file='/home/buoffio/e2_studio/workspace/Lavinium/compiled_outs/total/results_$policy.csv'"
}

policy=random
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='$policy' -mllvm -rand-sample=10 -mllvm -sequence-length=10"
#build $policy

policy=genetic
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='$policy' -mllvm -genetic-pool=200 -mllvm -genetic-sample=1000 -mllvm -sequence-length=10"
#build $policy

policy=association
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='$policy' -mllvm -assocrule-pool=200 -mllvm -assoc-sample=1000 -mllvm -sequence-length=10"
#build $policy

policy=assoc-cleaning
#apply_csv $policy
#echo running $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='association' -mllvm -assocrule-pool=200 -mllvm -assoc-sample=1000 -mllvm -sequence-length=10 -mllvm -assoc-clean=true"
#mv results.csv results_polybench_rtss/results_$policy.csv
#build $policy

policy=greedy
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='$policy' -mllvm -lavinium-depth=10"
#build $policy
                                                                                                              
policy=cartesian-pruned
#apply_csv $policy
./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='$policy' -mllvm -lavinium-depth=10"
mv results.csv results_polybench_rtss/results_$policy.csv
#build $policy

policy=o0
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='none'" -opt-level=0 
#build $policy

policy=o1
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='none'" -opt-level=1 
#build $policy

policy=o2
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='none'" -opt-level=2
#build $policy

policy=o3
#apply_csv $policy
#./run_mt.py -cdir PolyBench -compile -custom_args="-mllvm -lavinium-strategy='none'" -opt-level=3 
#build $policy