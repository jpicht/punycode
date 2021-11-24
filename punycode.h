/**
 * Copyright (C) 2019 by hugh13245
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 
 * Copyright (C) 2011 by Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// punycode parameters, see http://tools.ietf.org/html/rfc3492#section-5
#ifndef _PUNY_CODE_H_
#define _PUNY_CODE_H_

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#define PUNY_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PUNY_ERROR(x) throw std::range_error(x);
#define PUNY_HTTP  { 'h', 't', 't', 'p', ':', '/', '/', 0 }
#define PUNY_HTTPS { 'h', 't', 't', 'p', 's', ':', '/', '/', 0 }
#define PUNY_PREFIX  { 'x', 'n', '-', '-', 0 }
namespace puny_code {
namespace constants {
constexpr int const BASE = 36;
constexpr int const TMIN = 1;
constexpr int const TMAX = 26;
constexpr int const SKEW = 38;
constexpr int const DAMP = 700;
constexpr int const INITIAL_N = 128;
constexpr int const INITIAL_BIAS = 72;
}  // namespace constants

template <class _Ty>
const _Ty *Prefix() {
  static const _Ty prefix[] = PUNY_PREFIX;
  return prefix;
}

template <class _Ty>
const _Ty *HttpProcotol() {
  static const _Ty http[] = PUNY_HTTP;
  return http;
}

template <class _Ty>
const _Ty *HttpsProcotol() {
  static const _Ty https[] = PUNY_HTTPS;
  return https;
}

template <class _Ty>
_Ty adapt_bias(_Ty delta, unsigned n_points, int is_first) {
  delta /= is_first ? constants::DAMP : 2;
  delta += delta / n_points;
  _Ty k = 0;
  // while delta > 455: delta /= 35
  for (; delta > ((constants::BASE - constants::TMIN) * constants::TMAX) / 2;
       k += constants::BASE) {
    delta /= (constants::BASE - constants::TMIN);
  }

  return k + (((constants::BASE - constants::TMIN + 1) * delta) /
              (delta + constants::SKEW));
}

template <class _Ty>
_Ty encode_digit(int c) {
  if (!(c >= 0 && c <= constants::BASE - constants::TMIN))
    PUNY_ERROR("Overflow");
  if (c > 25) {
    return c + 22;  // '0'..'9'
  } else {
    return c + 'a';  // 'a'..'z'
  }
}

// Encode as a generalized variable-length integer. Returns number of bytes
// written.
template <class _Ty>
void encode_var(const size_t bias, const size_t delta,
                           std::basic_string<_Ty> &dst) {
  size_t k = constants::BASE;
  size_t q = delta;
  auto dst_add_char = [&dst](_Ty ch) { dst.push_back(ch); };
  do {
    size_t t = 0;
    if (k <= bias) {
      t = constants::TMIN;
    } else if (k >= bias + constants::TMAX) {
      t = constants::TMAX;
    } else {
      t = k - bias;
    }

    if (q < t) {
      break;
    }

    dst_add_char(encode_digit<_Ty>(t + (q - t) % (constants::BASE - t)));

    q = (q - t) / (constants::BASE - t);
    k += constants::BASE;
  } while (true);

  dst_add_char(encode_digit<_Ty>(q));
}

template <class _Ty>
size_t decode_digit(_Ty v) {
  if (isdigit(v)) {
    return 26 + (v - '0');
  }
  if (islower(v)) {
    return v - 'a';
  }
  if (isupper(v)) {
    return v - 'A';
  }
  return SIZE_MAX;
}

template <typename _Ty>
std::vector<std::basic_string<_Ty>> split(
    const std::basic_string_view<_Ty> &input, _Ty dem) {
  std::vector<size_t> idxs;
  size_t length = input.size();
  for (size_t i = 0; i < length; i++) {
    if (input[i] == dem) idxs.push_back(i);
  }
  idxs.push_back(length);
  std::vector<std::basic_string<_Ty>> result;
  size_t pre_idx = 0;
  for (size_t i = 0; i < idxs.size(); i++) {
    std::basic_string<_Ty> temp(input.begin() + pre_idx,
                                input.begin() + idxs[i]);

    result.push_back(temp);
    pre_idx = idxs[i] + 1;
  }

  return result;
}

// convert unicode string to punycode string
template <class _Ty, class _Tt>
std::basic_string<_Tt> encode(const std::basic_string_view<_Ty> &src) {
  auto srclen = src.length();
  std::basic_string<_Tt> dst;
  size_t di = 00, si = 0;
  auto dst_add_char = [&di, &dst](_Tt ch) {
    dst.push_back(ch);
    di++;
  };
  for (; si < srclen; si++) {
    auto ch = src[si];
    if (ch < 128) {
      dst_add_char(ch);
    }
  }

  size_t b = di, h = di;

  // Write out delimiter if any basic code points were processed.
  if (di > 0 && si != di) {
    dst_add_char('-');
  }

  size_t n = constants::INITIAL_N;
  size_t bias = constants::INITIAL_BIAS;
  size_t delta = 0;
  size_t m = 0;
  for (; h < srclen; n++, delta++) {
    // Find next smallest non-basic code point. */
    for (m = SIZE_MAX, si = 0; si < srclen; si++) {
      if (src[si] >= n && src[si] < m) {
        m = src[si];
      }
    }

    if ((m - n) > (SIZE_MAX - delta) / (h + 1)) {
      PUNY_ERROR("Overflow");
    }

    delta += (m - n) * (h + 1);
    n = m;

    for (si = 0; si < srclen; si++) {
      if (src[si] < n) {
        if (++delta == 0) {
          PUNY_ERROR("Overflow");
        }
      } else if (src[si] == n) {
        encode_var<_Tt>(bias, delta, dst);
        bias = adapt_bias(delta, h + 1, h == b);
        delta = 0;
        h++;
      }
    }
  }
  return dst;
}

