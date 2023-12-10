// //////////////////////////////////////////////////////////
// mersenne.h
// Copyright (c) 2014 Stephan Brumme. All rights reserved.
// see http://create.stephan-brumme.com/disclaimer.html
//
#pragma once
#include <cstdint>

namespace custom
{

  /// Mersenne twister pseudo-random number generator
  /** algorithm invented by Makoto Matsumoto and Takuji Nishimura **/
  class MersenneTwister
  {
  public:
    /// generate initial internal state
    explicit MersenneTwister(const uint32_t seed = 5489);
    /// return a random 32 bit number
    uint32_t operator()();
    /// return a uniform random float [0..1]
    float uniform_float();
    /// return a uniform random unsigned char [low..high]
    uint8_t uniform_uchar(const uint8_t low, const uint8_t high);

  private:
    /// create new state (based on old one)
    void twist();

    /// state size
    enum
    {
      SizeState = 624,
    };
    /// internal state
    uint32_t state[SizeState];
    /// offset of next state's word
    int next;
  };

}
