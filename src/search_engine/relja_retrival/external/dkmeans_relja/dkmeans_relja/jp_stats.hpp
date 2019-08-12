/**
 * James Philbin <philbinj@gmail.com>
 * Engineering Department
 * University of Oxford
 * Copyright (C) 2006. All rights reserved.
 * 
 * Use and modify all you like, but do NOT redistribute. No warranty is 
 * expressed or implied. No liability or responsibility is assumed.
 */
#ifndef __JP_STATS_HPP
#define __JP_STATS_HPP

template<class T>
class
jp_stats_mean_var
{
public:
  typedef T value_type;
  
private:
  size_t count_;
  value_type sum_x_;
  value_type sum_xx_;

public:
  jp_stats_mean_var() : count_(0), sum_x_(0), sum_xx_(0) { }

  template<class F>
  void
  operator()(const F& val)
  {
    sum_x_ += val;
    sum_xx_ += (val*val);
    count_++;
  }

  value_type
  mean() const
  {
    return sum_x_/count_;
  }

  value_type
  variance() const
  {
    if (count_ == 1) return value_type(0);
    else
      return (sum_xx_ - (value_type(1)/count_)*sum_x_*sum_x_)/(count_ - 1);
      //return std::max((value_type(1)/(count_ - 1)) * (sum_xx_ - (value_type(1)/count_)*sum_x_*sum_x_), value_type(0.0));
  }

  size_t
  count() const
  {
    return count_;
  }
};


#endif
