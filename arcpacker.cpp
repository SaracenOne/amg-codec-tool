/*
  This file is a part of the
  Amagami SS PS2 Translation Project

  You may feel free to use this code if you want so.
  I provide no guarantee that this code is working,
  useful or anything else. Use it at your own risk.
*/

#include "arcpacker.h"

ArcPacker::ArcPacker() : Module("ArcPacker")
{
}

inline void ArcPacker::initEncoder()
{
    counter = 0;
    encBufPos = 0;
    encBufLast = 0;
    bytesEncoded = 0;
    bufPos = BUF_START_POSITION;

    memset((void*)buf, 0x00, BUF_SZ);
}

inline void ArcPacker::initDecoder()
{
    bufPos = BUF_START_POSITION;

    memset((void*)buf, 0x00, BUF_SZ);
}


inline void ArcPacker::putFileSize( QFile &file, quint32 size)
{
    file.write((char*)&size, sizeof(quint32));
}

inline int ArcPacker::refillBuffer(QFile &inFile)
{
    // TODO: optimize
    // inFile.map(inFile.pos(), inFile.size());
    while((encBufLast - encBufPos < BUF_REFILL_AMOUNT) && !inFile.atEnd())
    {
        inFile.read((char*)(encBuf + (encBufLast % ENC_BUF_SZ)), 1);
        ++encBufLast;
    }

    if((encBufPos == encBufLast) && inFile.atEnd())
    {
        return REFILL_FAIL;
    }

    return REFILL_OK;
}

inline int ArcPacker::openFiles(QFile &input, QFile &output)
{
    if(!input.open(QIODevice::ReadOnly))
    {
        fprintf(stderr, "Failed to open input file!\n");
        return 1;
    }
    if(!output.open(QIODevice::WriteOnly))
    {
        fprintf(stderr, "Failed to open output file!\n");
        return 1;
    }

    return 0;
}

inline quint8 ArcPacker::getBufItem(size_t index)
{
    return buf[index % BUF_SZ];
}

inline void ArcPacker::setBufItem(size_t index, quint8 value)
{
    buf[index % BUF_SZ] = value;
}

inline quint8 ArcPacker::getEncBufItem(size_t index)
{
    return encBuf[index % ENC_BUF_SZ];
}

inline void ArcPacker::scanBuffer(size_t &newBufPos, size_t &newEncBufPos, size_t &matchedLength)
{
    while((getBufItem(newBufPos) == getEncBufItem(newEncBufPos))
          && (matchedLength < MAX_CHUNK_LENGTH))
    {
        newBufPos++;
        newEncBufPos++;
        matchedLength++;

        if((newBufPos % BUF_SZ) == (bufPos % BUF_SZ))
        {
            size_t tmpEncBufPos = encBufPos;
            while((getEncBufItem(newEncBufPos) == getEncBufItem(tmpEncBufPos))
                  && (matchedLength < MAX_CHUNK_LENGTH))
            {
                newEncBufPos++;
                tmpEncBufPos++;
                matchedLength++;
            }
            return;
        }
    }
}

inline int ArcPacker::findBestMatch( match_info_t &matchInfo, size_t blockSize)
{
    bool found = false;
    size_t maxMatchedLength = 0;
    size_t maxChunkStart = 0;
    quint8 chunkStart = getEncBufItem(encBufPos);

    matchInfo.chunkStart = chunkStart;

    HashManager::HashEntry * list = hashManager.FindByValue(chunkStart);

    while(list)
    {
        size_t pos = list->offset;

        size_t newPos = pos + 1;
        size_t newReadBufPos = encBufPos + 1;
        size_t matchedLength = 1;

        scanBuffer(newPos, newReadBufPos, matchedLength);

        if((matchedLength >= MIN_MATCH_LENGTH) && (matchedLength > maxMatchedLength))
        {
            bool ok = true;

            size_t posStart = (bufPos - 2) % BUF_SZ;
            size_t posFinish = (bufPos + blockSize + maxMatchedLength) % BUF_SZ;

            assert(posStart != posFinish);

            if(posStart > posFinish)
            {
                if((pos <= posFinish) || (pos > posStart))
                {
                    ok = false;
                }
            }
            else
            {
                if((pos <= posFinish) && (pos > posStart))
                {
                    ok = false;
                }
            }

            if(ok)
            {
                maxMatchedLength = matchedLength;
                maxChunkStart = pos;
                found = true;
            }
        }

        list = list->next;
    }

    if(found && (maxMatchedLength) >= MIN_MATCH_LENGTH)
    {
        matchInfo.valid = true;
        matchInfo.maxChunkStart = maxChunkStart;
        matchInfo.maxMatchedLength = maxMatchedLength;
    }
    else
    {
        matchInfo.valid = false;
    }

    return 0;
}

inline quint16 ArcPacker::makeRef(size_t maxChunkStart, size_t maxMatchedLength)
{
    quint16 ref = 0;

    ref |= (maxChunkStart & 0xFF);
    ref |= (((maxMatchedLength - 3) << 8) & 0x1F00);
    ref |= (((maxChunkStart) << 5) & 0xE000);

    return ref;
}

inline int ArcPacker::flushToBuffer(size_t length)
{
    hashManager.ClearByOffset(bufPos, length);
    for(size_t i = 0; i < length; ++i)
    {
        quint8 value = getEncBufItem(encBufPos);
        buf[bufPos % BUF_SZ] = value;

        //fprintf(stderr, "GLOB add 0x%x to offset %d\n", value, bufPos % BUF_SZ);
        hashManager.AddEntry(value, bufPos % BUF_SZ);

        ++bufPos;
        ++encBufPos;
    }

    return 0;
}

