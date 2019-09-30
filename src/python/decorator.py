#!/usr/bin/env python
from __future__ import print_function

def printstuff(fun):
    def fun_that_prints(*args, **kwargs):
        print("calling %s" % fun.__name__)
        fun(*args, **kwargs)
        print("done with %s." % fun.__name__)

    return fun_that_prints


def myfunction():
    print("MYFUNCTION")


def myfunction2():
    print("MYFUNCTION_decorated")

myfunction2 = printstuff(myfunction2)
    
    
if __name__ == "__main__":
    myfunction()

    print()
    print()
    myfunction2()

