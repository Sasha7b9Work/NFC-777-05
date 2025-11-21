// 2025/01/06 13:16:49 (c) Aleksandr Shevchenko e-mail : Sasha7b9@tut.by
#include "defines.h"
#include "Reader/RC663/Commands.h"
#include "Reader/RC663/Registers.h"
#include "Reader/RC663/RC663_def.h"
#include "Hardware/Timer.h"
#include "Reader/Cards/TypeCard.h"
#include "Reader/Cards/AES.h"
#include "Reader/RC663/RC663.h"
#include "Reader/Cards/Card.h"
#include "Reader/Cards/CardsEntities.h"
#include "Settings/SET2.h"
#include "Utils/Log.h"


namespace Card
{
    using RC663::Register;

    namespace _14443
    {
        namespace T_CL
        {
            struct RNDA
            {
                uint8 bytes[16];
                void Generate()
                {
                    std::srand(TIME_MS);
                    for (uint i = 0; i < 16; i++)
                    {
                        bytes[i] = (uint8)std::rand();
                    }
                }
            };

            static bool WritePerso(int nbr_block, const Block16 &);
            static bool CommitPerso();
            static bool SwitchToSL3FromSL0();
            static bool SwitchToSL3FromSL1();


            static Block4 ti{ 0x00000000 };     // Номер транзакции. Выдаётся после авторизации
            static RNDA rnda;                   // Здесь случайное число, новое для каждой FirstAutenticate
            static uint8 picc_cap[6];           // Возможности карты (ставится при инициализации карты (mfp_write_perso b000))
            static uint8 pcd_cap[6];            // Возможности передатчика
            static uint16 w_counter = 0;        // Счетчик выполненных команды записи
            static uint16 r_counter = 0;        // Счетчик выполненных команд чтения
            static Block16 rndb(0, 0);          // Случаыйное число от PICC
            static Block16 session_key(0, 0);   // Используется для шифрования данных
            static Block16 mac_key(0, 0);       // Используется для расчёта MAC (Message Authentication Code)

            // http://www.gorferay.com/aes-128-key-diversification-example/
            // Генерирует сессионный ключ (используется для шифрования данных) на основне RNDA RNDB из аутентификации
            // Generation of Session Key for Encryption
            // Session Key for Encryption KENC
            // Расчет сессионного ключа -> [0] = 0x11; [1..5] = rnda[7..11] ^ rndb[7..11]; [6..10] = rndb[0..4]; [11..15] = rnda[0..4];
            // session = aes128(k,session);
            // @param Ключ для блока ( к котрому производилась аутентификация)
            static void GenerateSessionKey(const uint8 *key, uint8[16]);

            // Генерирует MAC ключ на основе RNDA RNDB
            // Generation of Session Key for calculating Message Authentication Code
            // Session Key for MAC KMAC
            // [0] = 0x22; [1..5] = rnda[11..15] ^ rndb[11..15]; [6..10] = rndb[4..8]; [11..15] = rnda[4..8]
            // @param Ключ для блока ( к котрому производилась аутентификация)
            static void GenerateMacKey(const uint8 *key, uint8[16]);

            // Генерирует MAC к сообщению
            // Prerequisites: block cipher CIPH with block size b; key K; MAC length parameter Tlen.
            // Input:  message M of bit length Mlen.
            // Output: MAC T of bit length Tlen.
            // Suggested Notation: CMAC(K, M, Tlen) or, if Tlen is understood from the context, CMAC(K, M).
            // Steps:
            // 1.  Apply the subkey generation process in Sec. 6.1 to K to produce K1 and K2. (mfp_gen_subkeys())
            // 2.  If Mlen = 0, let n = 1; else, let n = ªMlen/bº.
            // 3.  Let M1, M2, ... , Mn-1, Mn* denote the unique sequence of bit strings such that M = M1 || M2 || ... || Mn-1 || Mn*, where M1, M2,..., Mn-1 are complete blocks.2
            // 4.  If Mn* is a complete block, let Mn = K1  Mn*; else, let Mn = K2  (Mn*||10j), where j = nb-Mlen-1.
            // 5.  Let C0 = 0b.
            // 6.  For i = 1 to n, let Ci= CIPHK(Ci-1  Mi).
            // 7.  Let T = MSBTlen(Cn).
            // 8.  Return T.
            // В mac_half находятся те байты (через один), которые участвуют в передаче
            static void GenerateMac(const uint8 *msg, int len_msg, const Block16 &key, Block8 &mac_half);

