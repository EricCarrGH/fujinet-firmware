#ifndef _FN_APPKEY_H
#define _FN_APPKEY_H

#include <cstdint>
#include <map>
#include <vector>

#define DEFAULT_KEY_SIZE 64
#define MAX_KEY_SIZE 256

enum appkey_mode : uint8_t
{
    APPKEYMODE_INVALID = 255,
    APPKEYMODE_READ = 0,
    APPKEYMODE_WRITE = 1
};

struct appkey_open_params
{
    uint16_t creator = 0;
    uint8_t app = 0;
    uint8_t key = 0;
    appkey_mode mode = APPKEYMODE_INVALID;
    uint8_t flags = 0;
} __attribute__((packed));

struct appkey_payload
{
  uint16_t size;
  uint8_t data[MAX_KEY_SIZE]; 
};

class fnAppkey
{
public:
  appkey_open_params* get_open_params_buffer();
  int open(appkey_open_params* open_parameters_buffer);
  int open(uint16_t creator, uint8_t app, uint8_t key, appkey_mode mode, uint8_t reserved);
  int open();
  
  appkey_payload* read();

  std::vector<uint8_t> get_write_buffer(uint16_t size);
  int write(std::vector<uint8_t>& payload);
  int write(uint8_t* data, uint16_t size);
  
  
  void close();
  bool isWriteMode();

private:
  char * _generate_appkey_filename();

  appkey_open_params _open_params;
  appkey_payload _payload;
  std::vector<uint8_t> _write_buffer = std::vector<uint8_t>(DEFAULT_KEY_SIZE, 0);

  // We can determine the best way to specify keysize in the future, possibly with reserved|flag 
  uint16_t _keysize = DEFAULT_KEY_SIZE;

  // std::map<int, int> _mode_to_keysize = {
  //     {APPKEYMODE_READ, DEFAULT_KEY_SIZE},
  //     {APPKEYMODE_READ_MAX, MAX_KEY_SIZE}
  // };
};

extern fnAppkey Appkey;

#endif // _FN_APPKEY_H