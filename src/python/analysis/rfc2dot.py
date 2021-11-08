#!/usr/bin/env python3

import argparse
import yaml
from dtree2dot import parse_tree


def parse_rfc(y):
    rfc = y['rfc']
    dot = 'rfc [ shape=oval, label=\"rfc\n'
    for k in rfc:
        if 'tree' in k:
            continue
        if k == 'data':
            continue
        dot += '%s: %s' % (k, rfc[k])
        dot += ',\n'
    dot += '\"];\n'

    for tree_idx in range(rfc['num_trees']):
        dot += parse_tree(rfc, 'tree_%s' % (tree_idx))
        dot += 'rfc -> tree_%s\n' % (tree_idx)

    return dot


def main():
    parser = argparse.ArgumentParser(
        description='Generate dot graph file from Apollo random forests yaml.')
    parser.add_argument(
        '-i', '--input', help='input yaml file.', required=True)
    parser.add_argument(
        '-o', '--output', help='output dot file.', required=True)
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        y = yaml.safe_load(f)

    dot = 'digraph G {\n'
    dot += parse_rfc(y)
    dot += '\n}'

    with open(args.output, 'w') as f:
        f.write(dot)


if __name__ == "__main__":
    main()
