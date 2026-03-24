#pragma once

#include <memory>

#include <libtorrent/kademlia/dht_storage.hpp>

namespace MyDhtStorage {

std::unique_ptr<lt::dht::dht_storage_interface> CreateMemoryDhtStorage(
    lt::settings_interface const& settings);

} // namespace MyDhtStorage