            // Генерирует под-ключи
            // Steps: 1.  Let L = CIPHK(0b).
            // 2. If MSB1(L) = 0, then K1 = L << 1;  Else K1 = (L << 1) ^ Rb; see Sec. 5.3 for the definition of Rb.
            // 3. If MSB1(K1) = 0, then K2 = K1 << 1;  Else K2 = (K1 << 1) ^ Rb.  7
            // 4.  Return K1, K2
            static void GenerateSubKeys(const uint8 *key, uint8 *k1, uint8 *k2);

            // Description: Logical Bit Shift Left. Shift MSB out, and place a 0 at LSB
            //              position
            //              Ex: LSL {5,1,2,3,4} = {1,2,3,4,0}
            //
            // Arguments:   data = pointer to data block [modified]
            //              len  = length of data block
            //
            // Operation:   For each byte position, shift out highest bit with
            //              (data[n] << 1) and add in the highest bit from next lower byte
            //              with (data[n+1] >> 7)
            //              The addition is done with a bitwise OR (|)
            //              For the Least significant byte however, there is no next lower
            //              byte so this is handled last and simply set to data[len-1] <<= 1
            //
            // Assumptions: MSByte is at index 0
            //
            // Revision History:
            //   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
            //   May  03, 2013      Nnoduka Eruchalu     Updated Comments
            static void LSL(uint8 *data, size_t len = 16);

            // Возвращает номер сектора с ключом для сектора, в котором находится блок nbr_block
            static int NumberKeyForBlock(int nbr_block)
            {
                if (nbr_block < 256)                        // Если номер блока меньше 256, то это данные:
                {
                    return 0x4000 + (nbr_block / 4) * 2;    // рассчитываем для него номер ключа
                }

                return nbr_block;                           // Иначе это просто номер ключа
            }

            // Записать ключ переключения в SL2 (только для MFP X)
            static bool WriteLevel2SwitchKeySL0(const Block16 &);
        }
    }
}


namespace CardMF
{
    namespace Plus
    {
        void ResetReadWriteOperations();
    }
}


void CardMF::Plus::ResetReadWriteOperations()
{
}


bool Card::_14443::Transceive(const uint8 *tx, int len_tx, bool crc_tx, bool crc_rx)
{
    Command::Idle();

    RC663::FIFO::Clear();

    Register(MFRC630_REG_TXCRCPRESET).Write((uint8)(MFRC630_RECOM_14443A_CRC | (crc_tx ? 1 : 0)));
    Register(MFRC630_REG_RXCRCCON).Write((uint8)(MFRC630_RECOM_14443A_CRC | (crc_rx ? 1 : 0)));

    // enable the global IRQ for idle, errors and timer.
    Register(MFRC630_REG_IRQ0EN).Write(MFRC630_IRQ0EN_IDLE_IRQEN | MFRC630_IRQ0EN_ERR_IRQEN);
    Register(MFRC630_REG_IRQ1EN).Write(MFRC630_IRQ1EN_TIMER0_IRQEN);

    RC663::IRQ0::Clear();  // clear irq0
    RC663::IRQ1::Clear();  // clear irq1

    // Go into send, then straight after in receive.
    Command::Transceive(tx, len_tx);

    TimeMeterMS meter;

    while (!(RC663::IRQ1::GetValue() & MFRC630_IRQ1_GLOBAL_IRQ))        // stop polling irq1 and quit the timeout loop.
    {
        if (meter.ElapsedMS() > Card::timeout)
        {
            return false;
        }
    }

    Command::Idle();

    if (RC663::IRQ0::GetValue() & MFRC630_IRQ0_ERR_IRQ)
    {
        return false;
    }

    return true;
}


int Card::_14443::TransceiveReceive(const uint8 *tx, int len_tx, uint8 *rx, int len_rx, bool crc_tx, bool crc_rx)
{
    if (!Transceive(tx, len_tx, crc_tx, crc_rx))
    {
        return 0;
    }

    std::memset(rx, 0, (uint)len_rx);

    int length = RC663::FIFO::Length();

    RC663::FIFO::Read(rx, length > len_rx ? len_rx : length);

    return length;
}


