/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "_highgui.h"
#include "bitstrm.h"

#define  BS_DEF_BLOCK_SIZE   (1<<15)

const ulong bs_bit_mask[] = {
    0,
    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

void bsBSwapBlock( uchar *start, uchar *end )
{
    ulong* data = (ulong*)start;
    int i, size = (int)(end - start+3)/4;

    for( i = 0; i < size; i++ )
    {
        ulong temp = data[i];
        temp = BSWAP( temp );
        data[i] = temp;
    }
}

bool  bsIsBigEndian( void )
{
    return (((const int*)"\0\x1\x2\x3\x4\x5\x6\x7")[0] & 255) != 0;
}

/////////////////////////  RBaseStream ////////////////////////////

bool  RBaseStream::IsOpened()
{ 
    return m_is_opened;
}

void  RBaseStream::Allocate()
{
    if( !m_start )
    {
        m_start = new uchar[m_block_size + m_unGetsize];
        m_start+= m_unGetsize;
    }
    m_end = m_start + m_block_size;
    m_current = m_end;
}


RBaseStream::RBaseStream()
{
    m_start = m_end = m_current = 0;
    m_file = 0;
    m_block_size = BS_DEF_BLOCK_SIZE;
    m_unGetsize = 4; // 32 bits
    m_is_opened = false;
    m_jmp_set = false;
}


RBaseStream::~RBaseStream()
{
    Close();    // Close files
    Release();  // free  buffers
}


void  RBaseStream::ReadBlock()
{
    size_t readed;
    assert( m_file != 0 );

    // copy unget buffer
    if( m_start )
    {
        memcpy( m_start - m_unGetsize, m_end - m_unGetsize, m_unGetsize );
    }

    SetPos( GetPos() ); // normalize position

    fseek( m_file, m_block_pos, SEEK_SET );
    readed = fread( m_start, 1, m_block_size, m_file );
    m_end = m_start + readed;
    m_current   -= m_block_size;
    m_block_pos += m_block_size;

    if( readed == 0 || m_current >= m_end )
    {
        if( m_jmp_set )
            longjmp( m_jmp_buf, RBS_THROW_EOS );
    }
}


bool  RBaseStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_file = fopen( filename, "rb" );
    
    if( m_file )
    {
        m_is_opened = true;
        SetPos(0);
    }
    return m_file != 0;
}

void  RBaseStream::Close()
{
    if( m_file )
    {
        fclose( m_file );
        m_file = 0;
    }
    m_is_opened = false;
}


void  RBaseStream::Release()
{
    if( m_start )
    {
        delete[] (m_start - m_unGetsize);
    }
    m_start = m_end = m_current = 0;
}


void  RBaseStream::SetBlockSize( int block_size, int unGetsize )
{
    assert( unGetsize >= 0 && block_size > 0 &&
           (block_size & (block_size-1)) == 0 );

    if( m_start && block_size == m_block_size && unGetsize == m_unGetsize ) return;
    Release();
    m_block_size = block_size;
    m_unGetsize = unGetsize;
    Allocate();
}


void  RBaseStream::SetPos( int pos )
{
    int offset = pos & (m_block_size - 1);
    int block_pos = pos - offset;
    
    assert( IsOpened() && pos >= 0 );
    
    if( m_current < m_end && block_pos == m_block_pos - m_block_size )
    {
        m_current = m_start + offset;
    }
    else
    {
        m_block_pos = block_pos;
        m_current = m_start + m_block_size + offset;
    }
}


int  RBaseStream::GetPos()
{
    assert( IsOpened() );
    return m_block_pos - m_block_size + (int)(m_current - m_start);
}

void  RBaseStream::Skip( int bytes )
{
    assert( bytes >= 0 );
    m_current += bytes;
}

jmp_buf& RBaseStream::JmpBuf()
{ 
    m_jmp_set = true;
    return m_jmp_buf;
}

/////////////////////////  RLByteStream ////////////////////////////

RLByteStream::~RLByteStream()
{
}

int  RLByteStream::GetByte()
{
    uchar *current = m_current;
    int   val;

    if( current >= m_end )
    {
        ReadBlock();
        current = m_current;
    }

    val = *((uchar*)current);
    m_current = current + 1;
    return val;
}


void  RLByteStream::GetBytes( void* buffer, int count, int* readed )
{
    uchar*  data = (uchar*)buffer;
    assert( count >= 0 );
    
    if( readed) *readed = 0;

    while( count > 0 )
    {
        int l;

        for(;;)
        {
            l = (int)(m_end - m_current);
            if( l > count ) l = count;
            if( l > 0 ) break;
            ReadBlock();
        }
        memcpy( data, m_current, l );
        m_current += l;
        data += l;
        count -= l;
        if( readed ) *readed += l;
    }
}


