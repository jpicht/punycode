# Examples

## Encode

```c++
std::wstring input = L"http://аpple.com";
auto encode_url = puny_code::url_encode<wchar_t>(input);
assert(encode_url == "http://xn--pple-43d.com");
```

### Decode

```c++
std::string input = "http://xn--pple-43d.com";
auto decode_url = puny_code::url_decode<wchar_t>(input);
assert(decode_url == L"http://аpple.com");
```