bool Card::_14443::RATS()
{
    bool result = false;

    uint8 tx_buf[2] =
    {
        0xE0,
        0x50    // FSDI = 5 (максимальный размер кадра - 64), CID = 0
    };

    uint8 rx_buf[32];

    int rx_size = TransceiveReceive(tx_buf, 2, rx_buf, 32, 1, 1);

    if (rx_size > 0)
    {
        Card::ats.Set(rx_buf, rx_size);

//        LOG_WRITE("ATS : %s", Card::ats.ToASCII().c_str());

        result = ats.IsCorrect();
    }

    if (!result)                            // Если команда не принята картой или отработала с ошибкой
    {
        Card::Eject(false);
        RC663::RF::Off();
        Timer::Delay(50);
        RC663::Anticollision();             // Повторяем антиколлизию
    }

    return result;
}


bool Card::_14443::T_CL::GetVersion(uint8 answer[32], int *bytes_received)
{
    uint8 tx_buf[5] = { 0x90, 0x60, 0x00, 0x00, 0x00 };

    *bytes_received = TransceiveReceive(tx_buf, 5, answer, 9);

    if (*bytes_received == 1)
    {
        Card::Eject(false);
        Timer::Delay(50);
        RC663::Anticollision();
        Card::_14443::RATS();

        return false;
    }

    return true;
}


int Card::_14443::T_CL::TransceiveReceive(const uint8 *tx, int len_tx, uint8 *rx, int len_rx)
{
    uint8 PCD_I = 0x02;

//  uint8 PCD_I = (uint8)BINARY_U8(00000010);
//                                     | ||
//                                     | |+----- номер блока
//                                     | +------ всегда 1
//                                     +-------- выключить байт CID в ответном сообщении

    uint8 buffer[65] = { (uint8)(PCD_I | ((Card::number_block_T_CL++) & 0x01)) }; //-V1009
    std::memcpy(buffer + 1, tx, (uint)len_tx);

    if (!_14443::Transceive(buffer, len_tx + 1, true, true))
    {
        Card::null_bytes_received = (RC663::FIFO::Length() == 0);

        return 0;
    }

    Card::null_bytes_received = (RC663::FIFO::Length() == 0);

    const int length = RC663::FIFO::Length() - 1;   // Отбросим первые два байта - PCB и CID

    if (length != len_rx)
    {
        len_rx = length;
        if (len_rx > 65)
        {
            len_rx = 65;
        }
    }

    RC663::FIFO::Pop();         // Выбрасываем PCB

    RC663::FIFO::Read(rx, len_rx);

    return len_rx;
}


bool Card::_14443::T_CL::WritePerso(int nbr_block, const Block16 &key)
{
    uint8 tx[19] = { CMD_SL0_WRITE_PERSO }; //-V1009
    std::memcpy(tx + 1, &nbr_block, 2); //-V1086
    std::memcpy(tx + 3, key.bytes, 16);

    uint8 result = 0x00;

    if (TransceiveReceive(tx, 19, &result, 1) == 1)
    {
        return (result == CODE_T_CL_SUCCESS);
    }

    return false;
}


bool Card::_14443::T_CL::CommitPerso()
{
    uint8 tx = CMD_SL0_COMMIT_PERSO;    // Длина буфера передачи без учёта контрольной суммы

    uint8 resp = 0x00;

    if (TransceiveReceive(&tx, 1, &resp, 1) == 1)
    {
        return (resp == CODE_T_CL_SUCCESS);
    }

    return false;
}


bool Card::_14443::T_CL::WriteKeys(const Block16 *keys, int num_keys)
{
    if (num_keys != 4)
    {
        return false;
    }

    bool write0 = WritePerso(0x9000, keys[0]);
    bool write1 = WritePerso(0x9001, keys[1]);
    bool write2 = WriteLevel2SwitchKeySL0(keys[2]);
    bool write3 = WritePerso(0x9003, keys[3]);

    LOG_WRITE("write0 = %d, write1 = %d, write2 = %d, write3 = %d", write0, write1, write2, write3);

    return write0 && write1 && write2 && write3;
}


bool Card::_14443::T_CL::WriteLevel2SwitchKeySL0(const Block16 &key)
{
    if (TypeCard::Current() == TypeCard::MFP_X)
    {
        bool result = WritePerso(0x9002, key);

        return result;
    }

    return true;
}

 
bool Card::_14443::T_CL::SwitchToSL3FromSL0()
{
    Block16 *address_keys = SET2::KeysMFP::Get(0);

    bool write_keys = WriteKeys(address_keys, 4);

    // Переключаемся на уровень 1
    bool perso = CommitPerso();

    bool reconnect = RC663::RecoonectCard();

    bool result = write_keys && perso && reconnect;

    LOG_WRITE("write_keys = %d, perso = %d, reconnect = %d", write_keys, perso, reconnect);

    bool sw = true;

    // И сразу же на уровень 3
    if (result)
    {
        sw = SwitchToSL3FromSL1();
    }

    LOG_WRITE("sw = %d", sw);

    return result;
}


