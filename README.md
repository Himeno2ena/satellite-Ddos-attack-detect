# 仿真卫星Ddos攻击代码文件说明
## 版本说明
选择ns-3.43和sns3模块
ns-3.43安装：参照官网 https://www.nsnam.org/releases/ns-3-43/
sns3安装： 参照github https://github.com/sns3/sns3-satellite
## 文件说明
attack-file.h                            攻击函数头文件
attack-file.cc                           攻击函数实现文件
sat-constellation-flowmonitor.cc         仿真环境和检测文件
## 使用方式
将attack-file.*置入/src/internet/model中，注意修改对应的cmakelists
将sat-constellation-flowmonitor.cc置入/contrib/satellite/examples中，同样注意修改对应的cmakelists
回到根目录，运行
```bash
./ns3 run sat-constellation-flowmonitor
```
