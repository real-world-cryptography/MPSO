#!/bin/bash


k_values=(3 4 5 10)
#k_values=(3)
nn_values=$(seq 12 2 20)


for k in "${k_values[@]}"; do
    
    for nn in $nn_values; do
        echo "Running tests with k=$k, nn=$nn"
        
        
        echo "  Running pre-generation..."
        pregen_pids=()
        for ((r=0; r<k; r++)); do
            ./test_mpsic -preGen -k $k -nn $nn -nt 4 -r $r &
            pregen_pids+=($!)
        done
        
        
        for pid in "${pregen_pids[@]}"; do
            wait $pid
        done
        
        
        echo "  Running actual tests..."
        test_pids=()
        for ((r=0; r<k; r++)); do
            ./test_mpsic -k $k -nn $nn -nt 4 -r $r &
            test_pids+=($!)
        done
        
        
        for pid in "${test_pids[@]}"; do
            wait $pid
        done
        
        echo "  Completed tests for k=$k, nn=$nn"
        echo ""
    done
done

echo "All tests completed!"