////////////  RLByteStream & RMByteStream <Get[d]word>s ////////////////

RMByteStream::~RMByteStream()
{
}


int  RLByteStream::GetWord()
{
    uchar *current = m_current;
    int   val;

    if( current+1 < m_end )
    {
        val = current[0] + (current[1] << 8);
        m_current = current + 2;
    }
    else
    {
        val = GetByte();
        val|= GetByte() << 8;
    }
    return val;
}


int  RLByteStream::GetDWord()
{
    uchar *current = m_current;
    int   val;

    if( current+3 < m_end )
    {
        val = current[0] + (current[1] << 8) +
              (current[2] << 16) + (current[3] << 24);
        m_current = current + 4;
    }
    else
    {
        val = GetByte();
        val |= GetByte() << 8;
        val |= GetByte() << 16;
        val |= GetByte() << 24;
    }
    return val;
}


int  RMByteStream::GetWord()
{
    uchar *current = m_current;
    int   val;

    if( current+1 < m_end )
    {
        val = (current[0] << 8) + current[1];
        m_current = current + 2;
    }
    else
    {
        val = GetByte() << 8;
        val|= GetByte();
    }
    return val;
}


int  RMByteStream::GetDWord()
{
    uchar *current = m_current;
    int   val;

    if( current+3 < m_end )
    {
        val = (current[0] << 24) + (current[1] << 16) +
              (current[2] << 8) + current[3];
        m_current = current + 4;
    }
    else
    {
        val = GetByte() << 24;
        val |= GetByte() << 16;
        val |= GetByte() << 8;
        val |= GetByte();
    }
    return val;
}


/////////////////////////  RLBitStream ////////////////////////////

RLBitStream::~RLBitStream()
{
}


void  RLBitStream::ReadBlock()
{
    RBaseStream::ReadBlock();
    if( bsIsBigEndian() )
        bsBSwapBlock( m_start, m_end );
}


void  RLBitStream::SetPos( int pos )
{
    RBaseStream::SetPos(pos);
    int offset = (int)(m_current - m_end);
    m_current = m_end + (offset & -4);
    m_bit_idx = (offset&3)*8;
}


int  RLBitStream::GetPos()
{
    return RBaseStream::GetPos() + (m_bit_idx >> 3);
}


int  RLBitStream::Get( int bits )
{
    int    bit_idx     = m_bit_idx;
    int    new_bit_idx = bit_idx + bits;
    int    mask    = new_bit_idx >= 32 ? -1 : 0;
    ulong* current = (ulong*)m_current;

    assert( (unsigned)bits < 32 );

    if( (m_current = (uchar*)(current - mask)) >= m_end )
    {
        ReadBlock();
        current = ((ulong*)m_current) + mask;
    }
    m_bit_idx = new_bit_idx & 31;
    return ((current[0] >> bit_idx) |
           ((current[1] <<-bit_idx) & mask)) & bs_bit_mask[bits];
}

int  RLBitStream::Show( int bits )
{
    int    bit_idx = m_bit_idx;
    int    new_bit_idx = bit_idx + bits;
    int    mask    = new_bit_idx >= 32 ? -1 : 0;
    ulong* current = (ulong*)m_current;

    assert( (unsigned)bits < 32 );

    if( (uchar*)(current - mask) >= m_end )
    {
        ReadBlock();
        current = ((ulong*)m_current) + mask;
        m_current = (uchar*)current;
    }
    return ((current[0] >> bit_idx) |
           ((current[1] <<-bit_idx) & mask)) & bs_bit_mask[bits];
}


void  RLBitStream::Move( int shift )
{
    int new_bit_idx = m_bit_idx + shift;
    m_current += (new_bit_idx >> 5) << 2;
    m_bit_idx  = new_bit_idx & 31;
}


int  RLBitStream::GetHuff( const short* table )
{
    int  val;
    int  code_bits;

    for(;;)
    {
        int table_bits = table[0];
        val = table[Show(table_bits) + 2];
        code_bits = val & 15;
        val >>= 4;

        if( code_bits != 0 ) break;
        table += val*2;
        Move( table_bits );
    }

    Move( code_bits );
    if( val == RBS_HUFF_FORB )
    {
        if( m_jmp_set )
            longjmp( m_jmp_buf, RBS_THROW_FORB );
    }

    return val;
}

void  RLBitStream::Skip( int bytes )
{
    Move( bytes*8 );
}

