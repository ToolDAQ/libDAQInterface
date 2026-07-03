#ifndef RAW_DAQ_HEADER_H
#define RAW_DAQ_HEADER_H

#include <stddef.h>
#include <cstdint>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <limits.h>
#include <bitset>
#include <iostream>

class RAWDAQHeader{

 public:

  RAWDAQHeader();
  RAWDAQHeader(uint32_t* in_data);
  uint32_t GetMessageNum() const;
  uint32_t GetCoarseCounter() const;
  uint8_t GetType() const;
  uint16_t GetCardID() const;
  uint16_t GetNumWords() const;
  uint8_t GetFlags() const;
  void Print() const;

 private:

  uint32_t* data;


};

class DAQHeader{

 public:

  DAQHeader();
  DAQHeader(uint32_t coarse_counter, uint32_t msg_num);
  uint32_t GetMessageNum() const;
  uint32_t GetCoarseCounter() const;
  uint8_t GetType() const;
  uint16_t GetCardID() const;
  uint16_t GetNumWords() const;
  uint8_t GetFlags() const;
  uint32_t* GetData();

  void SetMessageNum(uint32_t in);
  void SetCoarseCounter(uint32_t in);
  void SetType(uint8_t in);
  void SetCardID(uint16_t in);
  void SetNumWords(uint16_t in);
  void SetFlags(uint8_t in);

  void Print() const;
  size_t Size();

 private:

  uint32_t data[3];


};




#endif
