#!/bin/bash
./../ExternalLibs/KaHIP/deploy/kaffpa \
        ../../Datasets/Berlin/MLTB/compact_layout.graph.metis \
        --imbalance=5 \
        --k=256 \
        --preconfiguration=ssocial \
        --hierarchy_parameter_string=2:2:2:2:2:2:2:2 \
        --distance_parameter_string=1:1:1:1:1:1:1:1 \
        --output_filename=../../Datasets/Berlin/MLTB/partition8_2.i5.txt \
        --time_limit=600