/////////////////////////  RMBitStream ////////////////////////////


RMBitStream::~RMBitStream()
{
}


void  RMBitStream::ReadBlock()
{
    RBaseStream::ReadBlock();
    if( !bsIsBigEndian() )
        bsBSwapBlock( m_start, m_end );
}


void  RMBitStream::SetPos( int pos )
{
    RBaseStream::SetPos(pos);
    int offset = (int)(m_current - m_end);
    m_current = m_end + ((offset - 1) & -4);
    m_bit_idx = (32 - (offset&3)*8) & 31;
}


int  RMBitStream::GetPos()
{
    return RBaseStream::GetPos() + ((32 - m_bit_idx) >> 3);
}


int  RMBitStream::Get( int bits )
{
    int    bit_idx = m_bit_idx - bits;
    int    mask    = bit_idx >> 31;
    ulong* current = ((ulong*)m_current) - mask;

    assert( (unsigned)bits < 32 );

    if( (m_current = (uchar*)current) >= m_end )
    {
        ReadBlock();
        current = (ulong*)m_current;
    }
    m_bit_idx = bit_idx &= 31;
    return (((current[-1] << -bit_idx) & mask)|
             (current[0] >> bit_idx)) & bs_bit_mask[bits];
}


int  RMBitStream::Show( int bits )
{
    int    bit_idx = m_bit_idx - bits;
    int    mask    = bit_idx >> 31;
    ulong* current = ((ulong*)m_current) - mask;

    assert( (unsigned)bits < 32 );

    if( ((uchar*)current) >= m_end )
    {
        m_current = (uchar*)current;
        ReadBlock();
        current = (ulong*)m_current;
        m_current -= 4;
    }
    return (((current[-1]<<-bit_idx) & mask)|
             (current[0] >> bit_idx)) & bs_bit_mask[bits];
}


int  RMBitStream::GetHuff( const short* table )
{
    int  val;
    int  code_bits;

    for(;;)
    {
        int table_bits = table[0];
        val = table[Show(table_bits) + 1];
        code_bits = val & 15;
        val >>= 4;

        if( code_bits != 0 ) break;
        table += val;
        Move( table_bits );
    }

    Move( code_bits );
    if( val == RBS_HUFF_FORB )
    {
        if( m_jmp_set )
            longjmp( m_jmp_buf, RBS_THROW_FORB );
    }

    return val;
}


void  RMBitStream::Move( int shift )
{
    int new_bit_idx = m_bit_idx - shift;
    m_current -= (new_bit_idx >> 5)<<2;
    m_bit_idx  = new_bit_idx & 31;
}


void  RMBitStream::Skip( int bytes )
{
    Move( bytes*8 );
}


static const int huff_val_shift = 20, huff_code_mask = (1 << huff_val_shift) - 1;

bool bsCreateDecodeHuffmanTable( const int* src, short* table, int max_size )
{   
    const int forbidden_entry = (RBS_HUFF_FORB << 4)|1;
    int       first_bits = src[0];
    struct
    {
        int bits;
        int offset;
    }
    sub_tables[1 << 11];
    int  size = (1 << first_bits) + 1;
    int  i, k;
    
    /* calc bit depths of sub tables */
    memset( sub_tables, 0, ((size_t)1 << first_bits)*sizeof(sub_tables[0]) );
    for( i = 1, k = 1; src[k] >= 0; i++ )
    {
        int code_count = src[k++];
        int sb = i - first_bits;
        
        if( sb <= 0 )
            k += code_count;
        else
            for( code_count += k; k < code_count; k++ )
            {
                int  code = src[k] & huff_code_mask;
                sub_tables[code >> sb].bits = sb;
            }
    }

    /* calc offsets of sub tables and whole size of table */
    for( i = 0; i < (1 << first_bits); i++ )
    {
        int b = sub_tables[i].bits;
        if( b > 0 )
        {
            b = 1 << b;
            sub_tables[i].offset = size;
            size += b + 1;
        }
    }

    if( size > max_size )
    {
        assert(0);
        return false;
    }

    /* fill first table and subtables with forbidden values */
    for( i = 0; i < size; i++ )
    {
        table[i] = (short)forbidden_entry;
    }

    /* write header of first table */
    table[0] = (short)first_bits;

    /* fill first table and sub tables */ 
    for( i = 1, k = 1; src[k] >= 0; i++ )
    {
        int code_count = src[k++];
        for( code_count += k; k < code_count; k++ )
        {
            int  table_bits= first_bits;
            int  code_bits = i;
            int  code = src[k] & huff_code_mask;
            int  val  = src[k] >>huff_val_shift;
            int  j, offset = 0;

            if( code_bits > table_bits )
            {
                int idx = code >> (code_bits -= table_bits);
                code &= (1 << code_bits) - 1;
                offset   = sub_tables[idx].offset;
                table_bits= sub_tables[idx].bits;
                /* write header of subtable */
                table[offset]  = (short)table_bits;
                /* write jump to subtable */
                table[idx + 1]= (short)(offset << 4);
            }
        
            table_bits -= code_bits;
            assert( table_bits >= 0 );
            val = (val << 4) | code_bits;
            offset += (code << table_bits) + 1;
        
            for( j = 0; j < (1 << table_bits); j++ )
            {
                assert( table[offset + j] == forbidden_entry );
                table[ offset + j ] = (short)val;
            }
        }
    }
    return true;
}


