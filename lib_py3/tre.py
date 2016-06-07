# coding:utf-8
# tinyre v0.9.0 wrapper

import _tinyre

# flags
I = IGNORECASE = 2
M = MULTILINE = 8
S = DOTALL = 16

class error(Exception):
    msg = ''

class TRE_Pattern:
    @classmethod
    def __new_pattern__(self, pattern, flags=0):
        if flags > I|M|S:
            flags = (flags & I) | (flags & M) | (flags & S)
        ret = _tinyre._compile(pattern, flags)
        if type(ret) == int:
            raise error(ret)
        else:
            ptn = TRE_Pattern()
            ptn.__cpattern__ = ret
            ptn.pattern = pattern
            return ptn

    def match(self, text, backtrack_limit=0):
        ret = _tinyre._match(self.__cpattern__, text, backtrack_limit)
        if ret:
            return TRE_Match(text, ret)


class TRE_Match:
    def __init__(self, match_text, data):
        groupspan = []
        grouptext = []
        groupdict = {}
        default_slots = []
        for i in data:
            name, a, b = i
            span = (a, b)
            text = match_text[a:b] if a is not None else None
            if i[0]:
                groupdict[i[0]] = (span, text)
            if span[0] is None:
                default_slots.append(True)
            else:
                default_slots.append(False)
            groupspan.append(span)
            grouptext.append(text)
        self.__text__ = match_text
        self.__groupspan__ = tuple(groupspan)
        self.__grouptext__ = tuple(grouptext)
        self.__groupdict__ = groupdict
        self.__default_slots__ = default_slots
        self.lastindex = None

        if groupspan:
            index = 0
            for i in groupspan[1:]:
                if i[0]:
                    index += 1
                    self.lastindex = index

    def __get_text_by_index__(self, i):
        return self.__grouptext__[i]

    def __get_text_by_name__(self, i):
        if i in self.__groupdict__:
            return self.__groupdict__[i][1]
        else:
            return None

    def span(self, index=0):
        a, b = self.__groupspan__[index]
        if a is None:
            return -1, -1
        return a, b

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
            return tuple(ret)

    def groups(self, default=None):
        if default is None:
            return self.__grouptext__[1:]
        else:
            ret = list(self.__grouptext__[1:])
            for i in range(1, len(self.__default_slots__)):
                if self.__default_slots__[i]:
                    ret[i-1] = default
            return tuple(ret)

    def groupdict(self):
        ret = {}
        for k, v in self.__groupdict__.items():
            ret[k] = v[1]
        return ret

    def start(self, index=0):
        return self.span(index)[0]

    def end(self, index=0):
        return self.span(index)[1]

    def string(self):
        return self.__text__


def compile(pattern, flags=0):
    return TRE_Pattern.__new_pattern__(pattern, flags)


def match(pattern, text, flags=0, backtrack_limit=0):
    if pattern.__class__ != TRE_Pattern:
        if pattern.__class__ == str:
            pattern = compile(pattern, flags)
        else:
            return None
    return pattern.match(text, backtrack_limit)

