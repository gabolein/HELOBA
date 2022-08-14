#!/bin/bash

random_array_element() {
  local arr=("$@")
  printf '%s' "${arr[RANDOM % $#]}"
}

node_count=40
simulation_runtime=10

echo "Starting HELOBA Search Simulation with $node_count Nodes running for $simulation_runtime seconds"

rm simulation_log.txt
make -s clean
make -s simulation

pids=()
fifos=()
for i in $(seq $node_count); do
  fifo="/tmp/heloba_stdin$i"
  if [ ! -p "$fifo" ]; then
    mkfifo "$fifo"
  fi
  
  eval ./build/heloba < "$fifo" &>> simulation_log.txt&
  fifos+=("$fifo")
  pids+=($!)
  # Wait a little bit to not flood the network with too many concurrent joins
  sleep 0.01s
done

endtime=$(date -ud "$simulation_runtime second" +%s)
while [[ $(date -u +%s) -le $endtime ]]
do
  fifo=$(random_array_element "${fifos[@]}")
  pid=$(random_array_element "${pids[@]}")
  # pid=$(printf "%x" $(random_array_element "${pids[@]}") | perl -0777e 'print scalar reverse <>')
  # pid=$((16#$pid))
  pid_fmt=$(printf "%x:%x:%x:%x:%x:%x" $(((pid >> 0) & 0xff)) $(((pid >> 8) & 0xff)) $(((pid >> 16) & 0xff)) $(((pid >> 24) & 0xff)) $(((pid >> 32) & 0xff)) $(((pid >> 40) & 0xff)))
  echo "Commanding Node attached to $fifo to find Node $pid_fmt"
  echo "searchfor $pid_fmt" > "$fifo"
  sleep 0.01s
done

pkill -SIGKILL heloba

completed_searches=$(grep -c "Took" simulation_log.txt)
failed_searches=$(grep -c "find requested node" simulation_log.txt)
total_searches=$(echo "$completed_searches + $failed_searches" | bc)
search_ratio=$(echo "scale=3 ; $completed_searches / $simulation_runtime" | bc)
echo "Successfully Completed Searches: $completed_searches/$total_searches ($search_ratio per second)"

echo "Search Stats - Hops needed"
grep -oP "Took\s+\K[0-9]+" simulation_log.txt | perl -M'List::Util qw(sum max min)' -MPOSIX -0777 -a -ne 'printf "%-7s : %d\n"x4, "Min", min(@F), "Max", max(@F), "Average", sum(@F)/@F,  "Median", sum( (sort {$a<=>$b} @F)[ int( $#F/2 ), ceil( $#F/2 ) ] )/2;'

nswaps=$(grep -c "Swapping with" simulation_log.txt)
echo "Conducted $nswaps SWAPs"
