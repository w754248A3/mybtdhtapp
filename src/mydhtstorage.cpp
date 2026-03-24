#include "mydhtstorage.h"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <libtorrent/bencode.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/kademlia/item.hpp>
#include <libtorrent/settings_pack.hpp>

namespace {

using clock_type = std::chrono::steady_clock;

struct Sha1HashHasher {
    std::size_t operator()(lt::sha1_hash const& h) const noexcept
    {
        std::size_t seed = 0;
        for (char const c : h)
        {
            seed = (seed * 131) ^ static_cast<unsigned char>(c);
        }
        return seed;
    }
};

struct PeerRecord {
    lt::tcp::endpoint endpoint;
    bool seed = false;
    std::string name;
    clock_type::time_point seen_at = clock_type::now();
};

struct MutableItemRecord {
    std::vector<char> value;
    lt::dht::signature signature;
    lt::dht::sequence_number sequence = 0;
    lt::dht::public_key public_key;
    std::vector<char> salt;
    clock_type::time_point seen_at = clock_type::now();
};

constexpr std::size_t kMaxTorrentNameLength = 50;
constexpr auto kPeerTtl = std::chrono::minutes(30);
constexpr auto kItemTtl = std::chrono::hours(2);

class MemoryDhtStorage final : public lt::dht::dht_storage_interface {
public:
    explicit MemoryDhtStorage(lt::settings_interface const& settings)
        : m_max_peers_reply(settings.get_int(lt::settings_pack::dht_max_peers_reply))
        , m_max_torrents(settings.get_int(lt::settings_pack::dht_max_torrents))
        , m_max_items(settings.get_int(lt::settings_pack::dht_max_dht_items))
    {
    }

    void update_node_ids(std::vector<lt::node_id> const& ids) override
    {
        m_node_ids = ids;
    }

    bool get_peers(lt::sha1_hash const& info_hash, bool noseed, bool scrape,
        lt::address const& requester, lt::entry& peers) const override
    {
        (void)requester;
        auto const found = m_peers.find(info_hash);
        if (found == m_peers.end()) return false;

        if (scrape)
        {
            // 教学实现：仅返回占位 bloom filter。
            peers["BFpe"] = std::string(256 / 8, '\0');
            peers["BFsd"] = std::string(256 / 8, '\0');
            return false;
        }

        lt::entry::list_type values;
        int added = 0;
        for (PeerRecord const& p : found->second)
        {
            if (noseed && p.seed) continue;
            values.emplace_back(lt::entry(p.endpoint));
            ++added;
            if (added >= m_max_peers_reply) break;
        }

        peers["values"] = values;
        return static_cast<int>(found->second.size()) >= m_max_peers_reply;
    }

    void announce_peer(lt::sha1_hash const& info_hash, lt::tcp::endpoint const& endp,
        lt::string_view name, bool seed) override
    {
        auto& list = m_peers[info_hash];

        auto existing = std::find_if(list.begin(), list.end(), [&](PeerRecord const& p) {
            return p.endpoint == endp;
        });

        std::string name_copy(name.begin(), name.end());
        if (name_copy.size() > kMaxTorrentNameLength)
            name_copy.resize(kMaxTorrentNameLength);

        if (existing != list.end())
        {
            existing->seed = seed;
            existing->name = name_copy;
            existing->seen_at = clock_type::now();
            return;
        }

        if (m_peers.size() > static_cast<std::size_t>(m_max_torrents))
        {
            m_peers.erase(m_peers.begin());
        }

        list.push_back(PeerRecord{endp, seed, name_copy, clock_type::now()});
    }

    bool get_immutable_item(lt::sha1_hash const& target, lt::entry& item) const override
    {
        auto const found = m_immutable_items.find(target);
        if (found == m_immutable_items.end()) return false;

        lt::bdecode_node decoded;
        lt::error_code ec;
        lt::bdecode(found->second, decoded, ec);
        if (ec) return false;

        item["v"] = decoded;
        return true;
    }

    void put_immutable_item(lt::sha1_hash const& target, lt::span<char const> buf,
        lt::address const& addr) override
    {
        (void)addr;
        if (m_immutable_items.find(target) != m_immutable_items.end()) return;

        if (m_immutable_items.size() >= static_cast<std::size_t>(m_max_items))
        {
            m_immutable_items.erase(m_immutable_items.begin());
        }

        m_immutable_items.emplace(target, std::vector<char>(buf.begin(), buf.end()));
        m_immutable_seen[target] = clock_type::now();
    }

    bool get_mutable_item_seq(lt::sha1_hash const& target,
        lt::dht::sequence_number& seq) const override
    {
        auto const found = m_mutable_items.find(target);
        if (found == m_mutable_items.end()) return false;

        seq = found->second.sequence;
        return true;
    }

