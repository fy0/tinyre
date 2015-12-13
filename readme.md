
# tinyre ver 0.9.0 alpha

A tiny regex engine.  
Plan to be compatible with "Secret Labs' Regular Expression Engine"(SRE for python).  

utf-8 supported.

**warning: current version is still unstable!!!**

**Supported**:
*    "."      Matches any character except a newline.
*    "*"      Matches 0 or more (greedy) repetitions of the preceding RE. Greedy means that it will match as many repetitions as possible.
*    "+"      Matches 1 or more (greedy) repetitions of the preceding RE.
*    "?"      Matches 0 or 1 (greedy) of the preceding RE.
*    "\\"     Either escapes special characters or signals a special sequence.
*    []       Indicates a set of characters.
*    [^]      A "^" as the first character indicates a complementing set.
*    (...)    Matches the RE inside the parentheses. The contents can be retrieved or matched later in the string.
*    {m,n}    Matches from m to n repetitions of the preceding RE.
*    {m,n}?   Non-greedy version of the above.
*    *?,+?,?? Non-greedy versions of the previous three special characters.
*    "^"      Matches the start of the string.
*    "$"      Matches the end of the string or just before the newline at the end of the string.


**Unsupported**:
*    "|"      A|B, creates an RE that will match either A or B.
*    (?ims) Set the I, M or S flag for the RE (see below).
*    (?:...)  Non-grouping version of regular parentheses.
*    (?P<name>...) The substring matched by the group is accessible by name.
*    (?P=name)     Matches the text matched earlier by the group named name.
*    (?#...)  A comment; ignored.
*    (?=...)  Matches if ... matches next, but doesn't consume the string.
*    (?!...)  Matches if ... doesn't match next.
*    (?<=...) Matches if preceded by ... (must be fixed length).
*    (?<!...) Matches if not preceded by ... (must be fixed length).
*    (?(id/name)yes|no) Matches yes pattern if the group with id/name matched, the (optional) no pattern otherwise.
*    Flag: DOTALL
*    Flag: IGNORECASE
*    Flag: MULTILINE


Some of the functions in this module takes flags as optional parameters:
*    I  IGNORECASE  Perform case-insensitive matching.
*    M  MULTILINE   "^" matches the beginning of lines (after a newline) as well as the string. "$" matches the end of lines (before a newline) as well as the end of the string.
*    S  DOTALL      "." matches any character at all, including the newline.


## Use

```C
#include "tinyre.h"

tre_Pattern* pattern;
tre_Match* match;

pattern = tre_compile("^(bb)*a", 0);
match = tre_match(pattern, "bbbbabc");

// Group  0: bbbba
// Group  1: bb
```

## Doc

[基础设计](https://github.com/fy0/tinyre/wiki/%E5%9F%BA%E7%A1%80%E8%AE%BE%E8%AE%A1)  
[TODO列表](https://github.com/fy0/tinyre/wiki/todo-%E5%88%97%E8%A1%A8)  
[更新记录](https://github.com/fy0/tinyre/wiki/%E6%9B%B4%E6%96%B0%E8%AE%B0%E5%BD%95)  

License：zlib

