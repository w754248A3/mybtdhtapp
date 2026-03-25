#include "mydhtstorage2.h"

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
using dht_sequence_number = lt::dht::sequence_number;
using dht_node_id = lt::dht::node_id;

struct Sha1HashHasher {
    std::size_t operator()(lt::sha1_hash const& h) const noexcept
    {
        std::size_t seed = 0;
        for (auto const c : h)
        {
            seed = (seed * 131) ^ static_cast<std::size_t>(c);
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

constexpr std::size_t kMaxTorrentNameLength = 50;
constexpr auto kPeerTtl = std::chrono::minutes(30);
constexpr auto kItemTtl = std::chrono::hours(2);

class MemoryDhtStorage : public MyDhtStorage2::dht_storage_interface_t {
public:
    explicit MemoryDhtStorage(lt::settings_interface const& settings)
        : m_max_peers_reply(settings.get_int(lt::settings_pack::dht_max_peers_reply))
        , m_max_torrents(settings.get_int(lt::settings_pack::dht_max_torrents))
        , m_max_items(settings.get_int(lt::settings_pack::dht_max_dht_items))
    {
    }

    void update_node_ids(std::vector<dht_node_id> const& ids) override
    {
        
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
            //peers["BFpe"] = std::string(256 / 8, '\0');
            //peers["BFsd"] = std::string(256 / 8, '\0');
            return false;
        }

        lt::entry::list_type values;
        int added = 0;
        for (PeerRecord const& p : found->second)
        {
            if (noseed && p.seed) continue;
            values.emplace_back(p.endpoint.address().to_string() + ":" + std::to_string(p.endpoint.port()));
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
        return false;
    }

    void put_immutable_item(lt::sha1_hash const& target, lt::span<char const> buf,
        lt::address const& addr) override
    {
        
    }

    bool get_mutable_item_seq(lt::sha1_hash const& target,
        dht_sequence_number& seq) const override
    {
        return false;
    }

    bool get_mutable_item(lt::sha1_hash const& target, dht_sequence_number seq,
        bool force_fill, lt::entry& item) const override
    {
        
        return false;
    }

    void put_mutable_item(lt::sha1_hash const& target, lt::span<char const> buf,
        lt::dht::signature const& sig, dht_sequence_number seq,
        lt::dht::public_key const& pk, lt::span<char const> salt,
        lt::address const& addr) override
    {
        
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
        c.immutable_data = 0;
        c.mutable_data = 0;
        return c;
    }

private:
    int m_max_peers_reply = 20;
    int m_max_torrents = 2000;
    int m_max_items = 500;

    std::unordered_map<lt::sha1_hash, std::vector<PeerRecord>, Sha1HashHasher> m_peers;
};

} // namespace

namespace MyDhtStorage2 {

std::unique_ptr<dht_storage_interface_t> CreateMemoryDhtStorage(
    lt::settings_interface const& settings)
{
    return std::make_unique<MemoryDhtStorage>(settings);
}

} // namespace MyDhtStorage
