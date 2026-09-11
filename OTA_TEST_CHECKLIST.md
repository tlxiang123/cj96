# CJ96 TuyaLink OTA 测试流程

## 当前产物

本次使用当前工程重新生成了完整镜像：

- 云端上传文件：`Release/tuya_ota/update.bin`
- 原始镜像：`Release/internal_update_20260907_131128/update.img`
- 大小：`5456444` 字节
- MD5：`a8f7ee4da2a1c760b281c4630798e506`
- SHA-256：`1fb73d05cb47c3adddff6182902e98273b4f3eb42463fe5aad942b4db6eadf30`

`update.bin` 是 `update.img` 的字节完全一致副本，不是重新编码或压缩。每次生成云端文件都使用：

```text
python tools/prepare_tuya_ota_bin.py --source Release/internal_update_YYYYMMDD_HHMMSS/update.img
```

工具会检查 CJ96 镜像头 `ZKSWEV1.0`，并比对源文件和 `.bin` 的大小、MD5、SHA-256。

## 云端操作

1. 在 TuyaLink 产品的 OTA 固件页面新增固件。
2. 固件类型按涂鸦回复选择当前产品对应的模组固件类型；升级通道保持 `0`。
3. 上传 `Release/tuya_ota/update.bin`，不要上传 `update.img`。
4. 填写一个高于设备当前上报版本的版本号。当前桥接程序未从配置读到版本号时默认上报 `1.0.56`，因此测试版本不能仍填 `1.0.56`。
5. 创建并发布 OTA 任务，目标设备选择当前测试板。

## 板端实际流程

桥接程序收到 TuyaLink OTA 消息后执行以下流程：

1. 检查升级通道是否为 `0`，检查版本、文件大小和下载地址。
2. 下载云端 `.bin` 到临时文件。
3. 校验文件大小、MD5、HMAC 和 `ZKSWEV1.0` 镜像头。
4. 将临时文件原子改名为 `/mnt/extsd/update.img`。
5. 设置 `sys.zkupgrade.dir=/mnt/extsd`、`sys.zkupgrade.flag=zkautoupgrade`、`sys.zkupgrade.force=1`。
6. 执行 `ctl.restart=zkswe`，由板端升级器读取 `update.img` 并重启。

因此板端不需要把 `.bin` 的内容再转换成 `.img`；只需要把下载后的文件名改为升级器要求的 `update.img`。云端 `.bin` 和板端 `.img` 的内容必须保持完全一致。

## 测试前检查

在发送 OTA 任务前确认：

- 板子在线，TuyaLink MQTT 已连接。
- `/mnt/extsd` 是实际可写的 SD 卡挂载点，不是根文件系统中的普通目录。
- SD 卡剩余空间大于本次镜像大小，并额外留出临时下载空间。
- 设备当前版本低于云端目标版本。
- 升级任务的产品、设备、固件类型和通道都对应当前设备。

本次最后检查到的板端状态是：`/mnt/extsd` 尚未显示为独立 SD 卡挂载点，且该目录所在文件系统剩余约 30 MiB。这个状态不满足 OTA 测试条件，必须先让板端正确挂载 SD 卡，否则下载可能在 0% 卡住或因空间不足失败。

## 结果判断

- 平台进度开始变化：说明 OTA 任务已下发并开始下载。
- 下载完成后桥接日志应出现校验成功、保存 `update.img`、触发 `zkswe` 重启。
- 重启后设备重新上线，并上报高于旧版本的固件版本，才算升级完成。
- 若仍为 0%：先看 MQTT 是否收到 OTA 消息，再看 `/mnt/extsd` 挂载和剩余空间，最后核对云端文件 MD5 是否为本文件的 MD5。

## 重新生成

镜像内容或桥接程序变化后，先重新生成镜像，再生成云端文件：

```text
python tools/build_internal_update_image.py
python tools/prepare_tuya_ota_bin.py
```

不要直接修改 `.bin`，也不要只改扩展名后跳过镜像头、大小和哈希校验。
