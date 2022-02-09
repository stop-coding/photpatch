/*
 * Copyright(C)  All rights reserved.
 */

/*!
* \file 
* \brief 
apiVersion: test/v1
kind: hotpatch
describtion: "hongchunhua hotpatch"
metadata:
  name: "hot patch"

spec:
  #补丁名称 name
  - name: mytest
    #补丁库，包含替换的函数实体
    patch: /root/develop/photpatch/lib/test.so
    #目标文件，预留
    target: /root/develop/photpatch/bin/test
    pre_patch_callback: None
    post_patch_callback: None
    pre_unpatch_callback: None
    post_unpatch_callback: None
    #需要更新的函数
    function:
    - newname: my_test_func
      #可选,为空默认新旧名称一样
      oldname: none
      #可选，默认通过函数名查找跳转地址
      newaddr: 
      oldaddr: none
* 
* 包含..
*
* \copyright All rights reserved.
* \author hongchunhua
* \version v1.0.0
* \note none 
*/

#include <string>
#include <map>
#include <list>
#include "yaml-cpp/yaml.h"
#pragma once

namespace YAML {

struct hp_head { 
    std::string version;
    std::string kind;
    std::string describtion;
    std::map<std::string, std::string> meta;
};
template<>
struct convert<hp_head> {
  static Node encode(const hp_head& in) {
    Node node;
    node["apiVersion"] = in.version;
    node["kind"] = in.kind;
    node["describtion"] = in.describtion;

    Node meta;
    for (auto iter=in.meta.begin(); iter != in.meta.end(); iter++) {
        meta[iter->first] = iter->second;
    }
    node["metadata"] = meta;
    return node;
  }

  static bool decode(const Node& node, hp_head& in) {
    if(!node.IsMap() || node.size() < 3) {
      return false;
    }
    if (node["apiVersion"]) {
      in.version = node["apiVersion"].as<std::string>();
    } else {
      return false;
    }

    if (node["kind"]) {
      in.kind = node["kind"].as<std::string>();
    } else {
      return false;
    }

    if (node["describtion"]) {
      in.describtion = node["describtion"].as<std::string>();
    }

    if (node["metadata"] && node["metadata"].IsMap()) {
      for (auto it = node["metadata"].begin(); it != node["metadata"].end(); it++) {
        in.meta[it->first.as<std::string>()] = it->second.as<std::string>();
      }
    }
    return true;
  }
};

/*
    function:
    - newname: my_test_func  #可选，为空的话，需要确保地址不为空
      oldname: None  #可选,为空默认新旧名称一样
      newaddr: None #可选，默认通过函数名查找跳转地址,地址存在则优先使用地址
      oldaddr: None #可选，默认通过函数名查找跳转地址
*/
struct hp_func { 
    std::string new_name;
    std::string old_name;
    uint64_t new_addr = 0;
    uint64_t old_addr = 0;
};

template<>
struct convert<hp_func> {
  static Node encode(const hp_func& in) {
    Node node;
    node["newname"] = in.new_name;
    node["oldname"] = in.old_name;
    node["newaddr"] = in.new_addr;
    node["oldaddr"] = in.old_addr;
    return node;
  }

  static bool decode(const Node& node, hp_func& in) {
    if(!node.IsMap() || node.size() == 0 ) {
      return false;
    }

    if (!node["newname"] && !node["oldname"]) {
      if (!node["newaddr"] && !node["oldaddr"]) {
        return false;
      }
    }

    if (node["newname"]) {
      in.new_name = node["newname"].as<std::string>();
    } 

    if (node["oldname"]) {
      in.old_name = node["oldname"].as<std::string>();
    } 
    if (node["newaddr"]) {
      in.new_addr = node["newaddr"].as<uint64_t>();
    }

    if (node["oldaddr"]) {
      in.old_addr = node["oldaddr"].as<uint64_t>();
    }

    return true;
  }
};

/*
name: mytest
#补丁库，包含替换的函数实体
patch: /root/develop/photpatch/lib/test.so
#目标文件，预留,可选
target: /root/develop/photpatch/bin/test
pre_patch_callback: None
post_patch_callback: None
pre_unpatch_callback: None
post_unpatch_callback: None
*/
struct hp_patch { 
    std::string name;
    std::string patch;
    std::string target;
    bool pre_patch_callback;
    bool post_patch_callback;
    bool pre_unpatch_callback;
    bool post_unpatch_callback;
    std::vector<hp_func> funcs;
};

template<>
struct convert<hp_patch> {
  static Node encode(const hp_patch& in) {
    Node node;
    node["name"] = in.name;
    node["patch"] = in.patch;
    node["target"] = in.target;
    node["pre_patch_callback"] = in.pre_patch_callback;
    node["post_patch_callback"] = in.post_patch_callback;
    node["pre_unpatch_callback"] = in.pre_unpatch_callback;
    node["post_unpatch_callback"] = in.post_unpatch_callback;
    Node list;
    for (auto it = in.funcs.begin(); it != in.funcs.end(); it++) {
        Node func(*it);
        list.push_back(std::move(func));
    }
    node["function"] = list;
    return node;
  }

  static bool decode(const Node& node, hp_patch& in) {
    if(!node.IsMap() || node.size() < 3) {
      return false;
    }
    if (node["name"]) {
      in.name = node["name"].as<std::string>();
    } else {
      return false;
    }

    if (node["patch"]) {
      in.patch = node["patch"].as<std::string>();
    } else {
      return false;
    }

    if (node["target"]) {
      in.target = node["target"].as<std::string>();
    } else {
      in.target = "";
    }

    if (node["pre_patch_callback"]) {
      in.pre_patch_callback = node["pre_patch_callback"].as<bool>();
    }

    if (node["pre_patch_callback"]) {
      in.post_patch_callback = node["post_patch_callback"].as<bool>();
    }

    if (node["pre_unpatch_callback"]) {
      in.pre_patch_callback = node["pre_unpatch_callback"].as<bool>();
    }

    if (node["post_unpatch_callback"]) {
      in.pre_patch_callback = node["post_unpatch_callback"].as<bool>();
    }
    if (node["function"] && node["function"].IsSequence()) {
      for (auto it = node["function"].begin(); it != node["function"].end(); it++) {
        if (!it->IsMap()) {
            return false;
        }
        hp_func func;
        in.funcs.push_back(std::move(it->as<hp_func>()));
      }
    }
    return true;
  }
};

struct hp_cfg { 
    hp_head head;
    std::vector<hp_patch> patch;
};

template<>
struct convert<hp_cfg> {
  static Node encode(const hp_cfg& in) {
    Node root(in.head);
    Node list;
    for (auto it = in.patch.begin(); it != in.patch.end(); it++) {
        Node func(*it);
        list.push_back(std::move(func));
    }
    root["spec"] = std::move(list);
    return root;
  }

  static bool decode(const Node& node, hp_cfg& in) {
    if(!node.IsMap() || node.size() < 3) {
      return false;
    }
    in.head = std::move(node.as<hp_head>());
    if (node["spec"] && node["spec"].IsSequence()) {
      for (auto it = node["spec"].begin(); it != node["spec"].end(); it++) {
        if (!it->IsMap()) {
            return false;
        }
        hp_patch p = it->as<hp_patch>();

        in.patch.push_back(p);
      }
    }
    return true;
  }
};

}
