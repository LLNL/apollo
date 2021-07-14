#!/usr/bin/env python3



from apollo.config import VERBOSE
from apollo.config import DEBUG
from apollo.config import FRAME_INTERVAL
from apollo.config import ONCE_THEN_EXIT


def tablePrint(results):
    # Print out the results in a pretty column-aligned way:
    widths = [max(map(len, str(col))) for col in zip(*results)]
    for row in results:
        print("  ".join((val.ljust(width) for val, width in zip(row, widths))))
    #
    return


def progressBar(amount, total, length, fill='='):
    if amount >= total:
        return fill * length
    if length < 4: length = 4
    fillLen = int(length * amount // total)
    emptyLen = length - 1 - fillLen
    bar = (fill * fillLen) + ">" + ("-" * emptyLen)
    return bar


