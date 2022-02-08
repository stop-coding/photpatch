# hotpatch
The hotpatch of process tp fix functions on live.

## introduce
Function-level hotpatch are used to repair application processes online.
This approach avoids the risk of business interruption.

## Build

### get code 
```
./source.sh
```	
the code will download on directory "third-party/".


#### injector
```
 git clone  https://github.com/kubo/injector.git
 cd injector
 make
 make install
 cp src/linux/libinjector.* ../lib
```

#### libuwind

##### centos
 ```
    yum install libunwind-devel
 ```
 
##### ubuntu
 ```
 apt-get install libunwind-dev
 ```
##### build youself
it need gcc 4.9 or more...
```
git clone  https://github.com/libunwind/libunwind.git
```
More message, you could read the "libunwind/README.md"
 
#### yaml-cpp
 ```
	git clone  https://github.com/jbeder/yaml-cpp.git
	cd yaml-cpp
	mkdir build
	cd build
	cmake ../
	make -j
	cp libyaml-cpp.a ../../lib
 ```
 #### hotpatch
 cd project dir
 it need cmake3
```
mkdir build
cd build
cmake ../
make -j
```
- the directory of patch is a demo dynamic library that is patch file.
- the directory of target is a demo process which will be taking an patch.
- the directory of conf is a configure which describe message of patch and target.
 
## Testing
cd build
#### Run target
```
./bin/target
```

Then you can see log:
```
process id = [ 31107 ]
thread id = [ 31108 ]
thread id = [ 31109 ]
thread id = [ 31110 ]
i will running always ,id[5]....
i will running always ,id[6]....
i will running always ,id[7]....
i will running always ,id[5]....
```

#### edit conf/exmple-patch.yaml
```
apiVersion: test/v1
kind: hotpatch
describtion: "hongchunhua hotpatch"
metadata:
  name: "hot patch"
spec:
  - name: mytest
    path: /{your project dir}/build/lib/libpatch.so
    pre_patch_callback: false
    post_patch_callback: false
    pre_unpatch_callback: false
    post_unpatch_callback: false
    function:
    - newname: helloworld
      oldname: helloworld
      newaddr: 
      oldaddr: none	
```
#### Do patch
```
./bin/hotpatch -p 31107 -f ../conf/exmple-patch.yaml
```
You can get the log:
```
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.664666] hotpatch.cc:386          - "/root/develop/photpatch/build/lib/libpatch.so" successfully injected
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665176] hotpatch.cc:186          - Load pid[31221] with cfg[../conf/exmple-patch.yaml] success!
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665398] hotpatch.cc:128          - New Func -- start:0x7fbf473c8000, offset:0x725, size:36
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665417] hotpatch.cc:129          - Old Func -- start:0x0, offset:0x4009db, size:24
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665426] hotpatch.cc:130          - Get arch: X86_64
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665443] hotpatch.cc:152          - backup Func[helloworld] -- addr:0x4009db, size:0x18
[HOTPATCH][31324 ][INFO] [2022/02/08-23:37:49.665476] hotpatch.cc:43           - patch pid[31221] success!
```

#### Check result
you find the log of the target is running have change:
```
i will running always ,id[5]....
i will running always ,id[6]....
i will running always ,id[5]....
Hello world, I'm new patch, id[7].
Hello world, I'm new patch, id[6].
Hello world, I'm new patch, id[5].
Hello world, I'm new patch, id[7].
Hello world, I'm new patch, id[6].
Hello world, I'm new patch, id[5].
```

Wow!! it patch success!!








