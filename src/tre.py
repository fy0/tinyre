# coding:utf-8

import _tre

DOTALL = 2

class TRE_Pattern:
    def __init__(self, regex, flag=0):
        self.pattern = regex
        self.__pattern__ = _tre._compile(regex, flag)
    def match(self,text):
        ret = _tre._match(self.__pattern__,text)
        if ret:
            return TRE_Match(ret)

class TRE_Match:
    def __init__(self,args):
        self.__data__ = args

    def group(self,i=0):
        if type(i) == int:
            return self.__data__[i][1]
        else:
            for n in self.__data__:
                if n[0] == i:
                    return n[1]

    def groups(self):
        d = self.__data__
        l = len(d)
        if l>1:
            ret = []
            for i in d[1:]:
                ret.append(i[1])
            return tuple(ret)

    def string(self):
        return self.__data__[0][1]

def compile(regex, flag=0):
    return TRE_Pattern(regex, flag)

def match(pattern, text, flag=0):
    if pattern.__class__ != TRE_Pattern:
        if pattern.__class__ == str:
            return TRE_Match(_tre._compile_and_match(pattern, text, flag))
        return None
    ret = _tre._match(pattern.__pattern__, text)
    if ret:
        return TRE_Match(ret)

