#!/bin/zsh

nnodes=25
simulation_runtime=30

echo "Starting HELOBA Simulation with $nnodes Nodes running for $simulation_runtime seconds"

rm search_log.txt
make -s clean
make -s build
repeat $nnodes { ./build/heloba < /dev/null &>> search_log.txt& }

nstarted=$(ps aux | rg heloba | wc -l)
echo "Started $nstarted Nodes"

sleep $simulation_runtime && pkill -SIGKILL heloba

completed_searches=$(rg "Took" search_log.txt | wc -l)
failed_searches=$(rg "find requested node" search_log.txt | wc -l)
total_searches=$(echo "$completed_searches + $failed_searches" | bc)
search_ratio=$(echo "scale=3 ; $completed_searches / $simulation_runtime" | bc)
echo "Successfully Completed Searches: $completed_searches/$total_searches ($search_ratio per second)"

echo "Search Stats - Hops needed"
grep -oP "Took\s+\K[0-9]+" search_log.txt | perl -M'List::Util qw(sum max min)' -MPOSIX -0777 -a -ne 'printf "%-7s : %d\n"x4, "Min", min(@F), "Max", max(@F), "Average", sum(@F)/@F,  "Median", sum( (sort {$a<=>$b} @F)[ int( $#F/2 ), ceil( $#F/2 ) ] )/2;'

nswaps=$(rg "Sending SWAP \| ACK" search_log.txt | wc -l)
echo "Conducted $nswaps SWAPs"
