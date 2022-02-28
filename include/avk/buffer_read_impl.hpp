#pragma once
#include <avk/avk.hpp>

namespace avk
{
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
  [[nodiscard]] Ret buffer_t::read(size_t aMetaDataIndex, old_sync aSyncHandler) {
    auto memProps = memory_properties();
    Ret result;
    read(static_cast<void *>(&result), aMetaDataIndex, std::move(aSyncHandler));
    return result;
  }
}
