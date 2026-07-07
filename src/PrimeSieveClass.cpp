///
/// @file   PrimeSieveClass.cpp
/// @brief  PrimeSieve is a high level class that manages prime
///         sieving. It is used for printing and counting primes
///         and for computing the nth prime.
///
/// Copyright (C) 2026 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include "PrimeSieveClass.hpp"
#include "ParallelSieve.hpp"
#include "CountPrintPrimes.hpp"

#include <primesieve/forward.hpp>
#include <primesieve/pmath.hpp>
#include <primesieve/Vector.hpp>

#include <stdint.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

namespace {

struct SmallPrime
{
  uint64_t first;
  uint64_t last;
  int index;
  const char* str;
};

const primesieve::Array<SmallPrime, 10> smallPrimes
{{
  { 2,  2, primesieve::COUNTS_INDEX_PRIMES, "2" },
  { 3,  3, primesieve::COUNTS_INDEX_PRIMES, "3" },
  { 5,  5, primesieve::COUNTS_INDEX_PRIMES, "5" },
  { 3,  5, primesieve::COUNTS_INDEX_TWINS, "(3, 5)" },
  { 5,  7, primesieve::COUNTS_INDEX_TWINS, "(5, 7)" },
  { 5, 11, primesieve::COUNTS_INDEX_TRIPLETS, "(5, 7, 11)" },
  { 5, 13, primesieve::COUNTS_INDEX_QUADRUPLETS, "(5, 7, 11, 13)" },
  { 5, 17, primesieve::COUNTS_INDEX_QUINTUPLETS, "(5, 7, 11, 13, 17)" },
  { 3,  7, primesieve::COUNTS_INDEX_COUSINS, "(3, 7)" },
  { 5, 11, primesieve::COUNTS_INDEX_SEXYS, "(5, 11)" }
}};

} // namespace