// convert punycode string to unicode string
template <class _Ty, class _Tt>
const std::basic_string<_Tt> decode(const std::basic_string_view<_Ty> &src) {
  auto srclen = src.length();
  // Ensure that the input contains only ASCII characters.
  for (auto ch : src) {
    if (ch & 0x80) {
      PUNY_ERROR("The input string contains non-ASCII characters");
    }
  }

  size_t b = src.find('-');
  b = (b == -1) ? 0 : b;
  size_t di = b;
  // Copy basic code points to output.
  std::basic_string<_Tt> dst;
  for (auto i = 0; i < di ; i++) {
    dst.push_back(src[i]);
  }

  size_t idx = 0;
  _Tt n = constants::INITIAL_N;
  auto bias = constants::INITIAL_BIAS;

  for (auto si = b + (b > 0); si < srclen; di++) {
    auto org_i = idx;

    for (size_t w = 1, k = constants::BASE; ; k += constants::BASE) {
      if (si >= srclen) {
        PUNY_ERROR("Overflow");
      }
      auto digit = decode_digit(src[si++]);

      if (digit == SIZE_MAX) {
        PUNY_ERROR("Overflow");
      }

      if (digit > (SIZE_MAX - idx) / w) {
        PUNY_ERROR("Overflow");
      }

      idx += digit * w;
      size_t t = 0;
      if (k <= bias) {
        t = constants::TMIN;
      } else if (k >= bias + constants::TMAX) {
        t = constants::TMAX;
      } else {
        t = k - bias;
      }

      if (digit < t) {
        break;
      }

      if (w > SIZE_MAX / (constants::BASE - t)) {
        PUNY_ERROR("Overflow");
      }

      w *= constants::BASE - t;
    }

    bias = adapt_bias(idx - org_i, di + 1, org_i == 0);

    if (idx / (di + 1) > SIZE_MAX - n) {
      PUNY_ERROR("Overflow");
    }

    n += idx / (di + 1);
    idx %= (di + 1);
    dst.insert(dst.begin() + (idx++), n);
  }

  return dst;
}

template <class _Ty, class _Tt>
using ProtocolPair =
    std::pair<std::basic_string_view<_Ty>, std::basic_string<_Tt>>;

template <class _Ty, class _Tt>
ProtocolPair<_Ty, _Tt> parse_url_prefix(
    const std::basic_string_view<_Ty> &input) {
  if (input.find(HttpsProcotol<_Ty>()) == 0) {
    return std::make_pair<std::basic_string_view<_Ty>, std::basic_string<_Tt>>(
        input.substr(8), HttpsProcotol<_Tt>());
  }
  if (input.find(HttpProcotol<_Ty>()) == 0) {
    return std::make_pair<std::basic_string_view<_Ty>, std::basic_string<_Tt>>(
        input.substr(7), HttpProcotol<_Tt>());
  }
  return std::make_pair<std::basic_string_view<_Ty>, std::basic_string<_Tt>>(
      input.substr(0), std::basic_string<_Tt>());
}

template <class _Ty, class _Tt, class _Func>
std::basic_string<_Tt> convert(const std::basic_string_view<_Ty> &input,
                               _Func &&convert_part) {
  std::basic_string_view<_Ty> unicode = input;
  auto protocol = parse_url_prefix<_Ty, _Tt>(input);
  auto parts = split<_Ty>(protocol.first, '.');
  bool is_first = true;
  std::basic_stringstream<_Tt> ss;
  for (auto const &part : parts) {
    if (!is_first) {
      ss << '.';
    } else {
      is_first = false;
    }
    if (!part.empty()) {
      ss << convert_part(part);
    }
  }
  return protocol.second + ss.str();
}

// convert unicode url to punycode url
template <class _Ty>
std::string url_encode(const std::basic_string_view<_Ty> &input) {
  return convert<_Ty, char>(
      input, [](const std::basic_string_view<_Ty> &part) {
        std::string output = encode<_Ty, char>(part);
        return part.size() == output.size() ? output : Prefix<char>() + output;
      });
}

// convert punycode url to unicode url
template <class _Ty, class _Tf = char>
std::basic_string<_Ty> url_decode(const std::basic_string_view<_Tf> &input) {
  return convert<_Tf, _Ty>(input, [](const std::basic_string_view<_Tf> &part) {
    std::basic_string<_Ty> output;
    for (auto ch : part) {
      if (ch & 0x80) {
        PUNY_ERROR("The input string contains non-ASCII characters");
      }
    }
    if (part.substr(0, 4) == Prefix<_Tf>())
      output = decode<_Tf, _Ty>(part.substr(4, part.length() - 4));
    else
      for (auto ch : part) output.push_back(ch);
    return output;
  });
}
}  // namespace puny_code
#endif
