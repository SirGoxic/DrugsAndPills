#include "png_reader.h"
#include <cstdint>
#include <fstream>
#include <vector>

struct PNGHeader {
   uint32_t width;
   uint32_t height;
   uint8_t bitDepth;
   uint8_t colorType;
   uint8_t compMethod;
   uint8_t filterMethod;
   uint8_t interlaceMethod;
   uint8_t bbp;
};

struct BitReader {
   char* data = nullptr;
   size_t length = 0;
   size_t index = 0;
   char posInByte = 0;
   std::vector<char> buffer;

   uint32_t ToUInt32() {
      uint32_t result = 0;

      for (int i = 0; i < buffer.size(); i++) {
         if (buffer[i]) {
            result += 1 << (buffer.size() - 1 - i);
         }
      }

      return result;
   }

   uint32_t ToUInt32Reverse() {
      uint32_t result = 0;

      for (int i = 0; i < buffer.size(); i++) {
         if (buffer[i]) {
            result += 1 << i;
         }
      }

      return result;
   }

   bool TryReadBits(int count) {
      for (int i = 0; i < count; i++) {
         if (index >= length) {
            return false;
         }

         buffer.push_back((data[index] & (1 << posInByte)) >> posInByte);

         posInByte++;
         if (posInByte > 7) {
            posInByte = 0;
            index++;
         }
      }

      return true;
   }

   void Clear() {
      buffer.clear();
   }

   bool InRange() {
      return index >= 0 && index < length;
   }
};

enum class CompType {
   NONE = 0,
   FIXED = 1,
   DYNAMIC = 2,
   ERROR_TYPE = 3
};

struct Symbol {
   uint32_t code;
   uint32_t len;
};

struct Alphabet {

   Symbol* syms = nullptr;
   size_t symCount = 0;
   bool isFilled = false;

   Alphabet(size_t symCount) {
      syms = new Symbol[symCount]{0};
      this->symCount = symCount;
   }

   ~Alphabet() {
      delete[] syms;
   }

   int TryGetSym(BitReader& reader) {
      uint32_t len = (uint32_t) reader.buffer.size();
      uint32_t code = reader.ToUInt32();
      for (int i = 0; i < symCount; i++) {
         if (syms[i].len == len && syms[i].code == code) {
            return i;
         }
      }

      return -1;
   }
};

static const char PNG[] = {(char) 137, 80, 78, 71, 13, 10, 26, 10};
static const char IHDR[] = {'I', 'H', 'D', 'R'};
static const char IDAT[] = {'I', 'D', 'A', 'T'};
static const char IEND[] = {'I', 'E', 'N', 'D'};

