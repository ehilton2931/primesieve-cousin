///
/// @file   PrimeSieveClass.hpp
/// @brief  PrimeSieve is a high level class that manages prime
///         sieving. It is used for printing and counting primes
///         and for computing the nth prime.
///
/// Copyright (C) 2026 Kim Walisch, <kim.walisch@gmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#ifndef PRIMESIEVE_CLASS_HPP
#define PRIMESIEVE_CLASS_HPP

#include <primesieve/config.hpp>
#include <primesieve/macros.hpp>
#include <primesieve/Vector.hpp>

#include <stdint.h>
#include <chrono>

namespace primesieve {

enum
{
  COUNTS_INDEX_PRIMES      = 0,
  COUNTS_INDEX_TWINS       = 1,
  COUNTS_INDEX_TRIPLETS    = 2,
  COUNTS_INDEX_QUADRUPLETS = 3,
  COUNTS_INDEX_QUINTUPLETS = 4,
  COUNTS_INDEX_SEXTUPLETS  = 5,
  COUNTS_INDEX_LENGTH      = 6
};

using counts_t = Array<uint64_t, COUNTS_INDEX_LENGTH>;
class ParallelSieve;

enum
{
  COUNT_PRIMES      = 1 << COUNTS_INDEX_PRIMES,
  COUNT_TWINS       = 1 << COUNTS_INDEX_TWINS,
  COUNT_TRIPLETS    = 1 << COUNTS_INDEX_TRIPLETS,
  COUNT_QUADRUPLETS = 1 << COUNTS_INDEX_QUADRUPLETS,
  COUNT_QUINTUPLETS = 1 << COUNTS_INDEX_QUINTUPLETS,
  COUNT_SEXTUPLETS  = 1 << COUNTS_INDEX_SEXTUPLETS,
  PRINT_PRIMES      = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_PRIMES),
  PRINT_TWINS       = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_TWINS),
  PRINT_TRIPLETS    = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_TRIPLETS),
  PRINT_QUADRUPLETS = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_QUADRUPLETS),
  PRINT_QUINTUPLETS = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_QUINTUPLETS),
  PRINT_SEXTUPLETS  = 1 << (COUNTS_INDEX_LENGTH + COUNTS_INDEX_SEXTUPLETS),
  PRINT_STATUS      = 1 << (2 * COUNTS_INDEX_LENGTH)
};

class PrimeSieve
{
public:
  PrimeSieve();
  PrimeSieve(ParallelSieve*);
  virtual ~PrimeSieve() = default;
  // Getters
  uint64_t getStart() const;
  uint64_t getStop() const;
  uint64_t getDistance() const;
  int getSieveSize() const;
  double getSeconds() const;
  // Setters
  void setStart(uint64_t);
  void setStop(uint64_t);
  void updateStatus(uint64_t);
  void setSieveSize(int);
  void setFlags(int);
  void addFlags(int);
  // Bool is*
  bool isCount(int) const;
  bool isCountPrimes() const;
  bool isCountkTuplets() const;
  bool isPrint() const;
  bool isPrint(int) const;
  bool isPrintPrimes() const;
  bool isPrintkTuplets() const;
  bool isFlag(int) const;
  bool isFlag(int, int) const;
  bool isStatus() const;
  // Sieve
  virtual void sieve();
  void sieve(uint64_t, uint64_t);
  void sieve(uint64_t, uint64_t, int);
  // nth prime
  uint64_t nthPrime(uint64_t);
  uint64_t nthPrime(int64_t, uint64_t);
  uint64_t negativeNthPrime(int64_t, uint64_t);
  // Count
  counts_t& getCounts();
  uint64_t getCount(int) const;
  uint64_t countPrimes(uint64_t, uint64_t);

protected:
  void reset();
  void setStatus(double);

  /// Sieve primes >= start_
  uint64_t start_ = 0;
  /// Sieve primes <= stop_
  uint64_t stop_ = 0;
  /// Time elapsed of sieve()
  double seconds_ = 0;
  /// Prime number and prime k-tuplet counts
  counts_t counts_;

private:
  void processSmallPrimes();
  static void printStatus(double, double);

  uint64_t updateDistance_ = 0;
  std::chrono::steady_clock::time_point lastUpdateTime_{};
  /// Default flags
  int flags_ = COUNT_PRIMES;
  /// Sieve size in KiB
  int sieveSize_ = 0;
  /// Status updates must be synchronized by main thread
  ParallelSieve* parent_ = nullptr;
  MAYBE_UNUSED char pad1_[config::MAX_CACHE_LINE_SIZE];
  /// These variables may be updated by multiple
  /// threads. Hence, they are on a separate
  /// cache line to prevent CPU false sharing.
  uint64_t sievedDistance_ = 0;
  double percent_ = 0;
  MAYBE_UNUSED char pad2_[config::MAX_CACHE_LINE_SIZE];
};

} // namespace

#endif
