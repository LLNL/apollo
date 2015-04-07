import numpy as np

"""
This module contains constants specifying the datatypes used for loading RAJA
data.
"""

PATH_DTYPE = {'names': 
            ('problem size',
                'outer',
                'inner',
                'loop type',
                'set type',
                'path',
                'loop',
                'range size',
                'segments',
                'fracrange',
                'fracunstructured',
                'func size',
                'time'),
            'formats': 
            ('i4',
                'S64',
                'S64',
                'S64',
                'S64',
                'S1024',
                'S64',
                'i4',
                'i4',
                np.float64,
                np.float64,
                'i4',
                np.float64)}

LULESH_DTYPE = {'names': 
            ('problem size',
                'outer',
                'inner',
                'loop type',
                'set type',
                'loop',
                'range size',
                'segments',
                'fracrange',
                'fracunstructured',
                'func size',
                'time'),
            'formats': 
            ('i4',
                'S64',
                'S64',
                'S64',
                'S64',
                'S1024',
                'i4',
                'i4',
                np.float64,
                np.float64,
                'i4',
                np.float64)}

LULESH_XTYPE = {'names': 
            ('loop type', 'set type', 'range size', 'segments', 'fracrange',
                'fracunstructured',
                'func size',
                'best'),
            'formats': 
            ('S64', 'S64', 'i4', 'i4', np.float64, np.float64, 'i4', 'bool')}

LULESH_YTYPE = {'names': ('outer', 'inner', 'time'), 
            'formats': ('S64', 'S64', 'f8')}

ARES_DTYPE = {'names': 
            ('problem size',
                'outer',
                'inner',
                'loop type',
                'set type',
                'loop',
                'range size',
                'segments',
                'function size',
                'time'
                ),
            'formats': 
            ('i4',
                'S64',
                'S64',
                'S64',
                'S64',
                'S1024',
                'i4',
                'i4',
                'i4',
                np.float64)}

ARES_XTYPE = {'names': 
            ('problem size',
                'loop type',
                'set type',
                'range size',
                'segments',
                'function size',
                'best'),
            'formats': ('i4', 'S64', 'S64', 'i4', 'i4', 'i4', 'bool')}

ARES_YTYPE = {'names': ('outer', 'inner'), 'formats': ('S64', 'S64')}
