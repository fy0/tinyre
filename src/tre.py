# coding:utf-8
# tinyre v0.9.1 wrapper
# fy, 2012-2013

import _tinyre

# flags
I = IGNORECASE = 2
M = MULTILINE = 8
S = DOTALL = 16


class TRE_Pattern:
    def __init__(self, pattern, flags=0):
        self.pattern = pattern
        self.__cpattern__ = _tinyre._compile(pattern, flags)

    def match(self, text, backtrack_limit=0):
        ret = _tinyre._match(self.__cpattern__, text, backtrack_limit)
        if ret:
            return TRE_Match(text, ret)


class TRE_Match:
    def __init__(self, text, data):
        groupspan = []
        groupdict = {}
        for i in data:
            info = i[1:]
            if i[0]:
                groupdict[i[0]] = info
            groupspan.append(info)
        self.__text__ = text
        self.__groupspan__ = groupspan
        self.__groupdict__ = groupdict

    def __get_text_by_index__(self, i):
        a, b = self.__groupspan__[i]
        return self.__text__[a:b]

    def __get_text_by_name__(self, i):
        a, b = self.__groupdict__[i]
        return self.__text__[a:b]

    def group(self, *indices):
        ret = []
        if len(indices) == 0:
            indices = {0}

        for i in indices:
            if type(i) == int:
                ret.append(self.__get_text_by_index__(i))
            elif type(i) == str:
                ret.append(self.__get_text_by_name__(i))
        if len(ret) == 1:
            return ret[0]
        else:
            return ret

    def groups(self):
        ret = []
        for i in self.__groupspan__:
            a, b = i
            ret.append(self.__text__[a:b])
        return ret

    def string(self):
        return self.__text__


def compile(pattern, flags=0):
    return TRE_Pattern(pattern, flags)


def match(pattern, text, flags=0, backtrack_limit=0):
    if pattern.__class__ != TRE_Pattern:
        if pattern.__class__ == str:
            pattern = compile(pattern, flags)
        else:
            return None
    return pattern.match(text, backtrack_limit)

