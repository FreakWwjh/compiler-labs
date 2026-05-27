# 编译原理实验套件

本仓库收录编译原理课程系列实验代码，在 **openEuler aarch64 (鲲鹏)** 环境下开发测试。

## 📁 实验列表

| 实验 | 内容 | 路径 |
|------|------|------|
| 实验一 | DFA 模拟器（确定有限自动机） | [`exp1/`](./exp1) |

## 🛠️ 环境信息

- **OS**: openEuler 22.03 LTS (aarch64)
- **Compiler**: g++ 10.3.1 (支持 C++17)
- **构建工具**: GNU Make

## 🚀 快速开始

```bash
# 克隆仓库
git clone https://github.com/<你的用户名>/compiler-labs.git
cd compiler-labs

# 编译实验一
cd exp1
make clean && make
./dfa_sim dfa_in1.dfa
```

## 📄 许可证

本项目采用 [MIT License](exp1/LICENSE) 开源。

## ✨ 后续计划

- [ ] 实验二：词法分析器（基于实验一 DFA）
- [ ] 实验三：语法分析（递归下降 / LL(1)）
- [ ] 实验四：中间代码生成
