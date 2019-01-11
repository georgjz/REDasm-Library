#include <zlib.h>
#include "n64.h"
#include "n64_analyzer.h"


// https://level42.ca/projects/ultra64/Documentation/man/pro-man/pro09/index9.3.html
// http://en64.shoutwiki.com/wiki/ROM#Cartridge_ROM_Header

#define N64_KUSEG_START_ADDR   0x00000000   // TLB mapped
#define N64_KUSEG_SIZE         0x7fffffff

#define N64_KSEG0_START_ADDR   0x80000000   // Direct mapped, cached
#define N64_KSEG0_SIZE         0x1FFFFFFF

#define N64_KSEG1_START_ADDR   0xa0000000   // Direct mapped, uncached
#define N64_KSEG1_SIZE         0x1FFFFFFF

#define N64_KSSEG_START_ADDR   0xc0000000   // TLB mapped
#define N64_KSSEG_SIZE         0x1FFFFFFF

#define N64_KSEG3_START_ADDR   0xe0000000   // TLB mapped
#define N64_KSEG3_SIZE         0x1FFFFFFF

#define N64_SEGMENT_AREA(name) N64_##name##_START_ADDR, N64_##name##_SIZE

namespace REDasm {

N64RomFormat::N64RomFormat(Buffer &buffer): FormatPluginT<N64RomHeader>(buffer) { }
const char *N64RomFormat::name() const { return "Nintendo 64 ROM"; }
u32 N64RomFormat::bits() const { return 64; }
const char *N64RomFormat::assembler() const { return "mips64be"; }

endianness_t N64RomFormat::endianness() const
{
    return Endianness::BigEndian;
}

Analyzer *N64RomFormat::createAnalyzer(DisassemblerAPI *disassembler, const SignatureFiles &signatures) const
{
    return new N64Analyzer(disassembler, signatures);
}

bool N64RomFormat::load()
{
    if(!this->validateRom())
        return false;

    m_document.segment("KSEG0", N64_ROM_HEADER_SIZE, this->getEP(), m_buffer.size()-N64_ROM_HEADER_SIZE, SegmentTypes::Code | SegmentTypes::Data);
    // TODO: map other segments
    m_document.entry(this->getEP());
    return true;
}

u32 N64RomFormat::getEP()
{
    u32 pc = Endianness::cfbe(static_cast<u32>(m_format->program_counter));
    u32 cic_version = N64RomFormat::getCICVersion();

    if(cic_version != 0) {
        if(cic_version == 6103)         // CIC 6103 EP manipulation
            pc -= 0x100000;
        else if (cic_version == 6106)   // CIC 6106 EP manipulation
            pc -= 0x200000;
    }

    return pc;
}

u32 N64RomFormat::calculateChecksum(u32 *crc) // Adapted from n64crc (http://n64dev.org/n64crc.html)
{
    u32 bootcode, i;
    u32 seed;

    u32 t1, t2, t3;
    u32 t4, t5, t6;
    u32 r, d;

    switch ((bootcode = N64RomFormat::getCICVersion())) {
        case 6101:
        case 7102:
        case 6102:
            seed = N64_ROM_CHECKSUM_CIC_6102;
            break;
        case 6103:
            seed = N64_ROM_CHECKSUM_CIC_6103;
            break;
        case 6105:
            seed = N64_ROM_CHECKSUM_CIC_6105;
            break;
        case 6106:
            seed = N64_ROM_CHECKSUM_CIC_6106;
            break;
        default:
            return 1;
    }

    t1 = t2 = t3 = t4 = t5 = t6 = seed;

    i = N64_ROM_CHECKSUM_START;
    while (i < (N64_ROM_CHECKSUM_START + N64_ROM_CHECKSUM_LENGTH)) {
        d = Endianness::cfbe(*reinterpret_cast<u32*>(&m_buffer[i]));

        if ((t6 + d) < t6) t4++;
        t6 += d;
        t3 ^= d;
        r = REDasm::rol(d, (d & 0x1F));
        t5 += r;
        if (t2 > d) t2 ^= r;
        else t2 ^= t6 ^ d;

        if (bootcode == 6105) t1 += Endianness::cfbe(*reinterpret_cast<u32*>(&m_buffer[N64_ROM_HEADER_SIZE + 0x0710 + (i & 0xFF)])) ^ d;
        else t1 += t5 ^ d;

        i += 4;
    }
    if (bootcode == 6103) {
        crc[0] = (t6 ^ t4) + t3;
        crc[1] = (t5 ^ t2) + t1;
    }
    else if (bootcode == 6106) {
        crc[0] = (t6 * t4) + t3;
        crc[1] = (t5 * t2) + t1;
    }
    else {
        crc[0] = t6 ^ t4 ^ t3;
        crc[1] = t5 ^ t2 ^ t1;
    }

    return 0;
}

u8 N64RomFormat::checkChecksum()
{
    u32 crc[2];
    u8 result = false;

    if(N64RomFormat::calculateChecksum(crc) == 0)
        if(crc[0] == Endianness::cfbe(static_cast<u32>(m_format->crc1)) && crc[1] == Endianness::cfbe(static_cast<u32>(m_format->crc2)))
            result = true;

    return  result;
}


u32 N64RomFormat::getCICVersion()
{
    u64 boot_code_crc = crc32(0L, reinterpret_cast<const unsigned char*>(m_format->boot_code), N64_BOOT_CODE_SIZE);
    u32 cic = 0;

    switch (boot_code_crc) {
        case N64_BOOT_CODE_CIC_6101_CRC:
            cic = 6101;
        break;
        case N64_BOOT_CODE_CIC_7102_CRC:
            cic = 7102;
        break;
        case N64_BOOT_CODE_CIC_6102_CRC:
            cic =  6102;
        break;
        case N64_BOOT_CODE_CIC_6103_CRC:
            cic = 6103;
        break;
        case N64_BOOT_CODE_CIC_6105_CRC:
            cic = 6105;
        break;
        case N64_BOOT_CODE_CIC_6106_CRC:
            cic = 6106;
        break;
    }

    return cic;
}


u8 N64RomFormat::checkMediaType()
{
    bool result = false;

    switch (m_format->media_format[3]) {
    case 'N':           // Cart
        result = true;
        break;
    case 'D':           // 64DD disk
        result = true;
        break;
    case 'C':           // Cartridge part of expandable game
        result = true;
        break;
    case 'E':           // 64DD expansion for cart
        result = true;
        break;
    case 'Z':           // Aleck64 cart
        result = true;
        break;
    default:
        result = false;
        break;
    }

    return result;
}

u8 N64RomFormat::checkCountryCode()
{
    /*
     0x37 '7' "Beta"
     0x41 'A' "Asian (NTSC)"
     0x42 'B' "Brazilian"
     0x43 'C' "Chinese"
     0x44 'D' "German"
     0x45 'E' "North America"
     0x46 'F' "French"
     0x47 'G': Gateway 64 (NTSC)
     0x48 'H' "Dutch"
     0x49 'I' "Italian"
     0x4A 'J' "Japanese"
     0x4B 'K' "Korean"
     0x4C 'L': Gateway 64 (PAL)
     0x4E 'N' "Canadian"
     0x50 'P' "European (basic spec.)"
     0x53 'S' "Spanish"
     0x55 'U' "Australian"
     0x57 'W' "Scandinavian"
     0x58 'X' "European"
     0x59 'Y' "European"
    */

    if(m_format->country_code == 0x37)
        return true;
    else if(m_format->country_code >= 0x41 &&  m_format->country_code <= 0x4C)
        return true;
    else if(m_format->country_code == 0x4E || m_format->country_code == 0x50)
        return true;
    else if(m_format->country_code == 0x53 || m_format->country_code == 0x55)
        return true;
    else if(m_format->country_code >= 0x57 &&  m_format->country_code <= 0x59)
        return true;
    else
        return false;
}

bool N64RomFormat::validateRom()
{
    u32 magic_number = static_cast<u32>(m_buffer);

    if(magic_number != 0x80371240 && magic_number != 0x37804012)
        return false;

    if(m_buffer.size() < N64_ROM_HEADER_SIZE)
        return false;

    if(m_format->pi_bsb_dom1_lat_reg == 0x37)
        m_buffer.swapEndianness<u16>();

    if(!N64RomFormat::checkMediaType())
        return false;

    if(!N64RomFormat::checkCountryCode())
        return false;

   return N64RomFormat::checkChecksum();
}

}