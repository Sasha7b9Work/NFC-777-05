#pragma once

/**< Из: https://github.com/DavyLandman/AESLib */

namespace AES
{
    enum Mode
    {
        ECB = 0,
        CBC
    };

    // Декодировать ответ
    void BeginDecResponse(const uint8 *key, uint8 *data, Mode mode, uint8 ti[4], uint16 r_counter, uint16 w_counter);
    void Dec(const uint8 *key, uint8 *data, Mode mode = ECB, uint8 *iv = nullptr);

    // Закодировать команду
    void BeginEncCommand(const uint8 *key, uint8 *data, Mode mode, uint8 ti[4], uint16 r_counter, uint16 w_counter);
    void Enc(const uint8 *key, uint8 *data, Mode mode = ECB, uint8 *iv = nullptr);
};

