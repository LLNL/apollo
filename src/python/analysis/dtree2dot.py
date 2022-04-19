#!/usr/bin/env python3

import argparse
import yaml


def parse_node(node, name):
    dot = name + ' [ shape=box, label=\"' + name + '\n'
    for k in node:
        if k == 'left' or k == 'right':
            continue
        dot += '%s: %s' % (k, node[k])
        dot += ',\n'

    dot += '\"];\n'

    if 'left' in node:
        dot += parse_node(node['left'], name + '_left')
        dot += name + ' -> ' + name + '_left\n'

    if 'right' in node:
        dot += parse_node(node['right'], name + '_right')
        dot += name + ' -> ' + name + '_right\n'

    return dot


def parse_tree(y, name):
    tree = y['tree']
    dot = name + ' [ shape=oval, label=\"' + name + '\n'
    for k in tree:
        if k == 'root':
            continue
        if k == 'data':
            continue
        dot += '%s: %s' % (k, tree[k])
        dot += ',\n'
    dot += '\"];\n'

    dot += parse_node(tree['root'], name + '_root')
    dot += name + ' -> ' + name + '_root\n'

    return dot


def main():
    parser = argparse.ArgumentParser(
        description='Generate dot graph file from Apollo decision trees yaml.')
    parser.add_argument(
        '-i', '--input', help='input yaml file.', required=True)
    parser.add_argument(
        '-o', '--output', help='output dot file.', required=True)
    args = parser.parse_args()

    with open(args.input, 'r') as f:
        y = yaml.safe_load(f)

    dot = 'digraph G {\n'
    dot += parse_tree(y, 'tree')
    dot += '\n}'

    with open(args.output, 'w') as f:
        f.write(dot)


if __name__ == "__main__":
    main()
