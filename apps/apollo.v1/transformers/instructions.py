import pandas as pd


INSTRUCTION_GROUPS = {
        'add': ['add', 'addsd'],
        'sub': ['sub', 'subsd'],
        'mul': ['imul', 'mulsd'],
        'div': ['divsd'],
        'conditional jmp': ['jb/jnaej/j', 'jbe', 'jnb/jae/j', 'jnbe', 'jnz', 'jp', 'jz', 'jl', 'jle', 'jnb', 'jnl'],
        'jmp': ['jmp'],
        'mov': ['mov', 'movsd', 'movsxd', 'lea'],
        'vector mov': ['movapd'],
        'call': ['call'],
        'logic': ['and', 'xorpd', 'neg', 'pxor', 'test'],
        'compare': ['cmp', 'comisd', 'ucomisd'],
        'misc': ['push'],
        'return': ['leave', 'ret']
        }

def coarsen_instruction_data(instruction_data):
    columns = list(instruction_data)

    grouped_columns = {}

    for group in INSTRUCTION_GROUPS:
        grouped_columns[group] = []
        for val in columns:
            if val in INSTRUCTION_GROUPS[group]:
                grouped_columns[group].append(val)

    for group in grouped_columns:
        instruction_data[group] = instruction_data[grouped_columns[group]].sum(axis=1)
        instruction_data = instruction_data.drop(grouped_columns[group], axis=1)

    return instruction_data