namespace primesieve {

PrimeSieve::PrimeSieve()
{
  int sieveSize = get_sieve_size();
  setSieveSize(sieveSize);
}

/// Used for multi-threading
PrimeSieve::PrimeSieve(ParallelSieve* parent) :
  flags_(parent->flags_),
  sieveSize_(parent->sieveSize_),
  parent_(parent)
{ }

void PrimeSieve::reset()
{
  counts_.fill(0);
  percent_ = -1.0;
  seconds_ = 0.0;
  sievedDistance_ = 0;
}

bool PrimeSieve::isFlag(int flag) const
{
  return (flags_ & flag) == flag;
}

bool PrimeSieve::isFlag(int first, int last) const
{
  int mask = (last * 2) - first;
  return (flags_ & mask) != 0;
}

bool PrimeSieve::isCountPrimes() const
{
  return isFlag(COUNT_PRIMES);
}

bool PrimeSieve::isPrintPrimes() const
{
  return isFlag(PRINT_PRIMES);
}

bool PrimeSieve::isPrint() const
{
  return isFlag(PRINT_PRIMES, (PRINT_PRIMES) << (COUNTS_INDEX_LENGTH - 1));
}

bool PrimeSieve::isCountkTuplets() const
{
  return isFlag(COUNT_TWINS, (COUNT_PRIMES) << (COUNTS_INDEX_LENGTH - 1));
}

bool PrimeSieve::isPrintkTuplets() const
{
  return isFlag(PRINT_TWINS, (PRINT_PRIMES) << (COUNTS_INDEX_LENGTH - 1));
}

bool PrimeSieve::isStatus() const
{
  return isFlag(PRINT_STATUS);
}

bool PrimeSieve::isCount(int i) const
{
  return isFlag(COUNT_PRIMES << i);
}

bool PrimeSieve::isPrint(int i) const
{
  return isFlag(PRINT_PRIMES << i);
}

uint64_t PrimeSieve::getStart() const
{
  return start_;
}

uint64_t PrimeSieve::getStop() const
{
  return stop_;
}

uint64_t PrimeSieve::getDistance() const
{
  if (start_ <= stop_)
    return stop_ - start_;
  else
    return 0;
}

uint64_t PrimeSieve::getCount(int i) const
{
  return counts_[i];
}

counts_t& PrimeSieve::getCounts()
{
  return counts_;
}

int PrimeSieve::getSieveSize() const
{
  return sieveSize_;
}

double PrimeSieve::getSeconds() const
{
  return seconds_;
}

void PrimeSieve::setFlags(int flags)
{
  flags_ = flags;
}

void PrimeSieve::addFlags(int flags)
{
  flags_ |= flags;
}

void PrimeSieve::setStart(uint64_t start)
{
  start_ = start;
}

void PrimeSieve::setStop(uint64_t stop)
{
  stop_ = stop;
}

/// Set the size of the sieve array in KiB (kibibyte)
void PrimeSieve::setSieveSize(int sieveSize)
{
  sieveSize_ = inBetween(16, sieveSize, 8192);
}

void PrimeSieve::setStatus(double percent)
{
  if (!parent_)
  {
    auto old = percent_;
    percent_ = percent;
    if (isFlag(PRINT_STATUS))
      printStatus(old, percent_);
  }
}

void PrimeSieve::updateStatus(uint64_t dist)
{
  // This is a worker thread, so we need to send
  // the update status request to the parent
  // object which handles thread synchronization.
  if (parent_)
  {
    updateDistance_ += dist;
    auto time = std::chrono::steady_clock::now();

    // tryUpdateStatus() uses a mutex. This code
    // reduces lock contention: each thread may
    // only try to lock the mutex every 0.03 secs
    if (lastUpdateTime_ != std::chrono::steady_clock::time_point{} &&
        time - lastUpdateTime_ < std::chrono::milliseconds(30))
      return;

    lastUpdateTime_ = time;

    if (parent_->tryUpdateStatus(updateDistance_))
      updateDistance_ = 0;
  }
  else
  {
    sievedDistance_ += dist;
    double percent = 100;

    if (getDistance() > 0)
      percent = sievedDistance_ * 100.0 / getDistance();

    double old = percent_;
    percent_ = std::min(percent, 100.0);

    if (isFlag(PRINT_STATUS))
      printStatus(old, percent_);
  }
}

void PrimeSieve::printStatus(double old, double current)
{
  int percent = (int) current;
  if (percent > (int) old)
  {
    std::string precentStr = '\r' + std::to_string(percent) + '%';
    std::cout << precentStr << std::flush;

    if (percent == 100)
      std::cout << '\n';
  }
}

/// Process small primes <= 5
/// and small k-tuplets with (at least) one prime <= 5
void PrimeSieve::processSmallPrimes()
{
  for (auto& p : smallPrimes)
  {
    if (p.first >= start_ && p.last <= stop_)
    {
      if (isCount(p.index))
        counts_[p.index]++;
      if (isPrint(p.index))
        std::cout << p.str << '\n';
    }
  }
}

uint64_t PrimeSieve::countPrimes(uint64_t start, uint64_t stop)
{
  sieve(start, stop, COUNT_PRIMES);
  return getCount(COUNTS_INDEX_PRIMES);
}

void PrimeSieve::sieve(uint64_t start, uint64_t stop)
{
  setStart(start);
  setStop(stop);
  sieve();
}

void PrimeSieve::sieve(uint64_t start, uint64_t stop, int flags)
{
  setStart(start);
  setStop(stop);
  setFlags(flags);
  sieve();
}

/// Sieve the primes and prime k-tuplets (twin primes,
/// prime triplets, ...) in [start, stop].
///
void PrimeSieve::sieve()
{
  reset();

  if (start_ > stop_)
    return;

  setStatus(0);
  auto t1 = std::chrono::steady_clock::now();

  if (start_ <= 5)
    processSmallPrimes();

  if (stop_ >= 7)
  {
    CountPrintPrimes countPrintPrimes(*this);
    countPrintPrimes.sieve();
  }

  auto t2 = std::chrono::steady_clock::now();
  std::chrono::duration<double> seconds = t2 - t1;
  seconds_ = seconds.count();
  setStatus(100);
}

} // namespace
