# libasn1c — asn1c 骨架 / runtime

本目录由 `../run_demo.sh` 从 `../../install/share/asn1c/` **同步生成**，
与 `asn_out/`（协议代码）分开存放。

对应 RSU 工程里的 `libasn1c/`。

## 怎么出现这些文件？

```bash
cd ..          # 到 asn1c-master
./build.sh     # 安装 asn1c（若未装）
cd demo
./run_demo.sh  # 会生成 asn_out/ 并同步本目录 *.c/*.h
```

同步后这里应有约 60+ 个头文件，例如：

- `asn_application.h`
- `ber_decoder.h` / `der_encoder.h`
- `INTEGER.h` / `OCTET_STRING.h`
- `constr_CHOICE.h` / `constr_SEQUENCE.h`

## 注意

- `asn_out/` 由脚本每次重建，默认不进 git；`libasn1c/` 可在资源管理器中直接查看。
- 业务编译时：`-Iasn_out -Ilibasn1c`，并链接本目录源码或做成 `libasn1c.so`。
