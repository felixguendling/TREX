#!/usr/bin/python3
import argparse

def generate_bash_script(name, levels, imbalance, time_limit):
    hierarchy_parameters = ':'.join(['2'] * levels)
    distance_parameters = ':'.join(['1'] * levels)
    command = "./../ExternalLibs/KaHIP/deploy/kaffpa \\\n\
        ../../Datasets/{}/MLTB/compact_layout.graph.metis \\\n\
        --imbalance={} \\\n\
        --k={} \\\n\
        --preconfiguration=ssocial \\\n\
        --hierarchy_parameter_string={} \\\n\
        --distance_parameter_string={} \\\n\
        --output_filename=../../Datasets/{}/MLTB/partition{}_2.i{}.txt \\\n\
        --time_limit={}".format(name, imbalance, 2**levels, hierarchy_parameters, distance_parameters, name, levels, imbalance, time_limit)

    script_content = f'''#!/bin/bash
{command}
'''

    script_filename = f'{name}_{imbalance}_script.sh'
    with open(script_filename, 'w') as script_file:
        script_file.write(script_content)

    print(f'Bash script generated: {script_filename}')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate a bash script with a specified command.')
    parser.add_argument('name', type=str, help='Name of the Network')
    parser.add_argument('levels', type=int, help='Number of levels')
    parser.add_argument('--imbalance', type=int, default=5, help='Imbalance parameter (default: 5)')
    parser.add_argument('--time_limit', type=int, default=600, help='Time limit in seconds (default: 600)')

    args = parser.parse_args()
    assert args.levels > 1
    generate_bash_script(args.name, args.levels, args.imbalance, args.time_limit)

