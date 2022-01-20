# photpatch
进程级热补丁工具



## 第三方库编译

### 安装injector

##### 手动编译
```
 git clone  https://github.com/kubo/injector.git
 cd injector
 make
 make install
 cp src/linux/libinjector.* ../lib
```
##### 脚本自动编译
运行source.sh即可
```
./source.sh
```
 
 ### 安装 libuwind
 ###### centos
 ```
    yum install libunwind-devel
 ```
 ##### ubuntu
 ```
 apt-get install libunwind-dev
 ```
##### 手动编译
gcc需要4.9版本以上，centos7默认为4.8.2，可能无法编译通过

更新编译器,以centos为例
```

```