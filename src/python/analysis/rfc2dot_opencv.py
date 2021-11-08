#!/usr/bin/env python3

import argparse
import cv2

def parse_node(node):
    if node.isMap():
        keys = node.keys()
        #print('map keys ', keys)
        dot = '{\n'
        for k in keys:
            dot += '%s: '%(k)
            dot += parse_node(node.getNode(k))
            dot += ',\n'
        dot += ' }\n'
    elif node.isSeq():
        #print('seq size ', node.size())
        dot = '[ '
        for k in range(0, node.size()):
            if k>0:
                dot += ', '
            dot += parse_node(node.at(k))
        dot += ' ]\n'
    else:
        if node.isInt():
            #print('int val ', int(node.real()))
            dot = '%d'%(int(node.real()))
        elif node.isReal():
            #print('real val ', node.real())
            dot = '%f'%(node.real())
        elif node.isString():
            #print('string val ', node.string())
            dot = '\"%s\"'%(node.real())
        else:
            assert(False)

    return dot


def parse_tree(tid, tree):
    dot = ''
    treenodes = tree.getNode('nodes')
    assert(treenodes.isSeq())
    # Create nodes.
    for i in range(0, treenodes.size()):
        node = treenodes.at(i)
        depth = int( node.getNode('depth').real() )
        dot += 'tree_%d_d_%d_n_%d [ label=\"' % (tid, depth, i)
        dot += parse_node(node)
        dot += '\" ];\n'
    # Create edges.
    stack = [(0,0)]
    for i in range(1, treenodes.size()):
        node = treenodes.at(i)
        depth = int( node.getNode('depth').real() )
        (j, d) = stack.pop()
        while d >= depth:
            (j, d) = stack.pop()

        dot += 'tree_%d_d_%d_n_%d -> '%(tid, d, j)
        dot += 'tree_%d_d_%d_n_%d;\n'%(tid, depth, i);
        stack.append((j,d))
        # append node if there are more nodes to traverse.
        if i < treenodes.size() - 1:
            stack.append((i,depth))

    (j,d) = stack.pop()
    assert((j,d) == (0,0))

    return dot

def parse(node):
    dot = 'root [ shape=box, label=\"'
    assert(node.isMap())
    keys = node.keys()
    keys.remove('trees')
    dot += '{\n'
    for k in keys:
        dot += '%s: '%(k)
        dot += parse_node(node.getNode(k))
        dot += ',\n'
    dot += ' }\n'

    dot += '\"];\n'

    for r in range(0, node.getNode('trees').size()):
        dot += parse_tree(r, node.getNode('trees').at(r))
        dot += 'root -> tree_%d_d_0_n_0;\n'%(r)
    return dot


def main():
    parser = argparse.ArgumentParser(description='Generate dot graph file from rtrees yaml.')
    parser.add_argument('-i', '--input', help='the input yaml file.', required=True)
    parser.add_argument('-o', '--output', help='the output dot file.', required=True)
    args = parser.parse_args()

    fs = cv2.FileStorage(args.input, cv2.FILE_STORAGE_READ) 

    fn = fs.getFirstTopLevelNode()

    dot = 'digraph G {' 
    dot += parse(fn);
    dot += '\n}'

    with open(args.output, 'w') as f:
        f.write(dot)

    fs.release()

if __name__ == "__main__":
    main()