    bool get_mutable_item(lt::sha1_hash const& target, lt::dht::sequence_number seq,
        bool force_fill, lt::entry& item) const override
    {
        auto const found = m_mutable_items.find(target);
        if (found == m_mutable_items.end()) return false;

        MutableItemRecord const& data = found->second;
        item["seq"] = static_cast<std::int64_t>(data.sequence);

        if (force_fill || seq < data.sequence)
        {
            lt::bdecode_node decoded;
            lt::error_code ec;
            lt::bdecode(data.value, decoded, ec);
            if (!ec) item["v"] = decoded;

            item["sig"] = std::string(data.signature.bytes.begin(), data.signature.bytes.end());
            item["k"] = std::string(data.public_key.bytes.begin(), data.public_key.bytes.end());
        }

        return true;
    }

    void put_mutable_item(lt::sha1_hash const& target, lt::span<char const> buf,
        lt::dht::signature const& sig, lt::dht::sequence_number seq,
        lt::dht::public_key const& pk, lt::span<char const> salt,
        lt::address const& addr) override
    {
        (void)addr;

        auto const found = m_mutable_items.find(target);
        if (found != m_mutable_items.end() && seq <= found->second.sequence)
        {
            return;
        }

        if (m_mutable_items.size() >= static_cast<std::size_t>(m_max_items)
            && found == m_mutable_items.end())
        {
            m_mutable_items.erase(m_mutable_items.begin());
        }

        MutableItemRecord rec;
        rec.value.assign(buf.begin(), buf.end());
        rec.signature = sig;
        rec.sequence = seq;
        rec.public_key = pk;
        rec.salt.assign(salt.begin(), salt.end());
        rec.seen_at = clock_type::now();

        m_mutable_items[target] = std::move(rec);
    }

    int get_infohashes_sample(lt::entry& item) override
    {
        // 教学实现：固定采样前 N 个 infohash。
        std::string samples;
        int count = 0;
        for (auto const& [hash, peer_list] : m_peers)
        {
            (void)peer_list;
            if (count >= 20) break;
            samples.append(hash.data(), hash.size());
            ++count;
        }

        item["samples"] = samples;
        item["interval"] = 300;
        item["num"] = static_cast<std::int64_t>(m_peers.size());
        return count;
    }

    void tick() override
    {
        auto const now = clock_type::now();

        for (auto it = m_peers.begin(); it != m_peers.end();)
        {
            auto& list = it->second;
            list.erase(std::remove_if(list.begin(), list.end(), [&](PeerRecord const& p) {
                return now - p.seen_at > kPeerTtl;
            }), list.end());

            if (list.empty()) it = m_peers.erase(it);
            else ++it;
        }

        for (auto it = m_immutable_seen.begin(); it != m_immutable_seen.end();)
        {
            if (now - it->second > kItemTtl)
            {
                m_immutable_items.erase(it->first);
                it = m_immutable_seen.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = m_mutable_items.begin(); it != m_mutable_items.end();)
        {
            if (now - it->second.seen_at > kItemTtl)
                it = m_mutable_items.erase(it);
            else
                ++it;
        }
    }

    lt::dht::dht_storage_counters counters() const override
    {
        lt::dht::dht_storage_counters c;
        c.torrents = static_cast<std::int32_t>(m_peers.size());

        std::int32_t total_peers = 0;
        for (auto const& [hash, list] : m_peers)
        {
            (void)hash;
            total_peers += static_cast<std::int32_t>(list.size());
        }
        c.peers = total_peers;
        c.immutable_data = static_cast<std::int32_t>(m_immutable_items.size());
        c.mutable_data = static_cast<std::int32_t>(m_mutable_items.size());
        return c;
    }

private:
    int m_max_peers_reply = 20;
    int m_max_torrents = 2000;
    int m_max_items = 500;

    std::vector<lt::node_id> m_node_ids;

    std::unordered_map<lt::sha1_hash, std::vector<PeerRecord>, Sha1HashHasher> m_peers;
    std::unordered_map<lt::sha1_hash, std::vector<char>, Sha1HashHasher> m_immutable_items;
    std::unordered_map<lt::sha1_hash, clock_type::time_point, Sha1HashHasher> m_immutable_seen;
    std::unordered_map<lt::sha1_hash, MutableItemRecord, Sha1HashHasher> m_mutable_items;
};

} // namespace

namespace MyDhtStorage {

std::unique_ptr<lt::dht::dht_storage_interface> CreateMemoryDhtStorage(
    lt::settings_interface const& settings)
{
    return std::make_unique<MemoryDhtStorage>(settings);
}

} // namespace MyDhtStorage