inline int ArcPacker::fillBlockEntry(packed_block_t &block, size_t index, match_info_t &match)
{
    if(match.valid)
    {
        size_t maxChunkStart = match.maxChunkStart;
        size_t maxMatchedLength = match.maxMatchedLength;

        block.entry[index].isRef = true;
        block.entry[index].ref =  makeRef(maxChunkStart, maxMatchedLength);
        block.entry[index].start = maxChunkStart;
        block.entry[index].length = maxMatchedLength;

        flushToBuffer(maxMatchedLength);

        bytesEncoded += maxMatchedLength;

        return maxMatchedLength;
    }
    else
    {
        size_t chunkStart = match.chunkStart;

        block.header |= 1 << 7;
        block.entry[index].isRef = false;
        block.entry[index].value = chunkStart;
        block.entry[index].start = bufPos % BUF_SZ;
        block.entry[index].length = 1;

        flushToBuffer(1);

        bytesEncoded += 1;

        return 1;
    }
}

int ArcPacker::dumpToFile(QFile &outFile, packed_block_t &block, int maxEntry = 8)
{
    outFile.write((char*)&block.header, sizeof(quint8));

    for(int i = 0; i < maxEntry; i++)
    {
        if(block.entry[i].isRef)
        {
            outFile.write((char*)&block.entry[i].ref, sizeof(quint16));
        }
        else
        {
            outFile.write((char*)&block.entry[i].value, sizeof(quint8));
        }
    }

    return 0;
}

int ArcPacker::pack(QString inFilePath, QString outFilePath)
{
    int i;
    packed_block_t block;

    QFile inputFile(inFilePath);
    QFile outputFile(outFilePath);

    if(openFiles(inputFile, outputFile))
    {
        return 1;
    }

    putFileSize(outputFile, inputFile.size());
    initEncoder();

    bool stop = false;
    while(!stop)
    {
        size_t blockSize = 0;
        block.header = 0x00;

        for(i = 0; i < 8; i++)
        {
            match_info_t match;
            block.header >>= 1;

            if(refillBuffer(inputFile) == REFILL_FAIL)
            {
                stop = true;
                for(int j = i ; j < 8; j++)
                {
                    if( j != i)
                    {
                        block.header >>= 1;
                    }

                    match.chunkStart = 0;
                    match.valid = false;

                    blockSize += fillBlockEntry(block, j, match);
                }
                fprintf(stderr, "Stopped\n");
                break;
            }

            findBestMatch(match, blockSize);
            blockSize += fillBlockEntry(block, i, match);
#if 0
            // Debug dumps
            fprintf(stderr, "Block [%d] :\n", i);
            fprintf(stderr, "isRef  : %d\n", block.entry[i].isRef);
            fprintf(stderr, "ref    : %x\n", block.entry[i].ref);
            fprintf(stderr, "value  : %x\n", block.entry[i].value);
            fprintf(stderr, "start  : %x\n", block.entry[i].start);
            fprintf(stderr, "length : %d\n", block.entry[i].length);
#endif
        }

        if(stop)
        {
            dumpToFile(outputFile, block, i);
            break;
        }

        dumpToFile(outputFile, block);

        ++counter;

        if((counter % 10000) == 0)
        {
            fprintf(stderr, "Processed: %d blocks\n", counter);
            //getchar();
            //break;
        }
    }

    // -- Loop end
    inputFile.close();
    outputFile.close();

    return 0;
}

int ArcPacker::unpack(QString inFilePath, QString outFilePath)
{
    QFile inFile(inFilePath);
    QFile outFile(outFilePath);

    inFile.open(QIODevice::ReadOnly);
    outFile.open(QIODevice::WriteOnly);

    initDecoder();

    quint32 unpackedSize = 0;
    inFile.read((char*)&unpackedSize, sizeof(quint32));

    size_t bytesWritten = 0;
    size_t bytesRead = 4;

    quint8 blockHeader;

    for(size_t iter = 0; !inFile.atEnd(); iter++)
    {
        inFile.read((char*)&blockHeader, sizeof(quint8));
        ++bytesRead;

        for(size_t i = 0; i < 8; i++)
        {
            if(blockHeader & 0x1)
            {
                inFile.read((char*)(buf + bufPos), sizeof(quint8));
                outFile.write((char*)(buf + bufPos), sizeof(quint8));

                ++bytesRead;
                ++bytesWritten;
                ++bufPos;
            }
            else
            {
                quint16 reference;
                inFile.read((char*)&reference, sizeof(quint16));
                bytesRead += 2;

                quint8 length = ((reference >> 8) & 0x1F) + 3;
                quint16 start = reference & 0xFF;
                size_t section = (reference >> 13) & 0x7;

                for(int j = 0; j < length; j++)
                {
                    quint8 value = getBufItem(start + j + section * SECTION_SZ);

                    setBufItem(bufPos + j, value);
                    outFile.write((char*)&value, sizeof(quint8));

                    ++bytesWritten;
                }

                bufPos += length;
            }

            /* Block can be terminated by global limit */
            if(bytesWritten == unpackedSize)
            {
                break;
            }

            bufPos = bufPos % BUF_SZ;
            blockHeader >>= 1;
        }
    }

    fprintf(stderr, "Bytes written: 0x%x\n", bytesWritten);
    fprintf(stderr, "Bytes expected: 0x%x\n", unpackedSize);

    assert( bytesWritten == unpackedSize );

    // -- Finalize
    inFile.close();
    outFile.close();

    return 0;
}