bool Card::_14443::T_CL::SwitchToSL3FromSL1()
{
    bool auth = FirstAuthentication(0x9003, *SET2::KeysMFP::Get(3));

    // Делаем сброс после перехода на SL3

    bool reconnect = RC663::RecoonectCard();

    LOG_WRITE("auth = %d, reconnect = %d", auth, reconnect);

    return auth && reconnect;
}


bool Card::_14443::T_CL::SwitchToSL3()
{
    if (TypeCard::IsSupportT_CL())
    {
        if (Card::SL == 0)      return SwitchToSL3FromSL0();
        else if (Card::SL == 1) return SwitchToSL3FromSL1();
    }

    return true;
}


bool Card::_14443::T_CL::ResetAuth()
{
    w_counter = 0;
    r_counter = 0;

    uint8 cmd = CMD_SL3_RESET_AUTH;
    uint8 resp = 0;

    if (TransceiveReceive(&cmd, 1, &resp, 1) == 1)
    {
        return (resp == CODE_T_CL_SUCCESS);
    }

    return false;
}


bool Card::_14443::T_CL::FirstAuthentication(int nbr_block, const Block16 &key)
{
    int nbr_key = NumberKeyForBlock(nbr_block);

    if (nbr_key >= 0x8000 ||        // Если номер ключа больше 0x9000, то сброс делать не нужно
        ResetAuth())
    {
        uint8 cmd_first[4] = { CMD_SL3_FIRST_AUTH }; //-V1009
        std::memcpy(cmd_first + 1, &nbr_key, 2);
        cmd_first[3] = 0;

        uint8 resp_first[17] = { 0x00 };

        if (TransceiveReceive(cmd_first, 4, resp_first, 17) != 17)
        {
            return false;
        }

        AES::BeginDecResponse(key.bytes, resp_first + 1, AES::CBC, Block4().bytes, 0, 0);

        uint8 cmd_sec[33] = { 0x72 };               // MFP_AUTH_B //-V1009
        rnda.Generate();
        std::memcpy(cmd_sec + 1, rnda.bytes, 16);

        cmd_sec[32] = resp_first[1];
        std::memcpy(cmd_sec + 17, resp_first + 2, 15);       // < RNB` = rotate_left(RNB,1);

        // *Теперь шифруем RNDA + _rndb`, сначала А, потом Б с наложенным на него А(CBC режим)
        //  *E(Kx, RndA || RndB‟)

        AES::BeginEncCommand(key.bytes, cmd_sec + 1, AES::CBC, Block4().bytes, 0, 0);
        AES::Enc(key.bytes, cmd_sec + 17, AES::CBC);

        /**
        * Где-то там проверяется RNDA
        * там RNDA сдвигается и получается RNDA`;
        * В ответ должен получить нечто, E(Kx, TI || RndA‟ || PICCcap2 || PCDcap2) -> 32 байта
        * либо resp[0] = 6;
        */

        uint8 resp_sec[33] = { 0x00 };

        if (TransceiveReceive(cmd_sec, 33, resp_sec, 33) != 33)
        {
            return false;
        }

        AES::BeginDecResponse(key.bytes, resp_sec + 1, AES::CBC, Block4().bytes, 0, 0);
        AES::Dec(key.bytes, resp_sec + 17, AES::CBC);

        std::memcpy(&ti, resp_sec + 1, 4);
        std::memcpy(picc_cap, resp_sec + 21, 6);
        std::memcpy(pcd_cap, resp_sec + 27, 6);
        w_counter = 0;
        r_counter = 0;
        std::memcpy(rndb.bytes, resp_first + 1, sizeof(rndb));

        GenerateSessionKey(key.bytes, session_key.bytes);

        GenerateMacKey(key.bytes, mac_key.bytes);

        return true;
    }

    return false;
}


