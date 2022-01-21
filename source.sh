#!/usr/bin/bash
path_root=`pwd`
injector=$path_root/third-party/'injector'
libuwind=$path_root/third-party/'libunwind'
yamlcpp=$path_root/third-party/'yaml-cpp'

if [ ! -d $path_root/third-party ];then
    mkdir third-party
fi
cd third-party/
if [ ! -d $injector ];then
    git clone  https://github.com/kubo/injector.git
fi

if [ ! -d $libuwind ];then
    git clone  https://github.com/libunwind/libunwind.git
fi

if [ ! -d $yamlcpp ];then
    git clone  https://github.com/jbeder/yaml-cpp.git
fi

cd -

