
# tinyre ver 0.8.1

A tiny regex engine.  
小型正则引擎，语法与python的sre近似，当前仅支持匹配(match)和替换(sub)。  

支持   : . ? * + | ^ $ () [] 分组 非贪婪匹配  
不支持 : {}  

## 作为源码使用

直接复制 tinyre.h 和 tinyre.c 到源码目录，  

并在程序中引用 tinyre.h  

## 作为链接库使用

执行:
`cd src`  
`make lib`  

引用头文件并配合 tinyre.so 使用。  

## 用于 Python 2.7

执行:  
`cd src`  
`make pylib27`  

将tre.py与_tre.so加入工程，就像使用re一样使用它。  

## 文档

[基础设计](https://github.com/fy0/tinyre/wiki/%E5%9F%BA%E7%A1%80%E8%AE%BE%E8%AE%A1)  
[TODO列表](https://github.com/fy0/tinyre/wiki/todo-%E5%88%97%E8%A1%A8)  
[更新记录](https://github.com/fy0/tinyre/wiki/%E6%9B%B4%E6%96%B0%E8%AE%B0%E5%BD%95)  

许可协议：zlib license

