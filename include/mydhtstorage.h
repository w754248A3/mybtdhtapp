#pragma once

#include <memory>

#include <libtorrent/kademlia/dht_storage.hpp>
#include <libtorrent/version.hpp>

namespace MyDhtStorage {

// libtorrent 1.2 使用 lt::dht_storage_interface，2.x 使用 lt::dht::dht_storage_interface。
#if LIBTORRENT_VERSION_NUM < 20000
using dht_storage_interface_t = lt::dht_storage_interface;
#else
using dht_storage_interface_t = lt::dht::dht_storage_interface;
#endif

std::unique_ptr<dht_storage_interface_t> CreateMemoryDhtStorage(
    lt::settings_interface const& settings);

} // namespace MyDhtStorage
