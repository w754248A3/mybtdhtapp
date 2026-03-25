#pragma once

#include <memory>

#include <libtorrent/kademlia/dht_storage.hpp>
namespace MyDhtStorage {

// 1.2/2.x 都位于 lt::dht 命名空间。
using dht_storage_interface_t = lt::dht::dht_storage_interface;

std::unique_ptr<dht_storage_interface_t> CreateMemoryDhtStorage(
    lt::settings_interface const& settings);

} // namespace MyDhtStorage
