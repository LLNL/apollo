import matplotlib
from itertools import cycle

matplotlib.rcParams['font.sans-serif'] = 'Gill Sans Mt'
matplotlib.rcParams['font.size'] = '10'

__lines = ["o-","*--","s-.","^:"]
#__hatch = ["", "/", "\\", "//", "\\\\", "-", "+"]
__hatch = ['/', '\\', '|', '-', '+', 'x', 'o', 'O', '.', '*']
__color = ['#348ABD', '#7A68A6', '#A60628', '#467821', '#CF4457', '#188487', '#E24A33']

LINES = cycle(__lines)
HATCH = cycle(__hatch)
COLOR = cycle(__color)


def reset_cycles():
    global COLOR
    global LINES
    global HATCH

    COLOR = cycle(__color)
    LINES = cycle(__lines)
    HATCH = cycle(__hatch)


def tuftestyle(ax):
    """
    Styles an axes object to have minimalist graph features.
    """

#    ax.grid(True, 'major', color='w', linestyle='-', linewidth=1.5,alpha=0.6)
#    ax.patch.set_facecolor('white')

    ax.spines["right"].set_visible(False)
    ax.spines["top"].set_visible(False)
    ax.spines["bottom"].set_visible(True)
    ax.spines["left"].set_visible(True)
    ax.tick_params(right='off', top='off')
    ax.xaxis.grid(False)

#    #restyle the tick lines
#    for line in ax.get_xticklines() + ax.get_yticklines():
#        line.set_markersize(4)
#        line.set_color("black")
#        line.set_markeredgewidth(1)
#
#    for line in ax.xaxis.get_ticklines(minor=True) + ax.yaxis.get_ticklines(minor=True):
#        line.set_markersize(0)
#
#    matplotlib.rcParams['xtick.direction'] = 'out'
#    matplotlib.rcParams['ytick.direction'] = 'out'
#    ax.xaxis.set_ticks_position('bottom')
#    ax.yaxis.set_ticks_position('left')
