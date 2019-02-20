#include <cassert>
#include <iostream>

#include "punycode.h"

int main() {
  {
    try {
      auto decode =
          puny_code::url_decode<wchar_t, char>("http://xn--111123123-test.com");
      assert(false);
    } catch (const std::range_error& error) {
      std::cout << error.what() << std::endl;
    }
  }

  {
    std::wstring input = L"http://xn--pple-43d.com";
    auto encode_url = puny_code::url_encode<wchar_t>(input);
    assert(encode_url == "http://xn--pple-43d.com");
    auto decode_url = puny_code::url_decode<wchar_t, char>(encode_url);
    assert(decode_url == L"http://аpple.com");

    try {
      auto decode = puny_code::url_decode<wchar_t, char>("http://аpple.com");
      assert(false);
    } catch (const std::range_error& error) {
      std::cout << error.what() << std::endl;
    }
  }

  {
    std::wstring input = L"https://www.你好.com";
    auto encode_url = puny_code::url_encode<wchar_t>(input);
    assert(encode_url == "https://www.xn--6qq79v.com");
    auto decode_url = puny_code::url_decode<wchar_t, char>(encode_url);
    assert(decode_url == L"https://www.你好.com");

    try {
      auto decode =
          puny_code::url_decode<wchar_t, char>("https://www.你好.com");
      assert(false);
    } catch (const std::range_error& error) {
      std::cout << error.what() << std::endl;
    }
  }

  {
    std::wstring input = L"https://𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊";
    auto encode_url = puny_code::url_encode<wchar_t>(input);
    assert(encode_url == "https://xn--ub9baaaaaaaaaa842joapqrstuvwx");
    auto decode_url = puny_code::url_decode<wchar_t, char>(encode_url);
    assert(decode_url == input);
  }
}
