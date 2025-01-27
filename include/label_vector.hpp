#ifndef LABELVECTOR_H_
#define LABELVECTOR_H_

#include <emmintrin.h>
#include <vector>

#include "config.hpp"

namespace fst {

class LabelVector {
 public:
  LabelVector() : num_bytes_(0), labels_(nullptr){};

  explicit LabelVector(const std::vector<std::vector<label_t> > &labels_per_level,
              const level_t start_level = 0,
              level_t end_level = 0 /* non-inclusive */) {
    if (end_level == 0) end_level = labels_per_level.size();

    num_bytes_ = 1;
    for (level_t level = start_level; level < end_level; level++)
      num_bytes_ += labels_per_level[level].size();

    labels_ = new label_t[num_bytes_];

    position_t pos = 0;
    for (level_t level = start_level; level < end_level; level++) {
      for (position_t idx = 0; idx < labels_per_level[level].size(); idx++) {
        labels_[pos] = labels_per_level[level][idx];
        pos++;
      }
    }
  }

  ~LabelVector() {
    delete[] labels_;
  }

  position_t getNumBytes() const { return num_bytes_; }

  position_t serializedSize() const {
    position_t size = sizeof(num_bytes_) + num_bytes_;
    sizeAlign(size);
    return size;
  }

  position_t size() const { return (sizeof(LabelVector) + num_bytes_); }

  label_t read(const position_t pos) const { return labels_[pos]; }

  label_t operator[](const position_t pos) const { return labels_[pos]; }

  bool search(label_t target, position_t &pos, position_t search_len) const;
  bool searchGreaterThan(label_t target, position_t &pos,
                         position_t search_len) const;

  bool binarySearch(label_t target, position_t &pos,
                    position_t search_len) const;
  bool simdSearch(label_t target, position_t &pos, position_t search_len) const;
  bool linearSearch(label_t target, position_t &pos,
                    position_t search_len) const;

  bool binarySearchGreaterThan(label_t target, position_t &pos,
                               position_t search_len) const;
  bool linearSearchGreaterThan(label_t target, position_t &pos,
                               position_t search_len) const;

  void serialize(char *&dst) const {
    memcpy(dst, &num_bytes_, sizeof(num_bytes_));
    dst += sizeof(num_bytes_);
    memcpy(dst, labels_, num_bytes_);
    dst += num_bytes_;
    align(dst);
  }

  static std::unique_ptr<LabelVector> deSerialize(char *&src) {
    auto lv = std::make_unique<LabelVector>();
    memcpy(&(lv->num_bytes_), src, sizeof(lv->num_bytes_));
    src += sizeof(lv->num_bytes_);
    lv->labels_ = const_cast<label_t *>(reinterpret_cast<const label_t *>(src));
    src += lv->num_bytes_;
    align(src);
    return lv;
  }

  void destroy() {

  }

 private:
  position_t num_bytes_;
  label_t *labels_;
};

bool LabelVector::search(const label_t target, position_t &pos,
                         position_t search_len) const {
  // skip terminator label
  if ((search_len > 1) && (labels_[pos] == kTerminator)) {
    pos++;
    search_len--;
  }

  if (search_len < 3) return linearSearch(target, pos, search_len);
  if (search_len < 12)
    return binarySearch(target, pos, search_len);
  else
    return simdSearch(target, pos, search_len);
}

bool LabelVector::searchGreaterThan(const label_t target, position_t &pos,
                                    position_t search_len) const {
  // skip terminator label
  if ((search_len > 1) && (labels_[pos] == kTerminator)) {
    pos++;
    search_len--;
  }

  if (search_len < 3)
    return linearSearchGreaterThan(target, pos, search_len);
  else
    return binarySearchGreaterThan(target, pos, search_len);
}

bool LabelVector::binarySearch(const label_t target, position_t &pos,
                               const position_t search_len) const {
  position_t l = pos;
  position_t r = pos + search_len;
  while (l < r) {
    position_t m = (l + r) >> 1;
    if (target < labels_[m]) {
      r = m;
    } else if (target == labels_[m]) {
      pos = m;
      return true;
    } else {
      l = m + 1;
    }
  }
  return false;
}

bool LabelVector::simdSearch(const label_t target, position_t &pos,
                             const position_t search_len) const {
  // CA: todo fix invalid read
  position_t num_labels_searched = 0;
  position_t num_labels_left = search_len;
  while ((num_labels_left >> 4) > 0 && pos + num_labels_searched + 15 < num_bytes_) { // while at least 16 elements remain and a 16 byte simd read is safe
    // check that we are still allowed to read
    label_t *start_ptr = labels_ + pos + num_labels_searched;
    assert(num_bytes_ - reinterpret_cast<uintptr_t>(start_ptr) + reinterpret_cast<uintptr_t>(labels_) >= 16); // needs to be safe to read 16 bytes from start_ptr
    __m128i cmp =
        _mm_cmpeq_epi8(_mm_set1_epi8(target),
                       _mm_loadu_si128(reinterpret_cast<__m128i *>(start_ptr)));
    unsigned check_bits = _mm_movemask_epi8(cmp);
    if (check_bits) {
      pos += (num_labels_searched + __builtin_ctz(check_bits));
      return true;
    }
    num_labels_searched += 16;
    num_labels_left -= 16;
  }

  while (num_labels_left > 0) {
    assert(pos + num_labels_searched < num_bytes_);
    if (labels_[pos + num_labels_searched] == target) {
      pos += num_labels_searched;
      return true;
    }
    ++num_labels_searched;
    --num_labels_left;
  }

  return false;
}

bool LabelVector::linearSearch(const label_t target, position_t &pos,
                               const position_t search_len) const {
  for (position_t i = 0; i < search_len; i++) {
    if (target == labels_[pos + i]) {
      pos += i;
      return true;
    }
  }
  return false;
}

bool LabelVector::binarySearchGreaterThan(const label_t target, position_t &pos,
                                          const position_t search_len) const {
  position_t l = pos;
  position_t r = pos + search_len;
  while (l < r) {
    position_t m = (l + r) >> 1;
    if (target < labels_[m]) {
      r = m;
    } else if (target == labels_[m]) {
      if (m < pos + search_len - 1) {
        pos = m + 1;
        return true;
      }
      return false;
    } else {
      l = m + 1;
    }
  }

  if (l < pos + search_len) {
    pos = l;
    return true;
  }
  return false;
}

bool LabelVector::linearSearchGreaterThan(const label_t target, position_t &pos,
                                          const position_t search_len) const {
  for (position_t i = 0; i < search_len; i++) {
    if (labels_[pos + i] > target) {
      pos += i;
      return true;
    }
  }
  return false;
}

}  // namespace fst

#endif  // LABELVECTOR_H_
