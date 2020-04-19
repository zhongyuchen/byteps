// Copyright 2019 Amazon Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#ifndef BYTEPS_COMPRESS_BASE_COMPRESSOR_H
#define BYTEPS_COMPRESS_BASE_COMPRESSOR_H

#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "../cpu_reducer.h"

namespace byteps {
namespace common {
namespace compressor {

/*!
 * \brief Byte buffer
 */
struct ByteBuf {
  char* data;
  size_t size;
};

/*!
 *  \brief Compressor interface used in BytePS core.
 */
class BaseCompressor {
 public:
  BaseCompressor();
  virtual ~BaseCompressor();

  /*!
   * \brief Allocate encoding buffer for compression.
   * \param aligned_size aligned size
   */
  virtual void Init(size_t aligned_size, int device=0);

  /*!
   * \brief Compress function
   *
   * \param grad gradient tensor
   * \param dtype data type
   * \param compressed compressed tensor
   */
  virtual void Compress(ByteBuf grad, int dtype, ByteBuf& compressed) = 0;

  /*!
   * \brief Decompress function
   *
   * \param compressed compressed tensor
   * \param dtype data type
   * \param decompressed decompressed tensor
   */
  virtual void Decompress(ByteBuf compressed, int dtype,
                          ByteBuf& decompressed) = 0;

 public:
#ifdef BYTEPS_ENABLE_CUDA
  char* get_dev_buf() { return _dev_buf; }
  cudaStream_t* get_stream() { return _stream; }
#endif
  std::unique_ptr<CpuReducer>& get_reducer() { return _cpu_reducer; };

 protected:
  /*!
   * \brief buffer
   */
  std::unique_ptr<char[]> _buf;

  /*!
   * \brief CPU reducer
   */
  std::unique_ptr<CpuReducer> _cpu_reducer;

#ifdef BYTEPS_ENABLE_CUDA
  char* _dev_buf;
  cudaStream_t* _stream;
  int _device;
#endif
};

using kwargs_t = std::unordered_map<std::string, std::string>;

class CompressorRegistry {
 public:
  using ctor_t =
      std::function<std::unique_ptr<BaseCompressor>(const kwargs_t& kwargs)>;

  using map_t = std::unordered_map<std::string, ctor_t>;

  struct Register {
    explicit Register(std::string name, ctor_t ctor);
  };

  static ctor_t Find(const std::string& name);

  static std::unique_ptr<BaseCompressor> Create(const kwargs_t& kwargs);

 private:
  static map_t _ctor_map;

  CompressorRegistry() = delete;
  ~CompressorRegistry() = delete;
};

inline std::string Serialize(const kwargs_t& kwargs) {
  std::ostringstream os;
  os << kwargs.size();
  for (auto const& kwarg : kwargs) {
    os << " " << kwarg.first << " " << kwarg.second;
  }
  return os.str();
}

inline kwargs_t Deserialize(const std::string& content) {
  kwargs_t kwargs;
  std::istringstream is(content);
  size_t size = 0;
  is >> size;
  for (size_t i = 0; i < size; ++i) {
    kwargs_t::key_type key;
    kwargs_t::mapped_type val;
    is >> key >> val;
    kwargs[key] = val;
  }

  return kwargs;
}
}  // namespace compressor
}  // namespace common
}  // namespace byteps

#endif  // BYTEPS_COMPRESS_BASE_COMPRESSOR_H