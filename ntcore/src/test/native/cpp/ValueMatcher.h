// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#ifndef NTCORE_VALUEMATCHER_H_
#define NTCORE_VALUEMATCHER_H_

#include <memory>
#include <ostream>
#include <utility>

#include "gmock/gmock.h"
#include "networktables/NetworkTableValue.h"

namespace nt {

class ValueMatcher
    : public ::testing::MatcherInterface<std::shared_ptr<Value>> {
 public:
  explicit ValueMatcher(std::shared_ptr<Value> goodval_)
      : goodval(std::move(goodval_)) {}

  bool MatchAndExplain(std::shared_ptr<Value> msg,
                       ::testing::MatchResultListener* listener) const override;
  void DescribeTo(::std::ostream* os) const override;
  void DescribeNegationTo(::std::ostream* os) const override;

 private:
  std::shared_ptr<Value> goodval;
};

inline ::testing::Matcher<std::shared_ptr<Value>> ValueEq(
    std::shared_ptr<Value> goodval) {
  return ::testing::MakeMatcher(new ValueMatcher(goodval));
}

}  // namespace nt

#endif  // NTCORE_VALUEMATCHER_H_
