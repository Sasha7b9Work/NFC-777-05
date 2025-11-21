// 2023/08/29 19:55:25 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#pragma once
#include "Utils/Buffer.h"
#include "Utils/Mutex.h"
#include "Utils/StringUtils.h"
#include "Settings/Settings.h"


class BufferUSART : public Buffer512
{
public:
    BufferUSART() : Buffer512() { }

    // Нумерация начинается с 1
    void GetWord(int num_word, ParseBuffer &) const;

    // Слово nuw_word == word. Нумерация с 1
    bool WordIs(int num_word, pchar string) const;

    // Столько слов содержится в буфере. Слова разделены пробелами, т.е. "param=232" это одно слово
    int GetCountWords() const;

    // Возвращает значение целочисленного параметра
    bool GetParamIntDec(pchar name_param, int *value) const;
    bool GetParamUInt(pchar name_param, uint *value, int base) const;
    bool GetParamUInt64dec(pchar name_param, uint64 *value) const;
    bool GetParamUInt128hex(pchar name_param, Block16 *value) const;

    // Преобразует слово в значение. Нумерация начинается с 1
    bool GetIntDec(int num_word, int *value) const;
    bool GetUInt(int num_word, uint *value, int base) const;
    bool GetUInt64(int num_word, uint64 *value, int base) const;
    bool GetUInt128hex(int num_word, Block16 *value) const;

    // Читает настройки. Возвращате true, если удалось
    bool ReadSettings(SettingsMaster &) const;

    // Возвращает true, если принятая crc32 соотвествует рассчитанному
    bool Crc32IsMatches() const;
};


namespace Communicator
{
    // Эту фуркцию вызываем в главном цикле, чтобы обработать полученные байты
    void Update(BufferUSART &);

    // Засылает в USART конфирурацию в текстовом виде
    void WriteConfitToUSART();
}
