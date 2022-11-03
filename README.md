# httpServer
#--------------------------------CN------------------------------------
#介绍
这是一个性能不是很好的在linux上运行的http服务器
最近学习网络时用于练手
对于刚刚开始学习linux socket编程的人应该会有些帮助
后续还会根据学习情况继续改进性能

#使用
下载：
    在github上下载:
        https://github.com/Dallous52/httpServer
    使用git clone:
        git clone https://github.com/Dallous52/httpServer.git
 
#运行测试:
  #请先将网页文件放入html文件夹中
    --转到文件目录
    cd httpServer
    
    --编译文件
    make
    
    --运行文件
    ./test
    
    --查看文件运行状况
    ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E "PID|test"
    
    --清理过程文件，日志文件
    make clean

#如果有问题无法解决，可以通过我的邮箱 huhao1226968736@outlook.com 联系

#--------------------------------EN------------------------------------

#Introduction

This is an http server running on Linux with poor performance
Recently, it was used to practice when learning online
It should be helpful to those who are just learning Linux socket programming
The performance will be further improved according to the learning situation

#Using
Download:
    Download on github:
        https://github.com/Dallous52/httpServer

    Use git clone:
        git clone https://github.com/Dallous52/httpServer.git


#Run test:
     #Please put the webpage file into the html folder first
    --Go to file directory
         cd httpServer

    --Compiled file
         make

    --Running files
         ./test

    --View file health
        ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E "PID|test"

    --Cleaning process files, log files
        make clean


#If there is a problem that cannot be solved, you can use my email “huhao1226968736@outlook.com” contact
