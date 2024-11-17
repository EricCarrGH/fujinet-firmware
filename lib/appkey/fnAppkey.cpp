#include "fnAppkey.h"
#include "fnFsSD.h"
#include "fnFS.h"
#include "utils.h"

#include "../../include/debug.h"

#include <string.h>


#define ERROR 1
#define SUCCESS 0

#define LOG_SOURCE "Appkey"

#define log(m) Debug_println(LOG_SOURCE ": " m)
#define logf(format, ...) Debug_printf(LOG_SOURCE ": " format "\n", ##__VA_ARGS__)

// There can be only one
fnAppkey Appkey;

/// @brief Returns a pointer to appkey open params object for direct manipulation
/// open() should be called aferward to open the appkey.
appkey_open_params* fnAppkey::get_open_params_buffer() 
{
  return &_open_params;
}

/// @brief Sets appkey open params and process an open action
/// @returns 0 on success or an error code
int fnAppkey::open(uint16_t creator, uint8_t app, uint8_t key, appkey_mode mode, uint8_t flags)
{
  _open_params.creator = creator;
  _open_params.app = app;
  _open_params.key = key;
  _open_params.mode = mode;
  _open_params.flags = flags;
  return open();
}

/// @brief Sets appkey open params and process an open action
/// @returns 0 on success or an error code
int fnAppkey::open(appkey_open_params* open_parameters_buffer)
{
  memcpy(&_open_params, open_parameters_buffer, sizeof(appkey_open_params));
  return open();
}

/// @brief Uses the current appkey open params to process an open action
/// @returns 0 on success or an error code
int fnAppkey::open()
{
  logf("OPEN - creator = 0x%04hx, app = 0x%02hhx, key = 0x%02hhx, mode = %hhu, flags = %hhu, filename = \"%s\"\n",
    _open_params.creator, _open_params.app, _open_params.key, _open_params.mode, _open_params.flags,
    _generate_appkey_filename());

  // We're only supporting writing to SD, so return an error if there's no SD mounted
  if (fnSDFAT.running() == false)
  {
      log("ERROR - No SD mounted");
      return ERROR;
  }

  // Basic check for valid data
  if (_open_params.creator == 0 || _open_params.mode == APPKEYMODE_INVALID)
  {
      log("ERROR - Invalid app key data");
      return ERROR; 
  }

  return SUCCESS;
}

/// @brief Reads the previously opened appkey
/// @returns Pointer to the appkey_payload for reading. The size will be 0 if the appkey was not found
appkey_payload* fnAppkey::read()
{
  log("READ");
 
  // Reset payload
  _payload.size = 0;
  memset(_payload.data, 0, sizeof(_payload.data));

  // Make sure we have an SD card mounted
  if (fnSDFAT.running() == false)
  {
      log("ERROR - No SD mounted");
      return &_payload;
  }

  // Make sure we have valid app key information
  if (_open_params.creator == 0 || _open_params.mode != APPKEYMODE_READ)
  {
      logf("ERROR - Creator is 0 or open mode is not read (%u)", APPKEYMODE_READ);
      return &_payload;
  }

  char *filename = _generate_appkey_filename();
  
  FILE *fIn = fnSDFAT.file_open(filename, FILE_READ);
  if (fIn == nullptr)
  {
      logf("ERROR - Failed to open input file. Error %d", errno);
      return &_payload;
  }

  _payload.size = fread(&_payload.data, 1, _keysize, fIn);
  fclose(fIn);

  logf("Read %u bytes", (unsigned)_payload.size);

  #ifdef DEBUG
  std::string msg = util_hexdump(_payload.data, _payload.size);
  logf("PAYLOAD\n%s\n", msg.c_str());
  #endif

  return &_payload;
}

/// @brief Returns a buffer that can be used to write out the appkey payload.
// write() should be called to write the buffer to the disk.
/// @param size the size of data the buffer will need to hold
std::vector<uint8_t> fnAppkey::get_write_buffer(uint16_t size)
{
  _write_buffer.resize(size);
  memset(_write_buffer.data(), 0, _write_buffer.size());
  return _write_buffer;
}

/// @brief Writes the specified payload to disk.
/// @returns 0 on success or an error code
int fnAppkey::write(std::vector<uint8_t>& payload)
{
  return write(payload.data(), payload.size());
}

/// @brief Writes the specified payload to disk.
/// @returns 0 on success or an error code
int fnAppkey::write(uint8_t* data, uint16_t size)
{
    log("WRITE");
    
    // Make sure we have an SD card mounted
    if (fnSDFAT.running() == false)
    {
      log("ERROR - No SD mounted");
      return ERROR;
    }

    // Make sure we have valid app key information
    if (_open_params.creator == 0 || _open_params.mode != APPKEYMODE_WRITE)
    {
        logf("ERROR - Creator is 0 or open mode is not write (%u)", APPKEYMODE_WRITE);
        return ERROR;
    }

    // Constrain size (alternatively, we COULD return an error here instead of storing the truncated payload)
    if (size > _keysize)
    {
      logf("WARNING - %u bytes attempted to be written, but truncated to current keysize of %u", size, _keysize);
      size = _keysize;
    }

    char *filename = _generate_appkey_filename();

    // Reset the open params data so we require calling APPKEY OPEN before another attempt
    _open_params.creator = 0;
    _open_params.mode = APPKEYMODE_INVALID;

    logf("Writing appkey to \"%s\"\n", filename);

    // Make sure we have a "/FujiNet" directory, since that's where we're putting these files
    fnSDFAT.create_path("/FujiNet");

    FILE *fOut = fnSDFAT.file_open(filename, FILE_WRITE);
    if (fOut == nullptr)
    {
        logf("Failed to open/create output file: errno=%d\n", errno);
        return ERROR;
    }
    size_t count = fwrite(data, 1, size, fOut);
    int e = errno;

    fclose(fOut);

    if (count != size)
    {
        logf("Only wrote %u bytes of expected %hu, errno=%d\n", (unsigned)count, size, e);
        return ERROR;
    }

    return SUCCESS;
}

/// @brief The app key close operation is a placeholder in case we want to provide more robust
/// file read/write operations. Currently, the file is closed immediately after the read or write operation.
/// @returns 0 on success or an error code
void fnAppkey::close()
{
    log("CLOSE");
    _open_params.creator = 0;
    _open_params.mode = APPKEYMODE_INVALID;
}

/// @brief Returns true if the current open params mode is writing
bool fnAppkey::isWriteMode()
{
  return _open_params.mode == 1;
}

char * fnAppkey::_generate_appkey_filename()
{
    static char filenamebuf[30];

    snprintf(filenamebuf, sizeof(filenamebuf), "/FujiNet/%04hx%02hhx%02hhx.key", _open_params.creator, _open_params.app, _open_params.key);
    return filenamebuf;
}
