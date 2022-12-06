#pragma once

#include <atomic>
#include <cstddef>
#include <iterator>
#include <list>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <thread>

namespace cytmwia {

template <typename T>
class MessqgeQueue {
 public:
  // ********** Types **********
  using size_type = size_t;
  using index_type = size_type;

  class OutputChannel {
   public:
    OutputChannel() = delete;
    size_type max_size();
    void max_size(size_type);
    size_type size();
    T pop();
    OutputChannel(MessqgeQueue<T>&, size_type = 0);  // TODO: be private

   private:
    void fit_max_size();

    MessqgeQueue<T>& mq_;
    std::atomic_intmax_t max_size_;

    typename std::list<T>::iterator iterator_;
    bool iterator_stagnated_;
    std::atomic_intmax_t iterator_index_;
    std::recursive_mutex pop_mu_;

    friend class MessqgeQueue<T>;
  };

  class InputChannel {
   public:
    InputChannel() = delete;
    template <typename T_>
    void push(T_&&);

   private:
    InputChannel(MessqgeQueue<T>&);
    MessqgeQueue<T>& mq_;
    std::recursive_mutex push_mu_;

    friend class MessqgeQueue<T>;
  };

  // ********** Interfaces **********

  MessqgeQueue() = delete;
  MessqgeQueue(size_type = 1);

  InputChannel& input_channel();
  OutputChannel& output_channel(index_type);

  std::vector<size_type> output_channels_size();

 private:
  void clean_front_data();

  std::list<T> data_;
  InputChannel input_channel_;
  std::deque<OutputChannel> output_channels_;

  friend class InputChannel;
  friend class OutputChannel;
};

template <typename T>
inline typename MessqgeQueue<T>::size_type
MessqgeQueue<T>::OutputChannel::max_size() {
  return max_size_.load();
}

template <typename T>
inline void MessqgeQueue<T>::OutputChannel::max_size(
    MessqgeQueue<T>::size_type new_size) {
  max_size_.store(new_size);
}

template <typename T>
inline typename MessqgeQueue<T>::size_type
MessqgeQueue<T>::OutputChannel::size() {
  fit_max_size();
  return mq_.data_.size() - iterator_index_.load();
}

template <typename T>
inline void MessqgeQueue<T>::OutputChannel::fit_max_size(void) {
  std::lock_guard<std::recursive_mutex> lk(pop_mu_);
  // First fit
  if (iterator_ == mq_.data_.end()) {
    iterator_ = mq_.data_.begin();
    if (iterator_ == mq_.data_.end()) {
      return;
    } else {
      iterator_stagnated_ = false;
    }
  }

  size_type _size =
      mq_.data_.size() - iterator_index_.load();  // Copy from size()
  size_type _max_size = max_size_.load();

  if (_max_size <= 0) return;

  std::make_signed_t<size_type> diff = _size - _max_size;
  if (0 < diff) {
    iterator_index_ += diff;
    for (decltype(diff) i = 0; i < diff; i++) iterator_++;
  }
}

template <typename T>
inline T MessqgeQueue<T>::OutputChannel::pop(void) {
  std::lock_guard<std::recursive_mutex> lk(pop_mu_);
  // Spin
  while (!size()) std::this_thread::sleep_for(std::chrono::nanoseconds(1));

  iterator_index_++;
  if (iterator_stagnated_) {
    iterator_++;
    iterator_stagnated_ = false;
  }
  if (std::next(iterator_) == mq_.data_.end()) {
    iterator_stagnated_ = true;
    return *iterator_;
  } else {
    return *(iterator_++);
  }
}

template <typename T>
inline MessqgeQueue<T>::OutputChannel::OutputChannel(MessqgeQueue<T>& mq,
                                                     size_type _max_size)
    : mq_(mq),
      max_size_(_max_size),
      iterator_(mq_.data_.begin()),
      iterator_stagnated_(true),
      iterator_index_(0) {}

template <typename T>
template <typename T_>
inline void MessqgeQueue<T>::InputChannel::push(T_&& o) {
  std::lock_guard<std::recursive_mutex> lk(push_mu_);
  mq_.data_.push_back(std::forward<T_>(o));
  mq_.clean_front_data();
}

template <typename T>
inline MessqgeQueue<T>::InputChannel::InputChannel(MessqgeQueue<T>& mq)
    : mq_(mq) {}

template <typename T>
inline MessqgeQueue<T>::MessqgeQueue(size_type count)
    : data_(), input_channel_(*this) {
  if (count <= 0) throw std::invalid_argument("`count` must greater than 0");
  for (size_type i = 0; i < count; i++) output_channels_.emplace_back(*this);
}

template <typename T>
inline typename MessqgeQueue<T>::InputChannel&
MessqgeQueue<T>::input_channel() {
  return input_channel_;
}

template <typename T>
inline typename MessqgeQueue<T>::OutputChannel& MessqgeQueue<T>::output_channel(
    typename MessqgeQueue<T>::index_type idx) {
  if (idx < output_channels_.size()) {
    return output_channels_[idx];
  } else {
    throw std::out_of_range("Not enough output channels.");
  }
}

template <typename T>
inline std::vector<typename MessqgeQueue<T>::size_type>
MessqgeQueue<T>::output_channels_size() {
  std::vector<typename MessqgeQueue<T>::size_type> res(output_channels_.size(),
                                                       0);
  for (size_type i = 0; i < output_channels_.size(); i++) {
    res[i] = output_channel(i).size();
  }
  return res;
}

template <typename T>
inline void MessqgeQueue<T>::clean_front_data() {
  size_type minidx = data_.size();
  for (size_type i = 0; i < output_channels_.size(); i++) {
    size_type idx = output_channels_[i].iterator_index_;
    minidx = minidx < idx ? minidx : idx;
  }

  if (minidx <= 1) return;
  minidx -= 1;  // preserve for iterator stagnation

  for (size_t _ = 0; _ < minidx; _++) data_.pop_front();
  for (size_type i = 0; i < output_channels_.size(); i++) {
    output_channels_[i].iterator_index_ -= minidx;
  }
}

}  // namespace cytmwia