bool Card::_14443::T_CL::ChangeKeyAES(int nbr_key_AES, const Block16 &old_key, const Block16 &new_key)
{
    LOG_WRITE("write key %X from %llu to %llu", nbr_key_AES, old_key.u64[0], new_key.u64[0]);

    Card::SetPasswordAuth(old_key);

    if (WriteBlock(nbr_key_AES, new_key))
    {
        LOG_WRITE("good write %s:%d", __FILE__, __LINE__);

        return true;
    }
    else
    {
        LOG_ERROR("bad write %s:%d", __FILE__, __LINE__);
    }

    return false;
}


bool Card::_14443::T_CL::FollowingAutentication(int nbr_block, const Block16 &key)
{
    uint8 resp_first[17] = { 0x00 };

    {
        uint8 cmd_first[3] = { CMD_SL3_FOLLOW_AUTH }; //-V1009
        int nbr_sector = NumberKeyForBlock(nbr_block);
        std::memcpy(cmd_first + 1, &nbr_sector, 2);

        if (TransceiveReceive(cmd_first, 3, resp_first, 17) != 17 ||
            resp_first[0] != CODE_T_CL_SUCCESS)
        {
            return false;
        }

        AES::BeginDecResponse(key.bytes, resp_first + 1, AES::CBC, ti.bytes, r_counter, w_counter);  // Сейчас в resp_first находится декодированные Rnd B
    }

    {
        uint8 cmd_sec[33] = { 0x72 };               // Код второго шага авторизации //-V1009
        std::memcpy(cmd_sec + 1, rnda.bytes, 16);   // Установили номер команду и RNDA, хотя оно и должно быть случайным

        cmd_sec[32] = resp_first[1];
        std::memcpy(cmd_sec + 17, resp_first + 2, 15); // RNB` = rotate_left(RNB,1);

        AES::BeginEncCommand(key.bytes, cmd_sec + 1, AES::CBC, ti.bytes, r_counter, w_counter);
        AES::Enc(key.bytes, cmd_sec + 17, AES::CBC);

        uint8 resp_sec[17];

        if (TransceiveReceive(cmd_sec, 33, resp_sec, 17) != 17 ||
            resp_sec[0] != CODE_T_CL_SUCCESS)
        {
            return false;
        }

        std::memcpy(rndb.bytes, resp_first + 1, 16);
    }

    GenerateSessionKey(key.bytes, session_key.bytes);

    GenerateMacKey(key.bytes, mac_key.bytes);

    return true;
}


bool Card::_14443::T_CL::ReadBlock(int nbr_block, Block16 &block)
{
    // \todo сделать проверку ответного MAC

    if (FirstAuthentication(nbr_block, Card::GetPasswordAuth()))
    {
        bool enc = TypeCard::IsReadEncripted();

        uint8 mac_cmd[10] = { enc ? CMD_SL3_READ_ENCRIPTED : CMD_SL3_READ_PLAIN }; //-V1009
        std::memcpy(mac_cmd + 1, &r_counter, 2);
        std::memcpy(mac_cmd + 3, ti.bytes, 4);
        std::memcpy(mac_cmd + 7, &nbr_block, 2);
        mac_cmd[9] = 0x01;

        Block8 mac_half;

        GenerateMac(mac_cmd, 10, mac_key, mac_half);

        uint8 cmd[12] = { mac_cmd[0] }; //-V1009
        std::memcpy(cmd + 1, &nbr_block, 2); //-V1086
        cmd[3] = 0x01;
        std::memcpy(cmd + 4, mac_half.bytes, 8);

        uint8 resp[25];

        if (TransceiveReceive(cmd, 12, resp, 25) != 25 ||
            resp[0] != CODE_T_CL_SUCCESS)
        {
            return false;
        }

        block.FillBytes(resp + 1);

        r_counter++;

        if (enc)
        {
            AES::BeginDecResponse(session_key.bytes, block.bytes, AES::CBC, ti.bytes, r_counter, w_counter);
        }

        return true;
    }

    return false;
}


