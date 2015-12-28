
# tinyre ver 0.9.0

A tiny regex engine.  
Plan to be compatible with "Secret Labs' Regular Expression Engine"(SRE for python).  

**warning: current version is still unstable!!!**

**Features**:  
* **utf-8 support**  
  Cheers for unicode!  

* **no octal number**  
  \\1 means group 1, \\1-100 means group n, \\01 match \\1, \\07 match \\7, \\08 match ['\\0', '8'], \\377 match 0o377, but \\400 isn't match with 0o400 and [chr(0o40), '\\0']!  
  What the hell ... I choose go die! Go away octal number!  

* **custom maximum number of backtracking**  
  An evil regex: **'a?'\*n+'a'\*n** against **'a'\*n**  
  For example: **'a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?a?aaaaaaaaaaaaaaaaaaaaaaaaa'** matches **'aaaaaaaaaaaaaaaaaaaaaaaaa'**  
  It will takes a long time because of too many times of backtracking. Perl/Python/PCRE requires over **10^15 years** to match a 29-character string.  
  You can set a limit to backtracking times to avoid this situation, and the match will be falied.  

* **more than 100 groups ...**  
  but who cares?  


**Supported**:
*    "."      Matches any character except a newline.
*    "^"      Matches the start of the string.
*    "$"      Matches the end of the string or just before the newline at the end of the string.
*    "*"      Matches 0 or more (greedy) repetitions of the preceding RE. Greedy means that it will match as many repetitions as possible.
*    "+"      Matches 1 or more (greedy) repetitions of the preceding RE.
*    "?"      Matches 0 or 1 (greedy) of the preceding RE.
*    *?,+?,?? Non-greedy versions of the previous three special characters.
*    {m}      Matches m copies of the previous RE.  
*    {m,n}    Matches from m to n repetitions of the preceding RE.
*    {m,n}?   Non-greedy version of the above.
*    "\\"     Either escapes special characters or signals a special sequence.
*    "\\1-N"  Matches the text matched earlier by the group index.  
*    []       Indicates a set of characters.  
*    [^]      A "^" as the first character indicates a complementing set.  
*    "|"      A|B, creates an RE that will match either A or B.  
*    (...)    Matches the RE inside the parentheses. The contents can be retrieved or matched later in the string.  
*    (?ims)   Set the I, M or S flag for the RE (see below).  
*    (?:...)  Non-grouping version of regular parentheses.  
*    (?P<name>...) The substring matched by the group is accessible by name.  
*    (?P=name)     Matches the text matched earlier by the group named name.
*    (?#...)  A comment; ignored.  
*    (?=...)  Matches if ... matches next, but doesn't consume the string.  
*    (?!...)  Matches if ... doesn't match next.  
*    (?<=...) Matches if preceded by ... (must be fixed length).  
*    (?<!...) Matches if not preceded by ... (must be fixed length).  
*    (?(id/name)yes|no) Matches yes pattern if the group with id/name matched, the (optional) no pattern otherwise.  
*    \\d \\D \\w \\W \\s \\S  
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
match = tre_match(pattern, "bbbbabc", 0);

// Group  0: bbbba
// Group  1: bb
```

## Doc

[基础设计](https://github.com/fy0/tinyre/wiki/%E5%9F%BA%E7%A1%80%E8%AE%BE%E8%AE%A1)  
[TODO列表](https://github.com/fy0/tinyre/wiki/todo-%E5%88%97%E8%A1%A8)  
[更新记录](https://github.com/fy0/tinyre/wiki/%E6%9B%B4%E6%96%B0%E8%AE%B0%E5%BD%95)  

License：zlib