int*  bsCreateSourceHuffmanTable( const uchar* src, int* dst,
                                  int max_bits, int first_bits )
{
    int   i, val_idx, code = 0;
    int*  table = dst;
    *dst++ = first_bits;
    for( i = 1, val_idx = max_bits; i <= max_bits; i++ )
    {
        int code_count = src[i - 1];
        dst[0] = code_count;
        code <<= 1;
        for( int k = 0; k < code_count; k++ )
        {
            dst[k + 1] = (src[val_idx + k] << huff_val_shift)|(code + k);
        }
        code += code_count;
        dst += code_count + 1;
        val_idx += code_count;
    }
    dst[0] = -1;
    return  table;
}


/////////////////////////// WBaseStream /////////////////////////////////

// WBaseStream - base class for output streams
WBaseStream::WBaseStream()
{
    m_start = m_end = m_current = 0;
    m_file = 0;
    m_block_size = BS_DEF_BLOCK_SIZE;
    m_is_opened = false;
}


WBaseStream::~WBaseStream()
{
    Close();    // Close files
    Release();  // free  buffers
}


bool  WBaseStream::IsOpened()
{ 
    return m_is_opened;
}


void  WBaseStream::Allocate()
{
    if( !m_start )
        m_start = new uchar[m_block_size];

    m_end = m_start + m_block_size;
    m_current = m_start;
}


void  WBaseStream::WriteBlock()
{
    int size = (int)(m_current - m_start);
    assert( m_file != 0 );

    //fseek( m_file, m_block_pos, SEEK_SET );
    fwrite( m_start, 1, size, m_file );
    m_current = m_start;

    /*if( written < size ) throw RBS_THROW_EOS;*/
    
    m_block_pos += size;
}


bool  WBaseStream::Open( const char* filename )
{
    Close();
    Allocate();
    
    m_file = fopen( filename, "wb" );
    
    if( m_file )
    {
        m_is_opened = true;
        m_block_pos = 0;
        m_current = m_start;
    }
    return m_file != 0;
}


void  WBaseStream::Close()
{
    if( m_file )
    {
        WriteBlock();
        fclose( m_file );
        m_file = 0;
    }
    m_is_opened = false;
}


void  WBaseStream::Release()
{
    if( m_start )
    {
        delete[] m_start;
    }
    m_start = m_end = m_current = 0;
}


void  WBaseStream::SetBlockSize( int block_size )
{
    assert( block_size > 0 && (block_size & (block_size-1)) == 0 );

    if( m_start && block_size == m_block_size ) return;
    Release();
    m_block_size = block_size;
    Allocate();
}


int  WBaseStream::GetPos()
{
    assert( IsOpened() );
    return m_block_pos + (int)(m_current - m_start);
}


///////////////////////////// WLByteStream /////////////////////////////////// 

WLByteStream::~WLByteStream()
{
}

void WLByteStream::PutByte( int val )
{
    *m_current++ = (uchar)val;
    if( m_current >= m_end )
        WriteBlock();
}


void WLByteStream::PutBytes( const void* buffer, int count )
{
    uchar* data = (uchar*)buffer;
    
    assert( data && m_current && count >= 0 );

    while( count )
    {
        int l = (int)(m_end - m_current);
        
        if( l > count )
            l = count;
        
        if( l > 0 )
        {
            memcpy( m_current, data, l );
            m_current += l;
            data += l;
            count -= l;
        }
        if( m_current == m_end )
            WriteBlock();
    }
}


void WLByteStream::PutWord( int val )
{
    uchar *current = m_current;

    if( current+1 < m_end )
    {
        current[0] = (uchar)val;
        current[1] = (uchar)(val >> 8);
        m_current = current + 2;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val);
        PutByte(val >> 8);
    }
}


