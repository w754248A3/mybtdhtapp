# 自定义 `dht_storage_interface` 实现说明

本文档说明本项目中的教学版内存 DHT 存储实现：`MemoryDhtStorage`（文件：`src/mydhtstorage.cpp`）。

## 1. 设计目标

- 使用 C++ 标准库容器（`std::unordered_map`、`std::vector`）实现。
- 数据只保存在进程内存，不落盘。
- 覆盖 `libtorrent::dht::dht_storage_interface` 的核心虚函数，便于学习 DHT 存储层与上层网络逻辑的接口边界。

## 2. 主要数据结构

`MemoryDhtStorage` 内部维护了四类数据：

1. **节点 ID 列表**
   - `std::vector<lt::node_id> m_node_ids`
   - 由 `update_node_ids()` 更新，当前实现仅保存，不参与淘汰策略。

2. **peer 存储（按 info-hash）**
   - `std::unordered_map<lt::sha1_hash, std::vector<PeerRecord>, Sha1HashHasher> m_peers`
   - `PeerRecord` 包含：`endpoint`、`seed`、`name`、`seen_at`。
   - 用于 `announce_peer()` 写入，`get_peers()` 读取。

3. **immutable item 存储**
   - `std::unordered_map<lt::sha1_hash, std::vector<char>, Sha1HashHasher> m_immutable_items`
   - `std::unordered_map<lt::sha1_hash, clock_type::time_point, Sha1HashHasher> m_immutable_seen`
   - 二进制值保存为 bencoded 原始字节，读取时解码到 `entry["v"]`。

4. **mutable item 存储**
   - `std::unordered_map<lt::sha1_hash, MutableItemRecord, Sha1HashHasher> m_mutable_items`
   - `MutableItemRecord` 包含：`value`、`signature`、`sequence`、`public_key`、`salt`、`seen_at`。

## 3. 每个函数的作用

### 3.1 生命周期与配置

- `MemoryDhtStorage(settings_interface const&)`
  - 从 `settings` 读取限制参数：
    - `dht_max_peers_reply`
    - `dht_max_torrents`
    - `dht_max_dht_items`
  - 这些限制用于控制返回数量和容量上限。

- `CreateMemoryDhtStorage(settings_interface const&)`
  - 工厂函数，返回 `std::make_unique<MemoryDhtStorage>`。
  - 在 `session_params.dht_storage_constructor` 中注册。

### 3.2 节点 ID 同步

- `update_node_ids(std::vector<lt::node_id> const&)`
  - 接收 libtorrent 当前 DHT 节点 ID 列表。
  - 本实现只缓存该列表，未做进一步策略使用。

### 3.3 peer 相关

- `announce_peer(info_hash, endpoint, name, seed)`
  - 新增或更新某个 info-hash 下的 peer。
  - 若 endpoint 已存在则更新 `seed/name/seen_at`。
  - 名称最长截断为 50 字符（教学实现）。

- `get_peers(info_hash, noseed, scrape, requester, peers)`
  - `scrape=true`：返回占位 bloom filter 字段（`BFpe/BFsd`）。
  - `scrape=false`：返回 `values` 列表，数量不超过 `dht_max_peers_reply`。
  - `noseed=true` 时过滤 seeder。

### 3.4 immutable item 相关

- `put_immutable_item(target, buf, addr)`
  - 如果不存在则写入；如果已存在则忽略（符合 immutable 语义）。
  - 达到容量上限时，简单删除一个旧元素（教学版简化策略）。

- `get_immutable_item(target, item)`
  - 找到则将值解码写入 `item["v"]`。

### 3.5 mutable item 相关

- `put_mutable_item(target, buf, sig, seq, pk, salt, addr)`
  - 若 key 已存在，仅当新 `seq` 更大才覆盖。
  - 保存签名、公钥、salt 及内容。

- `get_mutable_item_seq(target, seq)`
  - 仅返回当前存储的序列号。

- `get_mutable_item(target, seq, force_fill, item)`
  - 总是回填 `item["seq"]`。
  - 当 `force_fill=true` 或请求 `seq < stored_seq` 时，填充：
    - `item["v"]`
    - `item["sig"]`
    - `item["k"]`

### 3.6 采样与维护

- `get_infohashes_sample(item)`
  - 教学实现：从 `m_peers` 里取最多 20 个 info-hash，拼接到 `item["samples"]`。
  - 同时填充 `item["interval"]` 和 `item["num"]`。

- `tick()`
  - 周期性清理：
    - peer 超过 30 分钟未刷新则删除。
    - immutable/mutable item 超过 2 小时删除。

- `counters()`
  - 汇总当前存储中的 torrent/peer/immutable/mutable 数量，供上层统计。

## 4. 函数之间的关系（调用链）

1. 应用启动时在 `mydhtrecord.cpp` 中构造 `session_params` 并设置 `dht_storage_constructor`。
2. libtorrent 在需要 DHT 存储实例时调用构造器，创建 `MemoryDhtStorage`。
3. 网络层收到 DHT 报文后：
   - announce 请求进入 `announce_peer()` / `put_*()`
   - query 请求进入 `get_peers()` / `get_*()`
4. libtorrent 定期调用 `tick()` 做维护；统计时调用 `counters()`。

## 5. 教学版实现的简化点

- 未实现复杂淘汰算法（例如基于距离、热点、LRU 的组合策略）。
- scrape bloom filter 仅占位，未按完整 BEP 逻辑构建。
- 容量满时采用“删除任意一个元素”的简化处理。

这些简化有利于先理解接口和数据流，后续可逐步替换为更严格的生产级策略。
