#pragma once
#include <avk/avk.hpp>

namespace avk {
/**
 * Read back data from a buffer.
 *
 * This is a convenience overload to avk::read.
 *
 * Example usage:
 * uint32_t readData = avk::read<uint32_t>(mMySsbo, avk::sync::not_required());
 * // ^ given that mMySsbo is a host-coherent buffer. If it is not, sync is required.
 *
 * @tparam	Ret			Specify the type of data that shall be read from the buffer (this is `uint32_t` in the example above).
 * @returns				A value of type `Ret` which is returned by value.
 */
template<typename Ret>
Ret buffer_t::read(size_t aMetaDataIndex, sync aSyncHandler) {
  auto memProps = memory_properties();
  if (!avk::has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)) {
    throw avk::runtime_error(
      "This ak::read overload can only be used with host-visible buffers. Use ak::void read(const ak::buffer_t<Meta>& _Source, void* _Data, sync aSyncHandler) instead!");
  }
  Ret result;
  read(static_cast<void *>(&result), aMetaDataIndex, std::move(aSyncHandler));
  return result;
}
}