void WLByteStream::PutDWord( int val )
{
    uchar *current = m_current;

    if( current+3 < m_end )
    {
        current[0] = (uchar)val;
        current[1] = (uchar)(val >> 8);
        current[2] = (uchar)(val >> 16);
        current[3] = (uchar)(val >> 24);
        m_current = current + 4;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val);
        PutByte(val >> 8);
        PutByte(val >> 16);
        PutByte(val >> 24);
    }
}


///////////////////////////// WMByteStream /////////////////////////////////// 

WMByteStream::~WMByteStream()
{
}


void WMByteStream::PutWord( int val )
{
    uchar *current = m_current;

    if( current+1 < m_end )
    {
        current[0] = (uchar)(val >> 8);
        current[1] = (uchar)val;
        m_current = current + 2;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val >> 8);
        PutByte(val);
    }
}


void WMByteStream::PutDWord( int val )
{
    uchar *current = m_current;

    if( current+3 < m_end )
    {
        current[0] = (uchar)(val >> 24);
        current[1] = (uchar)(val >> 16);
        current[2] = (uchar)(val >> 8);
        current[3] = (uchar)val;
        m_current = current + 4;
        if( m_current == m_end )
            WriteBlock();
    }
    else
    {
        PutByte(val >> 24);
        PutByte(val >> 16);
        PutByte(val >> 8);
        PutByte(val);
    }
}


///////////////////////////// WMBitStream /////////////////////////////////// 

WMBitStream::WMBitStream()
{
    m_pad_val = 0;
    ResetBuffer();
}


WMBitStream::~WMBitStream()
{
}


bool  WMBitStream::Open( const char* filename )
{
    ResetBuffer();
    return WBaseStream::Open( filename );
}


void  WMBitStream::ResetBuffer()
{
    m_val = 0;
    m_bit_idx = 32;
    m_current = m_start;
}

void  WMBitStream::Flush()
{
    if( m_bit_idx < 32 )
    {
        Put( m_pad_val, m_bit_idx & 7 );
        *((ulong*&)m_current)++ = m_val;
    }
}


void  WMBitStream::Close()
{
    if( m_is_opened )
    {
        Flush();
        WBaseStream::Close();
    }
}


void  WMBitStream::WriteBlock()
{
    if( !bsIsBigEndian() )
        bsBSwapBlock( m_start, m_current );
    WBaseStream::WriteBlock();
}


int  WMBitStream::GetPos()
{
    return WBaseStream::GetPos() + ((32 - m_bit_idx) >> 3);
}


void  WMBitStream::Put( int val, int bits )
{
    int  bit_idx = m_bit_idx - bits;
    ulong  curval = m_val;

    assert( 0 <= bits && bits < 32 );

    val &= bs_bit_mask[bits];

    if( bit_idx >= 0 )
    {
        curval |= val << bit_idx;
    }
    else
    {
        *((ulong*&)m_current)++ = curval | ((unsigned)val >> -bit_idx);
        if( m_current >= m_end )
        {
            WriteBlock();
        }
        bit_idx += 32;
        curval = val << bit_idx;
    }

    m_val = curval;
    m_bit_idx = bit_idx;
}


void  WMBitStream::PutHuff( int val, const ulong* table )
{
    int min_val = (int)table[0];
    val -= min_val;
    
    assert( (unsigned)val < table[1] );

    ulong code = table[val + 2];
    assert( code != 0 );
    
    Put( code >> 8, code & 255 );
}


bool bsCreateEncodeHuffmanTable( const int* src, ulong* table, int max_size )
{   
    int  i, k;
    int  min_val = INT_MAX, max_val = INT_MIN;
    int  size;
    
    /* calc min and max values in the table */
    for( i = 1, k = 1; src[k] >= 0; i++ )
    {
        int code_count = src[k++];

        for( code_count += k; k < code_count; k++ )
        {
            int  val = src[k] >> huff_val_shift;
            if( val < min_val )
                min_val = val;
            if( val > max_val )
                max_val = val;
        }
    }

    size = max_val - min_val + 3;

    if( size > max_size )
    {
        assert(0);
        return false;
    }

    memset( table, 0, size*sizeof(table[0]));

    table[0] = min_val;
    table[1] = size - 2;

    for( i = 1, k = 1; src[k] >= 0; i++ )
    {
        int code_count = src[k++];

        for( code_count += k; k < code_count; k++ )
        {
            int  val = src[k] >> huff_val_shift;
            int  code = src[k] & huff_code_mask;

            table[val - min_val + 2] = (code << 8) | i;
        }
    }
    return true;
}