bool Card::_14443::T_CL::WriteBlock(int nbr_block, const Block16 &data)
{
    // \todo Сделать проверку ответного MAC

    if (FirstAuthentication(nbr_block, Card::GetPasswordAuth()))
    {
        bool enc = TypeCard::IsReadEncripted() ||       // Будем шифровать данные, если карта поддерживает шифрование
            nbr_block > 255;                            // Или записываем не блоки данных (в Mifare Plus S в этом случае надо шифровать)

        uint8 mac_cmd[25] = { enc ? CMD_SL3_WRITE_ENCRIPTED : CMD_SL3_WRITE_PLAIN }; //-V1009

        {                                                                   // Готовим данные для генерации MAC
            std::memcpy(mac_cmd + 1, &w_counter, 2);
            std::memcpy(mac_cmd + 3, ti.bytes, 4);
            std::memcpy(mac_cmd + 7, &nbr_block, 2); //-V1086
            std::memcpy(mac_cmd + 9, data.bytes, 16);

            if (enc)
            {
                AES::BeginEncCommand(session_key.bytes, mac_cmd + 9, AES::CBC, ti.bytes, r_counter, w_counter);
            }
        }

        Block8 mac_half;
        GenerateMac(mac_cmd, 25, mac_key, mac_half);            // Рассчитан MAC

        uint8 cmd[27] = { mac_cmd[0] }; //-V1009
        std::memcpy(cmd + 1, &nbr_block, 2); //-V1086
        std::memcpy(cmd + 3, mac_cmd + 9, 16);
        std::memcpy(cmd + 19, mac_half.bytes, 8);

        uint8 resp[9] = { 0xFF }; //-V1009

        int length = TransceiveReceive(cmd, 27, resp, 9);

        if (length != 9 ||
            resp[0] != CODE_T_CL_SUCCESS)
        {
            LOG_ERROR("bad transceive. resp code %02X, length=%d", resp[0], length);

            return false;
        }

        w_counter++;

        return true;
    }

    return false;
}


// http://www.gorferay.com/aes-128-key-diversification-example/
// Generation of Session Key for Encryption
// Session Key for Encryption KENC
// Расчет сессионного ключа -> [0] = 0x11; [1..5] = rnda[7..11] ^ rndb[7..11]; [6..10] = rndb[0..4]; [11..15] = rnda[0..4]; !вверх ногами
//
// sess[0..4] = rnda[11..15]
// sess[5..10] = rndb[11..15]
// sess[11..15] = rnda[4..8]^rndb[4..8];
// session = aes128(k,session);
void Card::_14443::T_CL::GenerateSessionKey(const uint8 *key, uint8 ses_key[16])
{
    ses_key[15] = 0x11;

    for (uint8 i = 0; i < 5; i++)
    {
        ses_key[i + 10U] = (uint8)(rnda.bytes[i + 4U] ^ rndb.bytes[i + 4U]);
        ses_key[i + 5U] = rndb.bytes[i + 11U];
        ses_key[i] = rnda.bytes[i + 11U];
    }

    AES::Enc(key, ses_key);
}


// Generation of Session Key for calculating Message Authentication Code
// Session Key for MAC KMAC
// [0] = 0x22; [1..5] = rnda[11..15] ^ rndb[11..15]; [6..10] = rndb[4..8]; [11..15] = rnda[4..8]
void Card::_14443::T_CL::GenerateMacKey(const uint8 *key, uint8 mac[16])
{
    mac[15] = 0x22;

    for (uint8 i = 0; i < 5; i++)
    {
        mac[i + 10] = (uint8)(rnda.bytes[i] ^ rndb.bytes[i]);
        mac[i + 5] = rndb.bytes[i + 7U];
        mac[i] = rnda.bytes[i + 7U];
    }

    AES::Enc(key, mac);
}