static uint32_t indexToCodeLengthSymMap[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
static uint32_t lengthTable[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static uint32_t lengthExtraBitTable[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static uint32_t distTable[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
static uint32_t distExtraBitTable[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

static Alphabet fixedLitLen(288);
static Alphabet fixedDist(32);

bool NotSameBuffer(const char* first, const char* second, size_t count, size_t firstOffset = 0, size_t secondOffset = 0);
uint32_t ToUInt32(const char* buffer, size_t offset = 0);

PNGHeader TryReadHeader(std::ifstream& file);

size_t TryReadData(std::ifstream& file, char** out);

void DecompressData(char* data, size_t dataLength, std::vector<uint8_t>& filteredData);
void FillFixedHuffman();
void FillDynamicHuffman(BitReader& reader, Alphabet** litLenAlphabet, Alphabet** distAlphabet);

void DefilterImage(std::vector<uint8_t>& filteredData, std::vector<uint8_t>& resultData, uint32_t width, uint32_t height, uint8_t bbp);
unsigned char PaethPredictor(uint8_t a, uint8_t b, uint8_t c);

HBITMAP LoadPNG(const wchar_t* fileName, HDC hdc, const COLORREF transparencyColor) {
   std::ifstream file(fileName, std::ios::binary);

   if (!file.is_open()) {
      return nullptr;
   }

   char startBuffer[8];

   if (!file.read(startBuffer, 8)) {
      return nullptr;
   }

   if (NotSameBuffer(startBuffer, PNG, 8)) {
      return nullptr;
   }

   PNGHeader header = TryReadHeader(file);
   if (!header.width) {
      return nullptr;
   }

   file.seekg(4, std::ios_base::cur);//Skip CRC

   char* data = nullptr;
   size_t dataLength = TryReadData(file, &data);
   if (!dataLength || !data) {
      delete[] data;
      return nullptr;
   }

   //First 4 bytes - CM, should be equal 8 for PNG
   //Last 4 bytes - CINFO, should be less than 7 for PNG
   if ((data[0] & 15) != 8 || ((data[0] & 240) >> 4) > 7) {
      delete[] data;
      return nullptr;
   }

   //5th bit flag for preset dictionary, unsupported
   if (data[1] & 32) {
      delete[] data;
      return nullptr;
   }

   std::vector<uint8_t> filteredData;
   uint32_t filteredSize = header.height * (header.width * header.bbp + 1);
   filteredData.reserve(filteredSize);

   DecompressData(data, dataLength, filteredData);

   delete[] data;

   if (filteredData.size() != filteredSize) {
      return nullptr;
   }

   std::vector<uint8_t> imageData;
   uint32_t imageSize = header.height * header.width * header.bbp;
   imageData.reserve(imageSize);
   DefilterImage(filteredData, imageData, header.width, header.height, header.bbp);

   if (imageData.size() != imageSize) {
      return nullptr;
   }

   BITMAPINFO bmi = {};
   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth = header.width;
   bmi.bmiHeader.biHeight = -(int64_t) (header.height);
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;

   uint32_t* pixels;
   HBITMAP image = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels), nullptr, 0);

   for (uint32_t line = 0; line < header.height; line++) {
      uint32_t lineShift = line * header.width;
      for (uint32_t column = 0; column < header.width; column++) {
         uint32_t dataIndex = line * (header.width * header.bbp) + (column * header.bbp);
         if (imageData[dataIndex + header.bbp - 1] < 128) {
            pixels[lineShift + column] = ((transparencyColor & 0x000000FF) << 16) + (transparencyColor & 0x0000FF00) + ((transparencyColor & 0x00FF0000) >> 16);
         } else {
            pixels[lineShift + column] = (uint32_t) imageData[dataIndex + 2] + (((uint32_t) imageData[dataIndex + 1]) << 8) + (((uint32_t) imageData[dataIndex]) << 16);
         }
      }
   }

   return image;
}

bool NotSameBuffer(const char* first, const char* second, size_t count, size_t firstOffset, size_t secondOffset) {
   for (int i = 0; i < count; i++) {
      if (first[firstOffset + i] != second[secondOffset + i]) {
         return true;
      }
   }

   return false;
}

uint32_t ToUInt32(const char* buffer, size_t offset) {
   return ((uint32_t) ((uint8_t) buffer[offset]) << 24) + ((uint32_t) ((uint8_t) buffer[offset + 1]) << 16) + ((uint32_t) ((uint8_t) buffer[offset + 2]) << 8) + (uint8_t) buffer[offset + 3];
}

PNGHeader TryReadHeader(std::ifstream& file) {
   char buffer[21];//4 bytes chunk length, 4 bytes chunk type, 13 bytes IHDR data
   if (!file.read(buffer, 21)) {
      return {};
   }

   uint32_t length = ToUInt32(buffer);
   if (length != 13) {
      return {};
   }

   if (NotSameBuffer(buffer, IHDR, 4, 4)) {
      return {};
   }

   uint32_t width = ToUInt32(buffer, 8);
   uint32_t height = ToUInt32(buffer, 12);
   uint8_t bitDepth = (uint8_t) buffer[16];
   uint8_t colorType = (uint8_t) buffer[17];
   uint8_t compMethod = (uint8_t) buffer[18];
   uint8_t filterMethod = (uint8_t) buffer[19];
   uint8_t interlaceMethod = (uint8_t) buffer[20];

   if (!width || !height) {
      return {};
   }

   if (colorType != 2 && colorType != 6) {//Supported only RGB and RGBA color types
      return {};
   }

   if (bitDepth != 8) {//Only 32 bits color
      return {};
   }

   if (compMethod || filterMethod) {
      return {};
   }

   if (interlaceMethod) {//Interlace is unsupported
      return {};
   }

   uint8_t bbp = colorType == 2 ? 3 : 4;//3 bytes for RGB and 4 bytes for RGBA

   return PNGHeader{width, height, bitDepth, colorType, compMethod, filterMethod, interlaceMethod, bbp};
}

size_t TryReadData(std::ifstream& file, char** out) {
   char* data = nullptr;
   char buffer[4];
   size_t dataLength = 0;

   while (file.read(buffer, 4)) {
      uint32_t chunkLength = ToUInt32(buffer);
      if (!file.read(buffer, 4)) {
         delete[] data;
         return 0;
      }

      if (NotSameBuffer(buffer, IDAT, 4)) {
         if (NotSameBuffer(buffer, IEND, 4)) {
            file.seekg(chunkLength + 4, std::ios_base::cur);//Skip chunk and CRC
            continue;
         } else {
            break;
         }
      }

      char* tempBuffer = new char[chunkLength];
      if (!file.read(tempBuffer, chunkLength)) {
         delete[] tempBuffer;
         delete[] data;
         return 0;
      }

      if (data) {
         char* prevData = data;
         size_t prevDataLength = dataLength;
         dataLength += chunkLength;

         data = new char[dataLength];
         memcpy_s(data, dataLength, prevData, prevDataLength);
         delete[] prevData;

         memcpy_s(&data[prevDataLength], dataLength, tempBuffer, chunkLength);
      } else {
         data = new char[chunkLength];
         dataLength = chunkLength;
         memcpy_s(data, dataLength, tempBuffer, chunkLength);
      }

      delete[] tempBuffer;

      file.seekg(4, std::ios_base::cur);//Skip CRC
   }

   *out = data;
   return dataLength;
}

void DecompressData(char* data, size_t dataLength, std::vector<uint8_t>& filteredData) {
   BitReader reader;
   reader.data = data;
   reader.length = dataLength;
   reader.index = 2;

   bool endBlock = false;
   while (!endBlock && reader.InRange()) {
      reader.TryReadBits(1);
      endBlock = reader.ToUInt32Reverse();
      reader.Clear();

      reader.TryReadBits(2);
      CompType compressionType = (CompType) reader.ToUInt32Reverse();
      reader.Clear();

      if (compressionType == CompType::NONE) {
         reader.index++;
         reader.posInByte = 0;

         reader.TryReadBits(16);//Read LEN
         uint32_t len = reader.ToUInt32();
         reader.Clear();
         reader.index += 2;//Skip NLEN

         for (size_t i = reader.index; i < reader.index + len; i++) {
            filteredData.emplace_back(data[i]);
         }

         reader.index += len;
      } else {
         Alphabet* litLenAlphabet = nullptr;
         Alphabet* distAlphabet = nullptr;
         if (compressionType == CompType::FIXED) {
            FillFixedHuffman();

            litLenAlphabet = &fixedLitLen;
            distAlphabet = &fixedDist;
         } else if (compressionType == CompType::DYNAMIC) {
            FillDynamicHuffman(reader, &litLenAlphabet, &distAlphabet);
         } else {
            return;
         }

         if (!litLenAlphabet || !distAlphabet) {
            delete litLenAlphabet;
            delete distAlphabet;
            return;
         }

         uint32_t lengthToCopy = -1;
         bool readLitLen = true;

         while (reader.InRange()) {
            reader.TryReadBits(1);
            int sym;

            if (readLitLen) {
               sym = litLenAlphabet->TryGetSym(reader);
            } else {
               sym = distAlphabet->TryGetSym(reader);
            }

            if (sym < 0) {
               continue;
            }

            if (readLitLen) {
               if (sym <= 255) {
                  filteredData.emplace_back(sym);
               } else if (sym == 256) {
                  break;
               } else {
                  uint32_t lengthIndex = sym - 257;
                  lengthToCopy = lengthTable[lengthIndex];
                  reader.Clear();

                  uint32_t extra = lengthExtraBitTable[lengthIndex];
                  if (extra) {
                     reader.TryReadBits(extra);
                     lengthToCopy += reader.ToUInt32Reverse();
                  }

                  readLitLen = false;
               }
            } else {
               uint32_t dist = distTable[sym];
               reader.Clear();

               uint32_t extra = distExtraBitTable[sym];
               if (extra) {
                  reader.TryReadBits(extra);
                  dist += reader.ToUInt32Reverse();
               }

               size_t size = filteredData.size();
               for (uint32_t i = 0; i < lengthToCopy; i++) {
                  filteredData.emplace_back(filteredData[size - dist + i]);
               }

               lengthToCopy = -1;
               readLitLen = true;
            }

            reader.Clear();
         }

         reader.Clear();

         if (compressionType == CompType::DYNAMIC) {
            delete litLenAlphabet;
            delete distAlphabet;
         }
      }
   }
}

void FillFixedHuffman() {
   if (fixedLitLen.isFilled && fixedDist.isFilled) {
      return;
   }

   for (int i = 0; i <= 143; i++) {
      fixedLitLen.syms[i].len = 8;
   }

   for (int i = 144; i <= 255; i++) {
      fixedLitLen.syms[i].len = 9;
   }

   for (int i = 256; i <= 279; i++) {
      fixedLitLen.syms[i].len = 7;
   }

   for (int i = 280; i <= 287; i++) {
      fixedLitLen.syms[i].len = 8;
   }

   int lengthCount[10]{0};
   for (int i = 0; i < 288; i++) {
      lengthCount[fixedLitLen.syms[i].len]++;
   }

   int nextCode[10]{0};
   int code = 0;
   for (int bits = 1; bits <= 9; bits++) {
      code = (code + lengthCount[bits - 1]) << 1;
      nextCode[bits] = code;
   }

   for (int i = 0; i < 288; i++) {
      int len = fixedLitLen.syms[i].len;
      fixedLitLen.syms[i].code = nextCode[len];
      nextCode[len]++;
   }

   for (int i = 0; i < 32; i++) {
      fixedDist.syms[i].len = 5;
      fixedDist.syms[i].code = i;
   }

   fixedLitLen.isFilled = true;
   fixedDist.isFilled = true;
}

void FillDynamicHuffman(BitReader& reader, Alphabet** litLenAlphabet, Alphabet** distAlphabet) {
   uint32_t hlit, hdist, hclen;
   hlit = hdist = hclen = 0;

   reader.TryReadBits(5);
   hlit = reader.ToUInt32Reverse() + 257;
   reader.Clear();

   reader.TryReadBits(5);
   hdist = reader.ToUInt32Reverse() + 1;
   reader.Clear();

   reader.TryReadBits(4);
   hclen = reader.ToUInt32Reverse() + 4;
   reader.Clear();

   Alphabet lengthAlphabet(19);
   for (uint32_t i = 0; i < hclen; i++) {
      reader.TryReadBits(3);
      uint32_t len = reader.ToUInt32Reverse();
      reader.Clear();

      lengthAlphabet.syms[indexToCodeLengthSymMap[i]].len = len;
   }

   uint32_t lengthLengthCount[8]{0};
   for (int i = 0; i < lengthAlphabet.symCount; i++) {
      uint32_t len = lengthAlphabet.syms[i].len;
      if (len) {
         lengthLengthCount[len]++;
      }
   }

   uint32_t lengthNextCode[8]{0};
   uint32_t code = 0;
   for (int bits = 1; bits <= 7; bits++) {
      code = (code + lengthLengthCount[bits - 1]) << 1;
      lengthNextCode[bits] = code;
   }

   for (int i = 0; i < lengthAlphabet.symCount; i++) {
      uint32_t len = lengthAlphabet.syms[i].len;
      if (len) {
         lengthAlphabet.syms[i].code = lengthNextCode[len];
         lengthNextCode[len]++;
      }
   }

   size_t totalCount = hlit + hdist;
   Alphabet tempAlphabet(totalCount);
   size_t symCount = 0;

   while (symCount < totalCount && reader.InRange()) {
      reader.TryReadBits(1);
      int sym = lengthAlphabet.TryGetSym(reader);
      if (sym < 0) {
         continue;
      }

      if (sym <= 15) {
         tempAlphabet.syms[symCount].len = sym;
         symCount++;
      } else if (sym == 16) {
         if (symCount - 1 < 0) {
            return;
         }

         reader.Clear();

         reader.TryReadBits(2);
         uint32_t copyLength = reader.ToUInt32Reverse() + 3;
         uint32_t lenToCopy = tempAlphabet.syms[symCount - 1].len;

         if (symCount + copyLength - 1 >= totalCount) {
            return;
         }

         for (uint32_t i = 0; i < copyLength; i++) {
            tempAlphabet.syms[symCount + i].len = lenToCopy;
         }
         symCount += copyLength;
      } else if (sym == 17) {
         reader.Clear();

         reader.TryReadBits(3);
         uint32_t zeroLength = reader.ToUInt32Reverse() + 3;

         if (symCount + zeroLength - 1 >= totalCount) {
            return;
         }

         for (uint32_t i = 0; i < zeroLength; i++) {
            tempAlphabet.syms[symCount + i].len = 0;
         }
         symCount += zeroLength;
      } else if (sym == 18) {
         reader.Clear();

         reader.TryReadBits(7);
         uint32_t zeroLength = reader.ToUInt32Reverse() + 11;

         if (symCount + zeroLength - 1 >= totalCount) {
            return;
         }

         for (uint32_t i = 0; i < zeroLength; i++) {
            tempAlphabet.syms[symCount + i].len = 0;
         }
         symCount += zeroLength;
      } else {
         return;
      }

      reader.Clear();
   }

   if (symCount != totalCount) {
      return;
   }

   Alphabet* lAlphabet = new Alphabet(hlit);
   for (uint32_t i = 0; i < hlit; i++) {
      lAlphabet->syms[i].len = tempAlphabet.syms[i].len;
   }

   int lAlphabetLengthCount[16]{0};
   for (uint32_t i = 0; i < lAlphabet->symCount; i++) {
      uint32_t len = lAlphabet->syms[i].len;
      if (len) {
         lAlphabetLengthCount[len]++;
      }
   }

   int lAlphabetNextCode[16]{0};
   code = 0;
   for (int bits = 1; bits <= 15; bits++) {
      code = (code + lAlphabetLengthCount[bits - 1]) << 1;
      lAlphabetNextCode[bits] = code;
   }

   for (int i = 0; i < lAlphabet->symCount; i++) {
      uint32_t len = lAlphabet->syms[i].len;
      if (len) {
         lAlphabet->syms[i].code = lAlphabetNextCode[len];
         lAlphabetNextCode[len]++;
      }
   }

   Alphabet* dAlphabet = new Alphabet(hdist);
   for (uint32_t i = 0; i < hdist; i++) {
      dAlphabet->syms[i].len = tempAlphabet.syms[hlit + i].len;
   }

   int dAlphabetLengthCount[16]{0};
   for (uint32_t i = 0; i < dAlphabet->symCount; i++) {
      uint32_t len = dAlphabet->syms[i].len;
      if (len) {
         dAlphabetLengthCount[len]++;
      }
   }

   int dAlphabetNextCode[16]{0};
   code = 0;
   for (int bits = 1; bits <= 15; bits++) {
      code = (code + dAlphabetLengthCount[bits - 1]) << 1;
      dAlphabetNextCode[bits] = code;
   }

   for (int i = 0; i < dAlphabet->symCount; i++) {
      uint32_t len = dAlphabet->syms[i].len;
      if (len) {
         dAlphabet->syms[i].code = dAlphabetNextCode[len];
         dAlphabetNextCode[len]++;
      }
   }

   *litLenAlphabet = lAlphabet;
   *distAlphabet = dAlphabet;
}

void DefilterImage(std::vector<uint8_t>& filteredData, std::vector<uint8_t>& resultData, uint32_t width, uint32_t height, uint8_t bbp) {
   for (uint32_t line = 0; line < height; line++) {
      uint32_t filteredLineShift = line * (width * bbp + 1);
      uint32_t lineShift = line * width * bbp;
      uint32_t filterCode = filteredData[filteredLineShift];
      if (filterCode == 0) {
         for (uint32_t column = 0; column < width * bbp; column++) {
            resultData.emplace_back(filteredData[filteredLineShift + column + 1]);
         }
      } else if (filterCode == 1) {
         for (int64_t column = 0; column < width * bbp; column++) {
            if (column - bbp >= 0) {
               resultData.emplace_back(filteredData[filteredLineShift + column + 1] + resultData[lineShift + column - bbp]);
            } else {
               resultData.emplace_back(filteredData[filteredLineShift + column + 1]);
            }
         }
      } else if (filterCode == 2) {
         for (uint32_t column = 0; column < width * bbp; column++) {
            if (line > 0) {
               resultData.emplace_back(filteredData[filteredLineShift + column + 1] + resultData[lineShift - width * bbp + column]);
            } else {
               resultData.emplace_back(filteredData[filteredLineShift + column + 1]);
            }
         }
      } else if (filterCode == 3) {
         for (int64_t column = 0; column < width * bbp; column++) {
            uint8_t byteAbove;
            if (line > 0) {
               byteAbove = resultData[lineShift - width * bbp + column];
            } else {
               byteAbove = 0;
            }

            uint8_t byteLeft;
            if (column - bbp >= 0) {
               byteLeft = resultData[lineShift + column - bbp];
            } else {
               byteLeft = 0;
            }

            resultData.emplace_back(filteredData[filteredLineShift + column + 1] + (byteLeft + byteAbove) / 2);
         }
      } else if (filterCode == 4) {
         for (int64_t column = 0; column < width * bbp; column++) {
            uint8_t byteAbove;
            if (line > 0) {
               byteAbove = resultData[lineShift - width * bbp + column];
            } else {
               byteAbove = 0;
            }

            uint8_t byteLeft;
            if (column - bbp >= 0) {
               byteLeft = resultData[lineShift + column - bbp];
            } else {
               byteLeft = 0;
            }

            uint8_t byteAboveLeft;
            if (line > 0 && column - bbp >= 0) {
               byteAboveLeft = resultData[lineShift - width * bbp + column - bbp];
            } else {
               byteAboveLeft = 0;
            }

            resultData.emplace_back(filteredData[filteredLineShift + column + 1] + PaethPredictor(byteLeft, byteAbove, byteAboveLeft));
         }
      } else {
         return;
      }
   }
}

unsigned char PaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
   int32_t p = (int32_t) a + (int32_t) b - (int32_t) c;
   int32_t pa = abs(p - a);
   int32_t pb = abs(p - b);
   int32_t pc = abs(p - c);

   if (pa <= pb && pa <= pc) {
      return a;
   } else if (pb <= pc) {
      return b;
   }

   return c;
}