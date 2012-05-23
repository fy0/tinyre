
# tinyre  

A tiny regex engine.  
一个小型的有点特别的 python 风格正则引擎。  

当前能够支持 . ? * + | ^ $ () (?!) (?#) (?:) [] {} \D \d \S \s \W \w  及 分组

## 用于 Python

现在你可以将这个库用在python2.7中了。  

执行:
`cd src  
make -f Makefile.pymod  `

随后把tre.py和编译出来的_tre.so放在一起就能用了。  

用法参考原版re模块。

## 文档

### 面向开发者
[基础设计](https://github.com/fy0/TinyRe/wiki/%E5%9F%BA%E7%A1%80%E8%AE%BE%E8%AE%A1)  
[TODO列表](https://github.com/fy0/TinyRe/wiki/todo-%E5%88%97%E8%A1%A8)  
[程序设计变更记录](https://github.com/fy0/TinyRe/wiki/%E7%A8%8B%E5%BA%8F%E8%AE%BE%E8%AE%A1%E5%8F%98%E6%9B%B4%E8%AE%B0%E5%BD%95)  

### 面向使用者
[更新记录](https://github.com/fy0/TinyRe/wiki/%E6%9B%B4%E6%96%B0%E8%AE%B0%E5%BD%95)  

许可协议：**zlib license**