// Prerequisites: block cipher CIPH with block size b; key K; MAC length parameter Tlen.
// Input:  message M of bit length Mlen.
// Output: MAC T of bit length Tlen.
// Suggested Notation: CMAC(K, M, Tlen) or, if Tlen is understood from the context, CMAC(K, M).
// Steps:
// 1.  Apply the subkey generation process in Sec. 6.1 to K to produce K1 and K2. (mfp_gen_subkeys())
// 2.  If Mlen = 0, let n = 1; else, let n = ªMlen/bº.
// 3.  Let M1, M2, ... , Mn-1, Mn* denote the unique sequence of bit strings such that M = M1 || M2 || ... || Mn-1 || Mn*, where M1, M2,..., Mn-1 are complete blocks.2
// 4.  If Mn* is a complete block, let Mn = K1  Mn*; else, let Mn = K2  (Mn*||10j), where j = nb-Mlen-1.
// 5.  Let C0 = 0b.
// 6.  For i = 1 to n, let Ci= CIPHK(Ci-1  Mi).
// 7.  Let T = MSBTlen(Cn).
// 8.  Return T.
void Card::_14443::T_CL::GenerateMac(const uint8 *msg, int len_msg, const Block16 &key, Block8 &mac_half)
{
    Block16 mac_full(0, 0);

    uint8 k1[16] = {};
    uint8 k2[16] = {};
    GenerateSubKeys(key.bytes, k1, k2);
    uint16 n = (uint16)(len_msg / 16);      // Количество блоков

    if (len_msg == 0 || (len_msg % 16))
    {
        // Блок не полный
        std::memcpy(mac_full.bytes, &msg[n * 16], (uint)(len_msg % 16));
        std::memset(&mac_full.bytes[(uint)(len_msg % 16)], 0, (uint)(16 - (len_msg % 16))); //-V557
        mac_full.bytes[(uint)(len_msg % 16)] = 0x80; //-V557
        for (uint8 i = 0; i < 16; i++)
        {
            mac_full.bytes[i] = (uint8)(mac_full.bytes[i] ^ k2[i]);
        }
    }
    else
    {
        // Блок полный
        for (uint8 i = 0; i < 16; i++)
        {
            mac_full.bytes[i] = (uint8)(msg[((n - 1) * 16) + i] ^ k1[i]);
        }
        n--;
    }
    uint8 iv[16] = {};
    for (uint16_t i = 0; i < n; i++)
    {
        uint8 tmp[16] = {};
        std::memcpy(tmp, &msg[i * 16], sizeof(tmp));
        AES::Enc(key.bytes, tmp, AES::CBC, iv);
    }

    AES::Enc(key.bytes, mac_full.bytes, AES::CBC, iv);

    for (int i = 1; i < 16; i += 2)
    {
        mac_half.bytes[i / 2] = mac_full.bytes[i];
    }
}


// Генерирует под-ключи
// Steps: 1.  Let L = CIPHK(0b).
// 2. If MSB1(L) = 0, then K1 = L << 1;  Else K1 = (L << 1) ^ Rb; see Sec. 5.3 for the definition of Rb.
// 3. If MSB1(K1) = 0, then K2 = K1 << 1;  Else K2 = (K1 << 1) ^ Rb.  7
// 4.  Return K1, K2
void Card::_14443::T_CL::GenerateSubKeys(const uint8 *key, uint8 *k1, uint8 *k2)
{
    Block16 L(0, 0);
    /**
    * 1. Let L = CIPHK(0b).
    */
    AES::Enc(key, L.bytes);
    /**
    * 2. If MSB1(L) = 0, then K1 = L << 1;  Else K1 = (L << 1) ^ Rb; see Sec. 5.3 for the definition of Rb.
    */
    std::memcpy(k1, L.bytes, sizeof(L));
    LSL(k1);
    if ((L.bytes[0] & 0x80) == 0x80)
    {
        k1[15] ^= 0x87;
    }
    /**
    * 3. If MSB1(K1) = 0, then K2 = K1 << 1;  Else K2 = (K1 << 1) ^ Rb.  7
    */
    std::memcpy(k2, k1, sizeof(L));
    LSL(k2);
    if ((k1[0] & 0x80) == 0x80)
    {
        k2[15] ^= 0x87;
    }
}


// Description: Logical Bit Shift Left. Shift MSB out, and place a 0 at LSB
//              position
//              Ex: LSL {5,1,2,3,4} = {1,2,3,4,0}
//
// Arguments:   data = pointer to data block [modified]
//              len  = length of data block
//
// Operation:   For each byte position, shift out highest bit with
//              (data[n] << 1) and add in the highest bit from next lower byte
//              with (data[n+1] >> 7)
//              The addition is done with a bitwise OR (|)
//              For the Least significant byte however, there is no next lower
//              byte so this is handled last and simply set to data[len-1] <<= 1
//
// Assumptions: MSByte is at index 0
//
// Revision History:
//   Jan. 03, 2012      Nnoduka Eruchalu     Initial Revision
//   May  03, 2013      Nnoduka Eruchalu     Updated Comments
void Card::_14443::T_CL::LSL(uint8 *data, size_t len)
{
    size_t n; /* index into data */
    for (n = 0; n < len - 1; n++)
    {
        data[n] = (uint8)((data[n] << 1) | (data[n + 1] >> 7));
    }
    data[len - 1] <<= 1;
}


bool Card::_14443::T_CL::InSL0()
{
    Block16 block(0, 0);

    return WritePerso(0x9000, block);
}
